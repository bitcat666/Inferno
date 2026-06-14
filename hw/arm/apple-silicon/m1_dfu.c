/*
 * Apple M1 USB DFU Controller Emulation
 * =====================================
 *
 * Emulates the M1 USB Device Firmware Upgrade (DFU) controller
 * for testing MacActivationTool's activation bypass flow.
 *
 * The M1 in DFU mode presents as a USB device with:
 *   VID: 0x05AC (Apple)
 *   PID: 0x1227 (standard Apple DFU PID)
 *
 * DFU protocol flow:
 *   1. Host sends DFU_GETSTATUS → device returns state=DFU_IDLE
 *   2. Host sends DFU_DNLOAD with iBSS image
 *   3. Device processes iBSS and transitions to DFU_DNLOAD_IDLE
 *   4. Host sends DFU_DNLOAD with iBEC image
 *   5. Device processes iBEC and transitions to DFU_MANIFEST_SYNC
 *   6. Reset into Recovery mode
 *
 * This model intercepts the RestoreOptions plist to verify
 * allow-hactivation=true is properly injected.
 *
 * Based on the DFU protocol analysis from iBoot j716s reverse engineering.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/sysbus.h"
#include "hw/usb.h"
#include "hw/arm/apple-silicon/t8103.h"
#include "migration/vmstate.h"

/* ── DFU Protocol Constants ─────────────────────────────── */

#define DFU_DETACH      0x00
#define DFU_DNLOAD      0x01
#define DFU_UPLOAD      0x02
#define DFU_GETSTATUS   0x03
#define DFU_CLRSTATUS   0x04
#define DFU_GETSTATE    0x05
#define DFU_ABORT       0x06

/* DFU states */
#define DFU_STATE_APP_IDLE              0
#define DFU_STATE_APP_DETACH            1
#define DFU_STATE_DFU_IDLE              2
#define DFU_STATE_DFU_DNLOAD_SYNC       3
#define DFU_STATE_DFU_DNLOAD_BUSY       4
#define DFU_STATE_DFU_DNLOAD_IDLE       5
#define DFU_STATE_DFU_MANIFEST_SYNC     6
#define DFU_STATE_DFU_MANIFEST          7
#define DFU_STATE_DFU_MANIFEST_WAIT_RESET 8
#define DFU_STATE_DFU_UPLOAD_IDLE       9
#define DFU_STATE_DFU_ERROR             10

#define DFU_POLL_TIMEOUT_MS  200  /* Minimum poll timeout */
#define DFU_MAX_PACKET_SIZE  0x800

/* ── Device Descriptor ──────────────────────────────────── */

static const uint8_t m1_dfu_device_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType: DEVICE */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0x00,       /* bDeviceClass: (at interface) */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    0x40,       /* bMaxPacketSize0: 64 */
    0xAC, 0x05, /* idVendor: 0x05AC (Apple) */
    0x27, 0x12, /* idProduct: 0x1227 (Apple DFU) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations */
};

static const uint8_t m1_dfu_config_descriptor[] = {
    0x09,       /* bLength */
    0x02,       /* bDescriptorType: CONFIG */
    0x1B, 0x00, /* wTotalLength */
    0x01,       /* bNumInterfaces */
    0x01,       /* bConfigurationValue */
    0x00,       /* iConfiguration */
    0x80,       /* bmAttributes: bus-powered */
    0x32,       /* bMaxPower: 100mA */
    /* Interface descriptor */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: INTERFACE */
    0x00,       /* bInterfaceNumber */
    0x00,       /* bAlternateSetting */
    0x00,       /* bNumEndpoints (DFU uses control EP only) */
    0xFE,       /* bInterfaceClass: APPLICATION */
    0x01,       /* bInterfaceSubClass: DFU */
    0x02,       /* bInterfaceProtocol: DFU mode */
    0x05,       /* iInterface */
    /* DFU Functional descriptor */
    0x09,       /* bLength */
    0x21,       /* bDescriptorType: DFU FUNCTIONAL */
    0x0F,       /* bmAttributes: detach, upload, download, manifest tolerant */
    0x00, 0xFF, /* wDetachTimeout: 255ms */
    0x00, 0x08, /* wTransferSize: 2048 */
    0x00, 0x01, /* bcdDFUVersion: 1.0 */
};

/* ── String Descriptors ─────────────────────────────────── */

static const char m1_dfu_string_manufacturer[] = "Apple Inc.";
static const char m1_dfu_string_product[] = "Apple M1 DFU Device";
static const char m1_dfu_string_serial[] = "CPID:8103 CPRV:01 CPFM:03 SCEP:01 BDID:22 ECID:00000001A2B3C4D5 IBFL:1C SRTG:[iBoot-20356.0.0.0.15]";
static const char m1_dfu_string_interface[] = "Apple DFU";

/* ── M1 DFU Device State ────────────────────────────────── */

#define TYPE_M1_DFU "m1-dfu"
OBJECT_DECLARE_SIMPLE_TYPE(M1DFUState, M1_DFU)

typedef struct M1DFUState {
    SysBusDevice parent_obj;

    /* DFU state machine */
    uint8_t dfu_state;
    uint8_t dfu_status;
    uint32_t poll_timeout;

    /* Firmware buffers */
    uint8_t *ibss_data;
    uint32_t ibss_len;
    uint8_t *ibec_data;
    uint32_t ibec_len;
    uint8_t *restore_options;
    uint32_t restore_options_len;

    /* Device info */
    uint32_t cpid;   /* 0x8103 for M1 */
    uint32_t bdid;   /* Board ID */
    uint64_t ecid;   /* Exclusive Chip ID */

    /* Download buffer */
    uint8_t download_buf[DFU_MAX_PACKET_SIZE];
    uint32_t download_offset;
    uint32_t download_total;

    /* Status tracking */
    bool ibss_received;
    bool ibec_received;
    bool restore_options_received;
    bool allow_hactivation_detected;

    /* Memory regions */
    MemoryRegion mmio;
} M1DFUState;

/* ── DFU Protocol Implementation ────────────────────────── */

static void m1_dfu_reset_state(M1DFUState *s)
{
    s->dfu_state = DFU_STATE_DFU_IDLE;
    s->dfu_status = 0;
    s->poll_timeout = DFU_POLL_TIMEOUT_MS;
    s->download_offset = 0;
    s->download_total = 0;
}

static uint64_t m1_dfu_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    M1DFUState *s = opaque;

    switch (addr) {
    case 0x00:  /* DFU_STATE */
        return s->dfu_state;
    case 0x04:  /* CPID */
        return s->cpid;
    case 0x08:  /* BDID */
        return s->bdid;
    case 0x0C:  /* ECID low */
        return (uint32_t)(s->ecid & 0xFFFFFFFF);
    case 0x10:  /* ECID high */
        return (uint32_t)(s->ecid >> 32);
    case 0x14:  /* Status flags */
        return (s->ibss_received ? 1 : 0) |
               (s->ibec_received ? 2 : 0) |
               (s->restore_options_received ? 4 : 0) |
               (s->allow_hactivation_detected ? 8 : 0);
    default:
        qemu_log_mask(LOG_UNIMP, "m1-dfu: unknown read @ 0x%lx\n", addr);
        return 0;
    }
}

static void m1_dfu_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    M1DFUState *s = opaque;

    switch (addr) {
    case 0x00:  /* Reset DFU state */
        m1_dfu_reset_state(s);
        break;
    case 0x04:  /* CPID */
        s->cpid = (uint32_t)val;
        break;
    case 0x08:  /* BDID */
        s->bdid = (uint32_t)val;
        break;
    case 0x0C:  /* ECID low */
        s->ecid = (s->ecid & 0xFFFFFFFF00000000ULL) | (uint32_t)val;
        break;
    case 0x10:  /* ECID high */
        s->ecid = (s->ecid & 0xFFFFFFFFULL) | ((uint64_t)val << 32);
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "m1-dfu: unknown write @ 0x%lx = 0x%lx\n",
                      addr, val);
        break;
    }
}

static const MemoryRegionOps m1_dfu_mmio_ops = {
    .read = m1_dfu_mmio_read,
    .write = m1_dfu_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

/* ── RestoreOptions Plist Parser ────────────────────────── */

static bool m1_dfu_parse_restore_options(M1DFUState *s,
                                          const uint8_t *data, uint32_t len)
{
    /*
     * Quick check for allow-hactivation in the plist data.
     * The plist is XML format: <key>allow-hactivation</key><true/>
     *
     * For proper parsing we'd use plistlib, but for the emulator
     * we just scan for the key string and verify it's followed by <true/>.
     */
    const char *key = "allow-hactivation";
    size_t key_len = strlen(key);

    for (uint32_t i = 0; i + key_len + 20 < len; i++) {
        if (memcmp(data + i, key, key_len) == 0) {
            /* Found the key, check for <true/> after it */
            const char *rest = (const char *)(data + i + key_len);
            if (strstr(rest, "<true/>") || strstr(rest, "<true>") ||
                strstr(rest, "<true />")) {
                s->allow_hactivation_detected = true;
                return true;
            }
        }
    }
    return false;
}

/* ── USB DFU Control Transfer Handler ───────────────────── */

bool m1_dfu_handle_control_transfer(M1DFUState *s,
                                     uint8_t request_type,
                                     uint8_t request,
                                     uint16_t value,
                                     uint16_t index,
                                     uint8_t *data,
                                     uint32_t *length)
{
    switch (request) {
    case DFU_DETACH:
        qemu_log("m1-dfu: DFU_DETACH → entering DFU mode\n");
        s->dfu_state = DFU_STATE_DFU_IDLE;
        *length = 0;
        return true;

    case DFU_DNLOAD:
        if (*length == 0) {
            /* Zero-length DNLOAD: finalize download, enter manifest */
            qemu_log("m1-dfu: DFU_DNLOAD zero-length → manifest\n");

            if (!s->ibss_received) {
                /* First firmware: iBSS */
                s->ibss_received = true;
                s->ibss_data = g_memdup2(s->download_buf, s->download_total);
                s->ibss_len = s->download_total;
                qemu_log("m1-dfu: iBSS received: %u bytes\n", s->ibss_len);
                s->dfu_state = DFU_STATE_DFU_DNLOAD_IDLE;

            } else if (!s->ibec_received) {
                /* Second firmware: iBEC */
                s->ibec_received = true;
                s->ibec_data = g_memdup2(s->download_buf, s->download_total);
                s->ibec_len = s->download_total;
                qemu_log("m1-dfu: iBEC received: %u bytes\n", s->ibec_len);
                s->dfu_state = DFU_STATE_DFU_MANIFEST_SYNC;

            } else {
                /* Subsequent: could be RestoreOptions or kernel/ramdisk */
                s->restore_options_received = true;
                s->restore_options =
                    g_memdup2(s->download_buf, s->download_total);
                s->restore_options_len = s->download_total;

                /* Check for activation bypass flag */
                if (m1_dfu_parse_restore_options(s, s->download_buf,
                                                  s->download_total)) {
                    qemu_log("m1-dfu: *** allow-hactivation=true DETECTED! ***\n"
                             "m1-dfu: *** Activation lock bypass ACTIVE ***\n");
                } else {
                    qemu_log("m1-dfu: RestoreOptions received (%u bytes) "
                             "— no activation bypass\n", s->download_total);
                }
                s->dfu_state = DFU_STATE_DFU_MANIFEST_WAIT_RESET;
            }

            s->download_offset = 0;
            s->download_total = 0;
        } else {
            /* Non-zero DNLOAD: buffer the firmware data */
            if (s->download_offset + *length <= sizeof(s->download_buf)) {
                memcpy(s->download_buf + s->download_offset, data, *length);
                s->download_offset += *length;
                s->download_total = s->download_offset;
            }
            s->dfu_state = DFU_STATE_DFU_DNLOAD_IDLE;
        }
        *length = 0;
        return true;

    case DFU_UPLOAD:
        /*
         * DFU_UPLOAD returns device identification:
         * bytes 0-3:  CPID
         * bytes 4:    BDID
         * bytes 5-7:  padding
         * bytes 8-15: ECID
         */
        if (*length >= 16) {
            data[0] = (s->cpid >> 0)  & 0xFF;
            data[1] = (s->cpid >> 8)  & 0xFF;
            data[2] = (s->cpid >> 16) & 0xFF;
            data[3] = (s->cpid >> 24) & 0xFF;
            data[4] = (uint8_t)s->bdid;
            data[5] = 0;
            data[6] = 0;
            data[7] = 0;
            data[8]  = (s->ecid >> 0)  & 0xFF;
            data[9]  = (s->ecid >> 8)  & 0xFF;
            data[10] = (s->ecid >> 16) & 0xFF;
            data[11] = (s->ecid >> 24) & 0xFF;
            data[12] = (s->ecid >> 32) & 0xFF;
            data[13] = (s->ecid >> 40) & 0xFF;
            data[14] = (s->ecid >> 48) & 0xFF;
            data[15] = (s->ecid >> 56) & 0xFF;
            *length = 16;
        }
        return true;

    case DFU_GETSTATUS:
        /* 6-byte status response */
        if (*length >= 6) {
            data[0] = s->dfu_status;           /* bStatus */
            data[1] = s->poll_timeout & 0xFF;   /* bwPollTimeout low */
            data[2] = (s->poll_timeout >> 8) & 0xFF;
            data[3] = (s->poll_timeout >> 16) & 0xFF;
            data[4] = s->dfu_state;             /* bState */
            data[5] = 0;                        /* iString */
            *length = 6;
        }
        return true;

    case DFU_CLRSTATUS:
        s->dfu_status = 0;
        s->dfu_state = DFU_STATE_DFU_IDLE;
        *length = 0;
        return true;

    case DFU_GETSTATE:
        if (*length >= 1) {
            data[0] = s->dfu_state;
            *length = 1;
        }
        return true;

    case DFU_ABORT:
        qemu_log("m1-dfu: DFU_ABORT\n");
        m1_dfu_reset_state(s);
        *length = 0;
        return true;

    default:
        qemu_log_mask(LOG_UNIMP, "m1-dfu: unknown DFU request 0x%02x\n",
                      request);
        return false;
    }
}

/* ── QEMU Device Interface ──────────────────────────────── */

static void m1_dfu_realize(DeviceState *dev, Error **errp)
{
    M1DFUState *s = M1_DFU(dev);

    memory_region_init_io(&s->mmio, OBJECT(dev), &m1_dfu_mmio_ops, s,
                          "m1-dfu-mmio", 0x1000);

    /* Default M1 device info */
    s->cpid = 0x8103;
    s->bdid = 0x22;
    s->ecid = 0x00000001A2B3C4D5ULL;
    s->dfu_state = DFU_STATE_DFU_IDLE;
    s->dfu_status = 0;
    s->poll_timeout = DFU_POLL_TIMEOUT_MS;
    s->ibss_received = false;
    s->ibec_received = false;
    s->allow_hactivation_detected = false;

    qemu_log("m1-dfu: M1 DFU controller initialized\n"
             "  CPID=0x%04X BDID=0x%02X ECID=0x%016llX\n"
             "  VID:PID = 0x05AC:0x1227\n"
             "  Initial state: DFU_IDLE\n",
             s->cpid, s->bdid, s->ecid);
}

static void m1_dfu_reset(DeviceState *dev)
{
    M1DFUState *s = M1_DFU(dev);
    m1_dfu_reset_state(s);
}

/* ── Properties ─────────────────────────────────────────── */

static Property m1_dfu_properties[] = {
    DEFINE_PROP_UINT32("cpid", M1DFUState, cpid, 0x8103),
    DEFINE_PROP_UINT32("bdid", M1DFUState, bdid, 0x22),
    DEFINE_PROP_UINT64("ecid", M1DFUState, ecid, 0x00000001A2B3C4D5ULL),
    DEFINE_PROP_END_OF_LIST(),
};

/* ── VMState ────────────────────────────────────────────── */

static const VMStateDescription vmstate_m1_dfu = {
    .name = "m1-dfu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(dfu_state, M1DFUState),
        VMSTATE_UINT8(dfu_status, M1DFUState),
        VMSTATE_UINT32(cpid, M1DFUState),
        VMSTATE_UINT32(bdid, M1DFUState),
        VMSTATE_UINT64(ecid, M1DFUState),
        VMSTATE_END_OF_LIST()
    }
};

/* ── Type Info ──────────────────────────────────────────── */

static void m1_dfu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = m1_dfu_realize;
    dc->reset = m1_dfu_reset;
    dc->vmsd = &vmstate_m1_dfu;
    device_class_set_props(dc, m1_dfu_properties);
    dc->desc = "Apple M1 USB DFU Controller";
}

static const TypeInfo m1_dfu_info = {
    .name          = TYPE_M1_DFU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(M1DFUState),
    .class_init    = m1_dfu_class_init,
};

static void m1_dfu_register_types(void)
{
    type_register_static(&m1_dfu_info);
}

type_init(m1_dfu_register_types);

/*
 * ── Public API ───────────────────────────────────────────
 *
 * Other modules can query the DFU status via these functions.
 */

M1DFUState *m1_dfu_get(MachineState *machine)
{
    Object *obj = object_resolve_path_type("", TYPE_M1_DFU, NULL);
    return obj ? M1_DFU(obj) : NULL;
}

bool m1_dfu_activation_bypass_active(M1DFUState *s)
{
    return s ? s->allow_hactivation_detected : false;
}

bool m1_dfu_is_in_recovery(M1DFUState *s)
{
    return s && s->dfu_state == DFU_STATE_DFU_MANIFEST_WAIT_RESET;
}

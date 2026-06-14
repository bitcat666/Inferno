/*
 * Apple T8103 SoC (M1).
 *
 * Based on the T8030 (A13) implementation by VisualEhrmanntraut and chris-pcguy.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef HW_ARM_APPLE_SILICON_T8103_H
#define HW_ARM_APPLE_SILICON_T8103_H

#include "qemu/osdep.h"
#include "exec/hwaddr.h"
#include "hw/arm/apple-silicon/m1.h"
#include "hw/arm/apple-silicon/boot.h"
#include "hw/boards.h"
#include "hw/sysbus.h"
#include "hw/usb/tcp-usb.h"
#include "system/kvm.h"

#define TYPE_APPLE_T8103 MACHINE_TYPE_NAME("t8103")

#define APPLE_T8103(obj) \
    OBJECT_CHECK(AppleT8103MachineState, (obj), TYPE_APPLE_T8103)

typedef struct {
    MachineClass parent;
} AppleT8103MachineClass;

typedef struct {
    MachineState parent;

    hwaddr armio_base;
    hwaddr armio_size;
    unsigned long dram_size;
    AppleM1State *cpus[M1_MAX_CPU];
    AppleM1Cluster clusters[M1_MAX_CLUSTER];
    SysBusDevice *aic;
    MachoHeader64 *kernel;
    AppleDTNode *device_tree;
    uint8_t *trustcache;
    char *securerom;
    gsize securerom_size;
    AppleBootInfo boot_info;
    AppleVideoArgs video_args;
    char *trustcache_filename;
    char *ticket_filename;
    char *sep_rom_filename;
    char *sep_fw_filename;
    char *securerom_filename;
    uint32_t sio_protocol;
    uint32_t build_version;
    uint64_t ecid;
    Notifier init_done_notifier;
    hwaddr panic_base;
    hwaddr panic_size;
    uint8_t pmgr_reg[0x100000];
    MemoryRegion amcc;
    uint8_t amcc_reg[0x100000];
    bool kaslr_off;
    bool force_dfu;
    uint32_t board_id;
    uint32_t chip_revision;
    USBTCPRemoteConnType usb_conn_type;
    char *usb_conn_addr;
    uint16_t usb_conn_port;
    char *model_number;
    char *region_info;
    char *config_number;
    char *serial_number;
    char *mlb_serial_number;
    char *regulatory_model;
    uint32_t disp_width;
    uint32_t disp_height;
} AppleT8103MachineState;

#endif /* HW_ARM_APPLE_SILICON_T8103_H */

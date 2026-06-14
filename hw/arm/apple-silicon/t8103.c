/*
 * Apple T8103 SoC (M1) — minimal implementation for DFU testing.
 *
 * Based on t8030.c (A13) by VisualEhrmanntraut & chris-pcguy.
 * M1: 4× Firestorm (P) + 4× Icestorm (E) = 8 cores, 2 clusters.
 * Platform: t8103.
 */

#include "qemu/osdep.h"
#include "hw/arm/apple-silicon/m1.h"
#include "hw/arm/apple-silicon/boot.h"
#include "hw/arm/apple-silicon/dart.h"
#include "hw/arm/apple-silicon/dt.h"
#include "hw/arm/apple-silicon/mem.h"
#include "hw/arm/apple-silicon/t8103.h"
#include "hw/intc/apple_aic.h"
#include "hw/misc/apple-silicon/smc.h"
#include "hw/nvram/apple_nvram.h"
#include "hw/pci-host/apcie.h"
#include "hw/watchdog/apple_wdt.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "system/reset.h"
#include "system/system.h"

/* ── M1-specific constants ──────────────────────────────── */

#define T8103_ARMIO_BASE  0x2000000000ULL
#define T8103_ARMIO_SIZE  (128 * MiB)

/* M1: 4 P-cores + 4 E-cores */
#define T8103_NUM_PCORES 4
#define T8103_NUM_ECORES 4
#define T8103_TOTAL_CPUS (T8103_NUM_PCORES + T8103_NUM_ECORES)
#define T8103_NUM_CLUSTERS 2

/* Cluster types: 1 = P-cluster, 0 = E-cluster */
#define CLUSTER_TYPE_P 1
#define CLUSTER_TYPE_E 0

/* M1 board IDs */
#define T8103_DEFAULT_BOARD_ID  0x22
#define T8103_DEFAULT_CHIP_REV  0x01

/* ── SoC implementation ─────────────────────────────────── */

static uint32_t t8103_real_cpu_count(AppleT8103MachineState *s)
{
    return MACHINE(s)->smp.cpus;
}

static void t8103_start_cpus(AppleT8103MachineState *s, uint64_t cpu_mask)
{
    for (int i = 0; i < M1_MAX_CPU; i++) {
        if (cpu_mask & (1ULL << i)) {
            AppleM1State *cpu = s->cpus[i];
            if (cpu && !apple_m1_is_off(cpu)) {
                apple_m1_set_on(cpu);
            }
        }
    }
}

static void t8103_create_cpus(AppleT8103MachineState *s)
{
    MachineState *ms = MACHINE(s);
    int cpu_index = 0;

    /* P-cluster: 4× Firestorm */
    for (int i = 0; i < T8103_NUM_PCORES && cpu_index < ms->smp.cpus; i++, cpu_index++) {
        char *name = g_strdup_printf("m1-pcpu%d", i);
        s->cpus[cpu_index] = apple_m1_create(name, cpu_index, i,
                                             0, CLUSTER_TYPE_P);
        g_free(name);
    }

    /* E-cluster: 4× Icestorm */
    for (int i = 0; i < T8103_NUM_ECORES && cpu_index < ms->smp.cpus; i++, cpu_index++) {
        char *name = g_strdup_printf("m1-ecpu%d", i);
        s->cpus[cpu_index] = apple_m1_create(name, cpu_index, i + T8103_NUM_PCORES,
                                             1, CLUSTER_TYPE_E);
        g_free(name);
    }

    /* Create clusters */
    s->clusters[0].cluster_type = CLUSTER_TYPE_P;
    s->clusters[1].cluster_type = CLUSTER_TYPE_E;

    for (int i = 0; i < T8103_NUM_PCORES && i < ms->smp.cpus; i++) {
        s->clusters[0].cpus[i] = s->cpus[i];
    }
    for (int i = 0; i < T8103_NUM_ECORES && (i + T8103_NUM_PCORES) < ms->smp.cpus; i++) {
        s->clusters[1].cpus[i] = s->cpus[i + T8103_NUM_PCORES];
    }
}

static void t8103_init(MachineState *machine)
{
    AppleT8103MachineState *s = APPLE_T8103(machine);

    s->armio_base = T8103_ARMIO_BASE;
    s->armio_size = T8103_ARMIO_SIZE;
    s->board_id = T8103_DEFAULT_BOARD_ID;
    s->chip_revision = T8103_DEFAULT_CHIP_REV;

    if (!s->dram_size) {
        s->dram_size = 8 * GiB;
    }

    /* Create a simple memory region */
    memory_region_add_subregion(get_system_memory(), 0x800000000ULL,
                                g_new0(MemoryRegion, 1));

    /* Create 8 M1 CPU cores */
    t8103_create_cpus(s);

    /* Create AIC (Apple Interrupt Controller) */
    s->aic = SYS_BUS_DEVICE(qdev_new(TYPE_APPLE_AIC));
    sysbus_realize_and_unref(s->aic, &error_fatal);

    /* Start all CPUs */
    t8103_start_cpus(s, (1ULL << ms->smp.cpus) - 1);

    qemu_log("T8103 (M1): %d cores, %lu GB RAM, board_id=0x%x\n",
             ms->smp.cpus, s->dram_size / GiB, s->board_id);
}

static void t8103_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "Apple T8103 (M1)";
    mc->init = t8103_init;
    mc->max_cpus = M1_MAX_CPU;
    mc->min_cpus = 1;
    mc->default_cpus = M1_MAX_CPU;
    mc->default_ram_size = 8 * GiB;
}

static const TypeInfo t8103_machine_info = {
    .name          = TYPE_APPLE_T8103,
    .parent        = TYPE_MACHINE,
    .class_init    = t8103_machine_class_init,
    .instance_size = sizeof(AppleT8103MachineState),
};

static void t8103_machine_register_types(void)
{
    type_register_static(&t8103_machine_info);
}

type_init(t8103_machine_register_types);

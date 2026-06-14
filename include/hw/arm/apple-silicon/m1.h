/*
 * Apple M1 CPU (T8103).
 *
 * M1 = 4× Firestorm (P-cores) + 4× Icestorm (E-cores) = 8 cores, 2 clusters.
 * Based on the A13 CPU implementation.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License.
 */

#ifndef HW_ARM_APPLE_SILICON_M1_H
#define HW_ARM_APPLE_SILICON_M1_H

#include "qemu/osdep.h"
#include "hw/arm/apple-silicon/dt.h"
#include "hw/cpu/cluster.h"
#include "qemu/queue.h"
#include "system/memory.h"
#include "cpu.h"

#define M1_MAX_CPU 8
#define M1_MAX_CLUSTER 2

#define TYPE_APPLE_M1 "apple-m1-cpu"
OBJECT_DECLARE_TYPE(AppleM1State, AppleM1Class, APPLE_M1)

#define TYPE_APPLE_M1_CLUSTER "apple-m1-cluster"
OBJECT_DECLARE_SIMPLE_TYPE(AppleM1Cluster, APPLE_M1_CLUSTER)

#define M1_CPREG_VAR_NAME(name) cpreg_##name
#define M1_CPREG_VAR_DEF(name) uint64_t M1_CPREG_VAR_NAME(name)

#define kDeferredIPITimerDefault 64000

typedef struct AppleM1Class {
    ARMCPUClass base_class;
    DeviceRealize parent_realize;
    DeviceUnrealize parent_unrealize;
    ResettablePhases parent_phases;
} AppleM1Class;

typedef struct AppleM1State {
    ARMCPU parent_obj;
    MemoryRegion memory;
    MemoryRegion sysmem;
    uint32_t cpu_id;
    uint32_t phys_id;
    uint32_t cluster_id;
    uint64_t ipi_sr;
    qemu_irq fast_ipi;
    M1_CPREG_VAR_DEF(ARM64_REG_EHID3);
    M1_CPREG_VAR_DEF(ARM64_REG_EHID4);
    M1_CPREG_VAR_DEF(ARM64_REG_EHID10);
    M1_CPREG_VAR_DEF(ARM64_REG_HID0);
    M1_CPREG_VAR_DEF(ARM64_REG_HID1);
    M1_CPREG_VAR_DEF(ARM64_REG_HID3);
    M1_CPREG_VAR_DEF(ARM64_REG_HID4);
    M1_CPREG_VAR_DEF(ARM64_REG_HID5);
    M1_CPREG_VAR_DEF(ARM64_REG_HID7);
    M1_CPREG_VAR_DEF(ARM64_REG_HID8);
    M1_CPREG_VAR_DEF(ARM64_REG_HID9);
    M1_CPREG_VAR_DEF(ARM64_REG_HID11);
    M1_CPREG_VAR_DEF(ARM64_REG_HID13);
    M1_CPREG_VAR_DEF(ARM64_REG_HID14);
    M1_CPREG_VAR_DEF(ARM64_REG_HID16);
    M1_CPREG_VAR_DEF(ARM64_REG_MMU_ERR_STS);
    M1_CPREG_VAR_DEF(ARM64_REG_LSU_ERR_STS);
    M1_CPREG_VAR_DEF(ARM64_REG_FED_ERR_STS);
    M1_CPREG_VAR_DEF(ARM64_REG_LLC_ERR_STS);
    M1_CPREG_VAR_DEF(ARM64_REG_LLC_ERR_INF);
    M1_CPREG_VAR_DEF(ARM64_REG_LLC_ERR_ADR);
    M1_CPREG_VAR_DEF(ARM64_REG_LLC_RAM_CONFIG);
    M1_CPREG_VAR_DEF(IMP_BARRIER_LBSY_BST_SYNC_W0_EL0);
    M1_CPREG_VAR_DEF(IMP_BARRIER_LBSY_BST_SYNC_W1_EL0);
    M1_CPREG_VAR_DEF(PMC0);
    M1_CPREG_VAR_DEF(PMC1);
    M1_CPREG_VAR_DEF(PMCR0);
    M1_CPREG_VAR_DEF(PMCR1);
    M1_CPREG_VAR_DEF(PMSR);
    M1_CPREG_VAR_DEF(S3_4_c15_c0_5);
    M1_CPREG_VAR_DEF(AMX_STATUS_EL1);
    M1_CPREG_VAR_DEF(AMX_CTL_EL1);
    M1_CPREG_VAR_DEF(ARM64_REG_CPU_OVRD);
    M1_CPREG_VAR_DEF(ARM64_REG_ACC_OVRD);
    M1_CPREG_VAR_DEF(ARM64_REG_ACC_CFG);
    M1_CPREG_VAR_DEF(S3_5_c15_c10_1);
    M1_CPREG_VAR_DEF(SYS_ACC_PWR_DN_SAVE);
    M1_CPREG_VAR_DEF(UPMPCM);
    M1_CPREG_VAR_DEF(UPMCR0);
    M1_CPREG_VAR_DEF(UPMSR);
} AppleM1State;

typedef struct AppleM1Cluster {
    CPUClusterState parent_obj;
    uint32_t cluster_type;
    MemoryRegion mr;
    AppleM1State *cpus[M1_MAX_CPU];
    uint32_t deferredIPI[M1_MAX_CPU];
    uint32_t noWakeIPI[M1_MAX_CPU];
    uint64_t ipi_cr;
    QTAILQ_ENTRY(AppleM1Cluster) next;
    M1_CPREG_VAR_DEF(CTRR_A_LWR_EL1);
    M1_CPREG_VAR_DEF(CTRR_A_UPR_EL1);
    M1_CPREG_VAR_DEF(CTRR_B_LWR_EL1);
    M1_CPREG_VAR_DEF(CTRR_B_UPR_EL1);
    M1_CPREG_VAR_DEF(CTRR_CTL_EL1);
    M1_CPREG_VAR_DEF(CTRR_LOCK_EL1);
} AppleM1Cluster;

AppleM1State *apple_m1_create(const char *name, uint32_t cpu_id,
                              uint32_t phys_id, uint32_t cluster_id,
                              uint16_t cluster_type);
AppleM1State *apple_m1_from_node(AppleDTNode *node);
bool apple_m1_is_asleep(const AppleM1State *acpu);
bool apple_m1_is_off(const AppleM1State *acpu);
void apple_m1_set_on(AppleM1State *acpu);
void apple_m1_reset(AppleM1State *acpu);
void apple_m1_set_off(AppleM1State *acpu);

#endif /* HW_ARM_APPLE_SILICON_M1_H */

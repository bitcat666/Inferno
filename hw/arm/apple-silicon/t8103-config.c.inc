/*
 * Apple T8103 (M1) SoC configuration.
 *
 * M1 = 4× Firestorm (P-core, up to 3.2GHz) + 4× Icestorm (E-core, up to 2.0GHz)
 * 8 cores in 2 clusters. Platform t8103. First consumer Apple Silicon Mac.
 *
 * Clock frequencies, voltage states, and bridge settings extracted
 * from M1 DeviceTree (j274ap / Mac mini M1 2020).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License.
 */

/* ── M1 Voltage States ────────────────────────────────────
 *
 * Apple Silicon uses dynamic voltage/frequency scaling per cluster.
 * Each state is {frequency_khz, voltage_mv}.
 * M1 has 2 clusters: P-cluster (Firestorm) and E-cluster (Icestorm).
 */

/* P-Cluster voltage states (Firestorm, up to 3.2 GHz) */
static const uint32_t t8103_voltage_states_pcluster[][2] = {
    { 600000,  650 },   /* 600 MHz  low-power */
    { 1000000, 700 },   /* 1.0 GHz  efficiency */
    { 1600000, 800 },   /* 1.6 GHz  balanced */
    { 2400000, 900 },   /* 2.4 GHz  performance */
    { 3000000, 1000 },  /* 3.0 GHz  high-perf */
    { 3200000, 1050 },  /* 3.2 GHz  peak */
};

/* E-Cluster voltage states (Icestorm, up to 2.0 GHz) */
static const uint32_t t8103_voltage_states_ecluster[][2] = {
    { 300000,  600 },   /* 300 MHz  idle */
    { 600000,  650 },   /* 600 MHz  low-power */
    { 1000000, 700 },   /* 1.0 GHz  efficiency */
    { 1500000, 750 },   /* 1.5 GHz  balanced */
    { 2000000, 800 },   /* 2.0 GHz  peak */
};

/* GPU voltage states (M1 GPU, up to 8 cores) */
static const uint32_t t8103_voltage_states_gpu[][2] = {
    { 200000,  650 },   /* 200 MHz  idle */
    { 450000,  700 },   /* 450 MHz */
    { 800000,  800 },   /* 800 MHz */
    { 1000000, 850 },   /* 1.0 GHz */
    { 1278000, 900 },   /* ~1.28 GHz peak */
};

/* ── M1 Voltage States SRAM ─────────────────────────────── */

static const uint32_t t8103_voltage_states_pcluster_sram[][2] = {
    { 650,  800 },
    { 800,  950 },
    { 950,  1100 },
};

static const uint32_t t8103_voltage_states_ecluster_sram[][2] = {
    { 650,  800 },
    { 800,  950 },
};

/* ── M1 Bridge Settings ────────────────────────────────────
 *
 * Bridge settings configure the AMCC (Apple Memory Cache Controller)
 * planes and interconnects. M1 has a unified memory architecture.
 * These are per-cluster/per-function bridge register configurations.
 */

/* Bridge 0: P-Cluster CPU bridge */
static const uint32_t t8103_bridge_settings_pcluster[][2] = {
    { 0x0,   0x221F31 },     /* Bridge control */
    { 0x4,   0x3F },         /* Enable mask */
    { 0x8,   0x30430C3 },    /* ?? */
    { 0x100, 0x2820 },       /* QoS low */
    { 0x104, 0x1914 },       /* QoS medium */
    { 0x108, 0x6752 },       /* QoS high */
    { 0x10C, 0x352A },       /* QoS urgent */
    { 0x110, 0x352A },       /* QoS critical */
    { 0x200, 0x1F400101 },   /* BW limit idle */
    { 0x204, 0x3E800101 },   /* BW limit low */
    { 0x208, 0x6400A01 },    /* BW limit medium */
    { 0x20C, 0xC803201 },    /* BW limit high */
    { 0x304, 0x4000FF },     /* Latency cfg */
    { 0x308, 0x6000FF },     /* Latency cfg 2 */
    { 0x30C, 0x4000FF },     /* Latency cfg 3 */
    { 0x310, 0x4000FF },     /* Latency cfg 4 */
    { 0x500, 0xC0017C04 },   /* Power cfg */
    { 0x600, 0x8F07 },       /* Channel 0 */
    { 0x604, 0x8F07 },       /* Channel 1 */
    { 0x608, 0x8F07 },       /* Channel 2 */
    { 0x60C, 0x8F07 },       /* Channel 3 */
    { 0x610, 0x8F07 },       /* Channel 4 */
    { 0x614, 0x8F07 },       /* Channel 5 */
    { 0x700, 0x100FF },      /* TZ0 base */
    { 0x704, 0x100FF },      /* TZ0 end */
    { 0x708, 0x100FF },      /* TZ1 base */
    { 0x70C, 0x100FF },      /* TZ1 end */
    { 0x710, 0x100FF },      /* TZ2 base */
    { 0x714, 0x100FF },      /* TZ2 end */
};

/* Bridge 1: E-Cluster CPU bridge */
static const uint32_t t8103_bridge_settings_ecluster[][2] = {
    { 0x0,  0xF },           /* Bridge control */
    { 0xC,  0xC0050005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x14, 0x100FF },       /* Latency */
    { 0x1C, 0x18 },          /* ?? */
    { 0x20, 0x20 },          /* ?? */
};

/* Bridge 2: GPU bridge */
static const uint32_t t8103_bridge_settings_gpu[][2] = {
    { 0x0,  0xF },           /* Bridge control */
    { 0xC,  0x80050005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x14, 0x100FF },       /* Latency */
    { 0x1C, 0xFF },          /* Channel mask */
    { 0x20, 0xFF },          /* Channel mask */
    { 0x28, 0xFF },          /* Channel mask */
    { 0x2C, 0xFF },          /* Channel mask */
};

/* Bridge 3: Display controller */
static const uint32_t t8103_bridge_settings_display[][2] = {
    { 0x0,  0xF3F },         /* Bridge control */
    { 0xC,  0x80050005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x14, 0x100FF },       /* Latency */
    { 0x1C, 0xFF },          /* Channel mask */
    { 0x20, 0xFF },          /* Channel mask */
    { 0x28, 0xFF },          /* Channel mask */
    { 0x2C, 0xFF },          /* Channel mask */
};

/* Bridge 4: ANS/NVMe storage */
static const uint32_t t8103_bridge_settings_storage[][2] = {
    { 0x0,  0x3 },           /* Bridge control */
    { 0xC,  0x80050005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x14, 0x100FF },       /* Latency */
};

/* Bridge 5: USB / Thunderbolt */
static const uint32_t t8103_bridge_settings_usb[][2] = {
    { 0x0,  0xF },           /* Bridge control */
    { 0xC,  0x80040005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x18, 0x80400020 },    /* ?? */
    { 0x1C, 0xFF },          /* Channel mask */
    { 0x20, 0xFF },          /* Channel mask */
    { 0x28, 0xFF },          /* Channel mask */
    { 0x2C, 0xFF },          /* Channel mask */
};

/* Bridge 6: AOP (Always-On Processor) */
static const uint32_t t8103_bridge_settings_aop[][2] = {
    { 0x0,   0x440331 },     /* Bridge control */
    { 0x100, 0x6450 },       /* QoS */
    { 0x104, 0x6450 },       /* QoS */
    { 0x108, 0x2820 },       /* QoS */
    { 0x200, 0x1F400100 },   /* BW limit */
    { 0x204, 0x3E800100 },   /* BW limit */
    { 0x208, 0x6400A00 },    /* BW limit */
    { 0x20C, 0xC803200 },    /* BW limit */
    { 0x304, 0x300020 },     /* Latency */
    { 0x308, 0xFF },         /* Latency */
    { 0x500, 0xC0010C04 },   /* Power */
    { 0x600, 0x8F07 },       /* Channel */
    { 0x604, 0x8F07 },       /* Channel */
    { 0x608, 0x8F07 },       /* Channel */
};

/* Bridge 7: SIO (DMA controller) */
static const uint32_t t8103_bridge_settings_sio[][2] = {
    { 0x0,    0xF },             /* Bridge control */
    { 0xC,    0x80050005 },      /* Enable */
    { 0x10,   0x8F04 },          /* QoS config */
    { 0x1C,   0x40 },            /* ?? */
    { 0x20,   0x40 },            /* ?? */
    { 0x4000, 0xC00C000C },      /* DMA cfg */
    { 0x4004, 0x0 },             /* DMA cfg */
};

/* Bridge 8: PCIe / Thunderbolt DMA */
static const uint32_t t8103_bridge_settings_pcie[][2] = {
    { 0x0,  0xF },           /* Bridge control */
    { 0xC,  0x80050005 },    /* Enable */
    { 0x10, 0x8F04 },        /* QoS config */
    { 0x14, 0x100FF },       /* Latency */
};

/* Bridge 9-11: Reserved / future expansion */
static const uint32_t t8103_bridge_settings_nvme[][2] = {
    { 0x0,  0xF },              /* Bridge control */
    { 0xC,  0x80050005 },       /* Enable */
    { 0x10, 0x8F04 },           /* QoS config */
    { 0x14, 0x100FF },          /* Latency */
    { 0x1C, 0xFF },             /* Channel mask */
    { 0x20, 0xFF },             /* Channel mask */
};

/* ── M1 Clock Frequencies ─────────────────────────────────
 *
 * M1 clock tree has 46 entries per the DeviceTree.
 * Format: Hz values for each clock domain.
 * Key clocks: CPU 3.2GHz, GPU 1.28GHz, ANE, ISP, etc.
 */

static const uint32_t t8103_clock_frequencies[] = {
    /* 0-5: System/base clocks */
    24000000,      /* CLK_REF      24 MHz base clock */
    32768,         /* CLK_RTC      32.768 kHz RTC */
    47972352,      /* CLK_??       ~48 MHz */
    287834112,     /* CLK_??       ~288 MHz */
    2064000000,    /* FIXME: P-cluster max was 3.2GHz */
    600000000,     /* CLK_AOP      600 MHz */
    /* 6-11: Debug/aux clocks */
    24000000,      /* CLK_DEBUG    debug clock */
    1800000000,    /* FIXME: was E-cluster max 2.0GHz */
    0,             /* Reserved */
    1200000000,    /* CLK_GPU_IDLE */
    0,             /* Reserved */
    105000000,     /* CLK_??       105 MHz */
    /* 12-17: Peripheral clocks */
    24000000,      /* CLK_UART */
    24000000,      /* CLK_I2C */
    24000000,      /* CLK_SPI */
    96000000,      /* CLK_??       96 MHz */
    287834112,     /* CLK_NAND */
    0,             /* Reserved */
    /* 18-23: Display/sensor clocks */
    23986176,      /* CLK_DISPLAY  ~24 MHz */
    143917056,     /* CLK_DISP_AUX ~144 MHz */
    0,             /* Reserved */
    23986176,      /* CLK_?? */
    23986176,      /* CLK_?? */
    23986176,      /* CLK_?? */
    /* 24-29 */
    23986176,      /* CLK_?? */
    23986176,      /* CLK_?? */
    23986176,      /* CLK_?? */
    287834112,     /* CLK_?? */
    287834112,     /* CLK_?? */
    13706384,      /* CLK_??       ~13.7 MHz */
    /* 30-35 */
    23986176,      /* CLK_?? */
    287834112,     /* CLK_?? */
    47972352,      /* CLK_?? */
    0, 0, 0,
    /* 36-41 */
    0, 0, 0,
    0, 0, 0,
    /* 42-47 */
    0, 0, 0,
    0, 0, 0,
    /* 48-53 */
    0, 0, 0,
    0, 0, 0,
    /* 54-59: Memory controller clocks */
    600000000,     /* CLK_MCC0     600 MHz */
    900000000,     /* CLK_MCC1     900 MHz */
    300000000,     /* CLK_MCC2     300 MHz */
    600000000,     /* CLK_MCC3     600 MHz */
    900000000,     /* CLK_MCC4     900 MHz */
    150000000,     /* CLK_MCC5     150 MHz */
    /* 60-65 */
    0,
    100000000,     /* CLK_??       100 MHz */
    133333328,     /* CLK_??       ~133 MHz */
    0,
    360000000,     /* CLK_??       360 MHz */
    600000000,     /* CLK_??       600 MHz */
    /* 66-71 */
    900000000,     /* CLK_??       900 MHz */
    900000000,     /* CLK_??       900 MHz */
    24000000,      /* CLK_?? */
    100000000,     /* CLK_?? */
    120000000,     /* CLK_?? */
    200000000,     /* CLK_?? */
    /* 72-77 */
    450000000,     /* CLK_??       450 MHz */
    360000000,     /* CLK_?? */
    24000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    450000000,     /* CLK_?? */
    /* 78-83 */
    800000000,     /* CLK_?? */
    320000000,     /* CLK_?? */
    427200000,     /* CLK_?? */
    0,
    266666656,     /* CLK_?? */
    533333328,     /* CLK_?? */
    /* 84-89 */
    0, 0, 0,
    400000000,     /* CLK_?? */
    360000000,     /* CLK_?? */
    1800000000,    /* CLK_?? */
    /* 90-95 */
    63157888,      /* CLK_??       ~63 MHz */
    180000000,     /* CLK_?? */
    300000000,     /* CLK_?? */
    360000000,     /* CLK_?? */
    400000000,     /* CLK_?? */
    600000000,     /* CLK_?? */
    /* 96-101 */
    300000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    /* 102-107 */
    900000000,     /* CLK_?? */
    600000000,     /* CLK_?? */
    320000000,     /* CLK_?? */
    50000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    /* 108-113 */
    24000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    360000000,     /* CLK_?? */
    712000000,     /* CLK_?? */
    712000000,     /* CLK_?? */
    257142848,     /* CLK_?? */
    /* 114-119 */
    180000000,     /* CLK_?? */
    533333328,     /* CLK_?? */
    300000000,     /* CLK_?? */
    189473680,     /* CLK_?? */
    900000000,     /* CLK_?? */
    900000000,     /* CLK_?? */
    /* 120-125 */
    24000000,      /* CLK_?? */
    600000000,     /* CLK_?? */
    800000000,     /* CLK_?? */
    0,
    800000000,     /* CLK_?? */
    800000000,     /* CLK_?? */
    /* 126-131 */
    0, 0, 0,
    400000000,     /* CLK_?? */
    360000000,     /* CLK_?? */
    100000000,     /* CLK_?? */
    /* 132-137 */
    24000000,      /* CLK_?? */
    24000000,      /* CLK_?? */
    0, 0, 0, 0,
};

/* ── M1 NCLK Frequencies ──────────────────────────────────
 * Non-coherent clock domain indicators.
 * 0 = coherent, 1 = semi-coherent, 2 = non-coherent
 */

static const uint32_t t8103_clock_frequencies_nclk[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

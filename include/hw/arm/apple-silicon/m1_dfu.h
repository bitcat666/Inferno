#ifndef HW_ARM_APPLE_SILICON_M1_DFU_H
#define HW_ARM_APPLE_SILICON_M1_DFU_H

#include "hw/sysbus.h"

#define TYPE_M1_DFU "m1-dfu"
OBJECT_DECLARE_SIMPLE_TYPE(M1DFUState, M1_DFU)

typedef struct M1DFUState M1DFUState;

M1DFUState *m1_dfu_get(MachineState *machine);
bool m1_dfu_activation_bypass_active(M1DFUState *s);
bool m1_dfu_is_in_recovery(M1DFUState *s);
bool m1_dfu_handle_control_transfer(M1DFUState *s,
                                     uint8_t request_type,
                                     uint8_t request,
                                     uint16_t value,
                                     uint16_t index,
                                     uint8_t *data,
                                     uint32_t *length);

#endif

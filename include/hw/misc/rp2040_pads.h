/*
 * RP2040 Pads
 *
 * Copyright (c) 2022 Mateusz Stadnik <matgla@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef RP2040_PADS_H
#define RP2040_PADS_H

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_PADS "rp2040.pads"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040PadsState, RP2040_PADS)

typedef struct RP2040GpioState RP2040GpioState;

struct RP2040PadsState {
    SysBusDevice parent_obj;

    MemoryRegion gpio_mmio;
    MemoryRegion qspi_mmio;

    bool pads_qspi_in_reset;
    bool pads_qspi_reset_done;
    bool pads_in_reset;
    bool pads_reset_done;


    RP2040GpioState *gpio;
};

void rp2040_pads_reset(RP2040PadsState *state, bool reset_state);
void rp2040_qspi_pads_reset(RP2040PadsState *state, bool reset_state);
int rp2040_pads_get_reset_state(RP2040PadsState *state);
int rp2040_qspi_pads_get_reset_state(RP2040PadsState *state);
int rp2040_pads_get_reset_done(RP2040PadsState *state);
int rp2040_qspi_pads_get_reset_done(RP2040PadsState *state);


#endif /* RP2040_PADS_H */

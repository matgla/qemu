/*
 * RP2040 SSI
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

#ifndef RP2040_SSI_H
#define RP2040_SSI_H

#include <stdbool.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/ptimer.h"
#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "qemu/fifo32.h"
#include "qom/object.h"

#define TYPE_RP2040_SSI "rp2040.ssi"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040SSIState, RP2040_SSI)

union RP2040SSICtrlr0
{
    uint32_t value;
    struct {
        uint32_t dfs : 4;
        uint32_t frf : 3;
        uint32_t scph : 1;
        uint32_t scpol : 1;
        uint32_t tmod : 2;
        uint32_t slv_oe : 1;
        uint32_t slr : 1;
        uint32_t cfs : 4;
        uint32_t dfs32 : 4;
        uint32_t spi_frf : 2;
        uint32_t _reserved : 1;
        uint32_t sste : 1;
    };
};

union RP2040SSIStatus
{
    uint32_t value;
    struct {
        uint32_t busy:1;
        uint32_t tfnf:1;
        uint32_t tfe:1;
        uint32_t rfne:1;
        uint32_t rff:1;
        uint32_t txe:1;
        uint32_t dcol:1;
    };
};

typedef union RP2040SSICtrlr0 RP2040SSICtrlr0;
typedef union RP2040SSIStatus RP2040SSIStatus;

typedef struct RP2040GpioState RP2040GpioState;

struct RP2040SSIState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    qemu_irq cs;
    bool slave_selected;

    RP2040GpioState *gpio;
    SSIBus *bus;

    Fifo32 tx_fifo;
    Fifo32 rx_fifo;

    RP2040SSICtrlr0 ctrlr0;
    uint32_t ctrlr1;
    RP2040SSIStatus status;
};

#endif /* RP2040_SSI_H */

/*
 * RP2040 SIO
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

#ifndef RP2040_SIO_H
#define RP2040_SIO_H

#include <stdint.h>

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/gpio/rp2040_gpio.h"
#include "hw/sysbus.h"
#include "qom/object.h"
#include "qemu/fifo32.h"

#define TYPE_RP2040_SIO "rp2040.sio"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040SioState, RP2040_SIO);

union RP2040SioFifoSt {
    struct {
        uint32_t vld:1;
        uint32_t rdy:1;
        uint32_t wof:1;
        uint32_t roe:1;
        uint32_t __reserved:28;
    };
    uint32_t _value;
};

typedef union RP2040SioFifoSt RP2040SioFifoSt;

union RP2040SioDivCsr {
    struct {
        uint32_t ready:1;
        uint32_t dirty:1;
        uint32_t _reserved:30;
    };
    uint32_t value;
};

typedef union RP2040SioDivCsr RP2040SioDivCsr;

struct RP2040SioState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    uint32_t cpuid;
    RP2040GpioState *gpio;
    RP2040QspiIOState *qspi;

    RP2040SioFifoSt fifo_st;
    Fifo32 fifo_rx;
    Fifo32 *fifo_tx;

    RP2040SioDivCsr div_csr;
    uint32_t dividend;
    uint32_t divisor;
    uint32_t quotient;
    uint32_t remainder;
};

#endif /* RP2040_SIO_H */
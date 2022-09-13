/*
 * RP2040 Resets
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

#ifndef RP2040_RESETS_H
#define RP2040_RESETS_H

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_RESETS "rp2040.resets"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040ResetsState, RP2040_RESETS)

struct RP2040ResetsState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    Object *adc;
    Object *busctrl;
    Object *dma;
    Object *i2c0;
    Object *i2c1;
    Object *gpio;
    Object *qspi_io;
    bool jtag;
    Object *pads;
    Object *qspi_pads;
    Object *pio0;
    Object *pio1;
    Object *pllsys;
    Object *pllusb;
    Object *pwm;
    Object *rtc;
    Object *spi0;
    Object *spi1;
    Object *syscfg;
    Object *sysinfo;
    Object *tbman;
    Object *timer;
    Object *uart0;
    Object *uart1;
    Object *usbctrl;
};

#endif /* RP2040_RESETS_H */

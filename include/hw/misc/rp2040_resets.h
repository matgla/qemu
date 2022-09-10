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

typedef struct RP2040GpioState RP2040GpioState;
typedef struct RP2040PadsState RP2040PadsState;
typedef struct RP2040TimerState RP2040TimerState;

struct RP2040ResetsDoneState {
    uint8_t adc;
    uint8_t busctrl;
    uint8_t dma;
    uint8_t i2c0;
    uint8_t i2c1;
    uint8_t iobank0;
    uint8_t ioqspi;
    uint8_t jtag;
    uint8_t padsbank0;
    uint8_t padsqspi;
    uint8_t pio0;
    uint8_t pio1;
    uint8_t pllsys;
    uint8_t pllusb;
    uint8_t pwm;
    uint8_t rtc;
    uint8_t spi0;
    uint8_t spi1;
    uint8_t syscfg;
    uint8_t sysinfo;
    uint8_t tbman;
    uint8_t timer;
    uint8_t uart0;
    uint8_t uart1;
    uint8_t usbctrl;
};

typedef struct RP2040ResetsDoneState RP2040ResetsDoneState;

struct RP2040ResetsState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    RP2040GpioState *gpio;
    RP2040PadsState *pads;
    RP2040TimerState *timer;

    RP2040ResetsDoneState done;
};

#endif /* RP2040_RESETS_H */

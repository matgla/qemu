/*
 * RP2040 PLL
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

#ifndef RP2040_PLL_H
#define RP2040_PLL_H

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_PLL "rp2040.pll"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040PLLState, RP2040_PLL)

union RP2040PLLCS {
    struct {
        uint32_t refdiv:6;
        uint32_t _reserved1:2;
        uint32_t bypass:1;
        uint32_t _reserved2:22;
        uint32_t lock:1;
    };
    uint32_t value;
};

typedef union RP2040PLLCS RP2040PLLCS;

struct RP2040PLLState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    RP2040PLLCS cs;
};

#endif /* RP2040_PLL_H */

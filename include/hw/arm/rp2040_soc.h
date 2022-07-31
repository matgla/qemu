/*
 * Raspberry RP2040 SoC
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

#ifndef HW_ARM_RP2040_SOC_H
#define HW_ARM_RP2040_SOC_H

#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "hw/char/pl011.h"
#include "hw/ssi/pl022.h"
#include "hw/clock.h"
#include "qom/object.h"
#include "sysemu/sysemu.h"

#include "hw/misc/rp2040_resets.h"
#include "hw/misc/rp2040_vreg.h"
#include "hw/misc/rp2040_clocks.h"
#include "hw/misc/rp2040_xosc.h"

#define TYPE_RP2040_SOC "rp2040_soc"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040State, RP2040_SOC)

#define RP2040_SOC_ROM_BASE_ADDRESS 0x00000000
#define RP2040_SOC_ROM_SIZE (16 * 1024) // picoboot

#define RP2040_SOC_XIP_BASE_ADDRESS 0x10000000
#define RP2040_SOC_XIP_SIZE (16 * 1024 * 1024)

#define RP2040_SOC_SRAM_BASE_ADDRESS 0x20000000
#define RP2040_SOC_SRAM_SIZE (264 * 1024)

#define RP2040_SOC_NUMBER_OF_CORES 1
#define RP2040_SOC_NUMBER_OF_SPIS 2
#define RP2040_SOC_NUMBER_OF_UARTS 2

struct RP2040State {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    char *cpu_type;

    /* ARMv7 is backward compatible with ARMv6 see (https://developer.arm.com/documentation/ddi0419/c/Application-Level-Architecture/Introduction/About-the-ARM-architecture-profiles?lang=en) */
    ARMv7MState armv6m[RP2040_SOC_NUMBER_OF_CORES];

    PL011State uart[RP2040_SOC_NUMBER_OF_UARTS];
    PL011State spi[RP2040_SOC_NUMBER_OF_SPIS];
    RP2040ClocksState clocks;
    RP2040XOSCState xosc;
    RP2040ResetsState resets;
    RP2040VregState vreg;

    MemoryRegion rom;
    MemoryRegion qspi_flash;
    MemoryRegion sram;

    Clock *sysclk;
    Clock *refclk;
};

#endif

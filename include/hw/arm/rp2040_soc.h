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

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "hw/char/pl011.h"
#include "hw/ssi/pl022.h"
#include "hw/clock.h"
#include "qemu/units.h"
#include "qom/object.h"
#include "sysemu/sysemu.h"

#include "hw/char/rp2040_uart.h"
#include "hw/gpio/rp2040_gpio.h"
#include "hw/gpio/rp2040_sio.h"
#include "hw/misc/rp2040_clocks.h"
#include "hw/misc/rp2040_pads.h"
#include "hw/misc/rp2040_resets.h"
#include "hw/misc/rp2040_xip.h"
#include "hw/misc/rp2040_xosc.h"
#include "hw/misc/rp2040_pll.h"
#include "hw/timer/rp2040_timer.h"
#include "hw/ssi/rp2040_ssi.h"


#define TYPE_RP2040_SOC "rp2040"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040State, RP2040_SOC)

#define RP2040_ROM_SIZE (16 * KiB)

#define RP2040_SOC_NUMBER_OF_CORES 2
#define RP2040_SOC_NUMBER_OF_SPIS 2
#define RP2040_SOC_NUMBER_OF_UARTS 2

struct RP2040State {
    /*< private >*/
    SysBusDevice parent_obj;

    /* ARMv7 is backward compatible with ARMv6 */
    ARMv7MState armv6m[RP2040_SOC_NUMBER_OF_CORES];


    RP2040ResetsState* resets;
    RP2040GpioState gpio;
    RP2040QspiIOState qspi_io;
    RP2040PadsState pads;
    RP2040TimerState timer;
    RP2040SSIState ssi;
    RP2040XipState xip;
    RP2040XOSCState xosc;
    RP2040ClocksState clocks;
    RP2040PLLState pll_sys;
    RP2040PLLState pll_usb;

    RP2040UartState uart0;
    RP2040UartState uart1;

    /* each core has own SIO register */
    RP2040SioState sio[RP2040_SOC_NUMBER_OF_CORES];

    MemoryRegion *system_memory;
    MemoryRegion container;
    MemoryRegion core_container[RP2040_SOC_NUMBER_OF_CORES];
    MemoryRegion core_container_alias[RP2040_SOC_NUMBER_OF_CORES - 1];

    MemoryRegion rom;
    MemoryRegion sram03;
    MemoryRegion sram4;
    MemoryRegion sram5;

    Clock *sysclk;
    Clock *refclk;
};

#endif

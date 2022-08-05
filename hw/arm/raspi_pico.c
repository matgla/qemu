/*
 * Raspberry Pico machine
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

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/loader.h"
#include "hw/irq.h"
#include "hw/arm/armv7m.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "qemu/error-report.h"
#include "hw/boards.h"

#include "hw/arm/rp2040_soc.h"
#include "hw/arm/boot.h"

#define SYSCLK_FREQ 12000000ULL 

static void raspi_pico_init(MachineState *machine)
{
    DeviceState *dev;
    Clock *sysclk;
    DriveInfo *dinfo = drive_get_by_index(IF_MTD, 0);
    qemu_irq cs_line;
    sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(sysclk, 1200000);

    dev = qdev_new(TYPE_RP2040_SOC);
    RP2040State *state = RP2040_SOC(dev);
    RP2040SSIState *ssi = &state->ssi;
    qdev_prop_set_string(dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));
    qdev_connect_clock_in(dev, "sysclk", sysclk);

    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    armv7m_load_kernel(ARM_CPU(first_cpu), "/home/mateusz/bootrom.elf", RP2040_SOC_ROM_SIZE);
    fprintf(stderr, "Loading firmware image: %s\n", machine->kernel_filename);

    
    /* create SPI flash */ 
    dev = qdev_new("w25q80");
    fprintf(stderr, "Created flash device\n");

    if (!dinfo)
    {
        fprintf(stderr, "Drive not set\n");
        return;
    }
    qdev_prop_set_drive(dev, "drive", blk_by_legacy_dinfo(dinfo));
    qdev_realize_and_unref(dev, BUS(ssi->spi), &error_fatal);
    cs_line = qdev_get_gpio_in_named(dev, SSI_GPIO_CS, 0);
    sysbus_connect_irq(SYS_BUS_DEVICE(ssi), 0, cs_line);
    qemu_set_irq(cs_line, 1);
    qdev_connect_gpio_out(DEVICE(&state->gpio_qspi), 1, cs_line);//ssi->cs_line);

    // qdev_realize_and_unref(dev, BUS(s->ssi), &error_fatal);
    

    // image_size = load_image_targphys(machine->kernel_filename, 0x10000000, 16 * 1024 * 1024);
    // if (image_size < 0) {
    //     error_report("Could not load kernel '%s'", machine->kernel_filename);
    //     exit(1);
    // }
}

static void raspi_pico_machine_init(MachineClass *mc)
{
    mc->desc = "Raspberry PICO (dual cortex-m0)";
    mc->init = raspi_pico_init;
}

DEFINE_MACHINE("raspi_pico", raspi_pico_machine_init)

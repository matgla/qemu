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

#define TYPE_RASPI_PICO_MACHINE MACHINE_TYPE_NAME("raspi_pico")
OBJECT_DECLARE_SIMPLE_TYPE(RaspiPicoMachineState, RASPI_PICO_MACHINE);

struct RaspiPicoMachineState {
    MachineState parent;
    RP2040State soc; 
    /* someday here I want to put externals like QSPI flash */
};

// static void raspi_pico_load_qspi_flash(MachineState *machine)
// {
//     DriveInfo *drive_info = drive_get_by_index(IF_MTD, 0); /* get flash image from command line, first drive with type MTD */ 

// }

static void raspi_pico_initialize_clock(MachineState *machine, DeviceState *soc)
{
    Clock *sysclk = clock_new(OBJECT(machine), "sysclk"); /* TODO: create clock tree with XOSC and working PLL in simulator */

    clock_set_hz(sysclk, 1200000);

    qdev_connect_clock_in(soc, "sysclk", sysclk);
}

static void raspi_pico_instance_init(MachineState *machine)
{
    RaspiPicoMachineState *machine_state = RASPI_PICO_MACHINE(machine);
    RP2040State *soc = &machine_state->soc;
    object_initialize_child(OBJECT(machine), "rp2040", soc, TYPE_RP2040_SOC);
    raspi_pico_initialize_clock(machine, DEVICE(soc));

    /* Trigger SOC realization procedure */
    sysbus_realize_and_unref(SYS_BUS_DEVICE(DEVICE(soc)), &error_fatal);

    // raspi_pico_initialize_clock(machine, soc);

    // qemu_irq cs_line;

    // dev = qdev_new(TYPE_RP2040_SOC);
    // RP2040State *state = RP2040_SOC(dev);
    // RP2040SSIState *ssi = &state->ssi;
    // qdev_prop_set_string(dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));

    // sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    // armv7m_load_kernel(ARM_CPU(first_cpu), "/home/mateusz/bootrom.elf", RP2040_SOC_ROM_SIZE);
    // fprintf(stderr, "Loading firmware image: %s\n", machine->kernel_filename);

    
    // /* create SPI flash */ 
    // dev = qdev_new("w25q80");
    // fprintf(stderr, "Created flash device\n");

    // if (!dinfo)
    // {
    //     fprintf(stderr, "Drive not set\n");
    //     return;
    // }
    // qdev_prop_set_drive(dev, "drive", blk_by_legacy_dinfo(dinfo));
    // qdev_realize_and_unref(dev, BUS(ssi->spi), &error_fatal);
    // cs_line = qdev_get_gpio_in_named(dev, SSI_GPIO_CS, 0);
    // sysbus_connect_irq(SYS_BUS_DEVICE(ssi), 0, cs_line);
    // qemu_set_irq(cs_line, 1);
    // qdev_connect_gpio_out_named(DEVICE(&state->gpio), "qspi-cs", 0, cs_line);

    // qdev_realize_and_unref(dev, BUS(s->ssi), &error_fatal);
    

    // image_size = load_image_targphys(machine->kernel_filename, 0x10000000, 16 * 1024 * 1024);
    // if (image_size < 0) {
    //     error_report("Could not load kernel '%s'", machine->kernel_filename);
    //     exit(1);
    // }
}

static void raspi_pico_class_init(ObjectClass *klass, void *data)
{
    MachineClass *mc = MACHINE_CLASS(klass);

    mc->desc = "Raspberry PICO (dual cortex-m0)";
    mc->init = raspi_pico_instance_init;
    mc->max_cpus = 2;
}


static const TypeInfo raspi_pico_info = {
    .name = TYPE_RASPI_PICO_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(RaspiPicoMachineState),
    .class_init = raspi_pico_class_init,
};

static void raspi_pico_machine_init(void)
{
    type_register_static(&raspi_pico_info);
}

type_init(raspi_pico_machine_init);
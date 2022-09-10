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
#include "hw/boards.h"
#include "hw/clock.h"
#include "hw/irq.h"
#include "hw/loader.h"
#include "hw/qdev-clock.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "qemu/datadir.h"
#include "qemu/units.h"
#include "qom/object.h"

#include "hw/arm/rp2040_soc.h"


#define SYSCLK_FREQ 12000000ULL


struct RaspiPicoMachineState {
    /* Private */
    MachineState parent;

    /* Public */
    RP2040State soc;
};
typedef struct RaspiPicoMachineState RaspiPicoMachineState;

struct RaspiPicoMachineClass {
    /* Private */
    MachineClass parent_obj;
};
typedef struct RaspiPicoMachineClass RaspiPicoMachineClass;

#define TYPE_RASPI_PICO_MACHINE MACHINE_TYPE_NAME("raspi_pico")
DECLARE_OBJ_CHECKERS(RaspiPicoMachineState, RaspiPicoMachineClass,
    RASPI_PICO_MACHINE, TYPE_RASPI_PICO_MACHINE);

static void raspi_pico_initialize_clock(MachineState *machine, DeviceState *soc)
{
    Clock *sysclk = clock_new(OBJECT(machine), "sysclk");

    clock_set_hz(sysclk, SYSCLK_FREQ);

    qdev_connect_clock_in(soc, "sysclk", sysclk);
}

static void raspi_pico_instance_init(MachineState *machine)
{
    RaspiPicoMachineState *machine_state = RASPI_PICO_MACHINE(machine);
    RP2040State *soc = &machine_state->soc;
    Error **errp = &error_fatal;
    qemu_irq cs_line;
    MemoryRegion *system_memory = get_system_memory();
    DeviceState *dev = qdev_new("w25q80");
    DriveInfo *dinfo = drive_get_by_index(IF_MTD, 0);
    // RP2040SSIState *ssi = &state->ssi;

    if (!dinfo)
    {
        fprintf(stderr, "Drive not set\n");
        return;
    }

    object_initialize_child(OBJECT(machine), "rp2040", soc, TYPE_RP2040_SOC);
    object_property_set_link(OBJECT(soc), "memory", OBJECT(system_memory),
        errp);
    raspi_pico_initialize_clock(machine, DEVICE(soc));

    /* Trigger SOC realization procedure */
    sysbus_realize_and_unref(SYS_BUS_DEVICE(DEVICE(soc)), errp);
    qdev_prop_set_drive(dev, "drive", blk_by_legacy_dinfo(dinfo));
    qdev_realize_and_unref(dev, BUS(soc->ssi.bus), &error_fatal);

    cs_line = qdev_get_gpio_in_named(dev, SSI_GPIO_CS, 0);
    qdev_connect_gpio_out_named(DEVICE(&soc->gpio), "qspi-cs", 0, cs_line);
    qemu_set_irq(soc->gpio.qspi_out[1], 0);
    soc->xip.blk = blk_by_legacy_dinfo(dinfo);
}

static void raspi_pico_class_init(ObjectClass *klass, void *data)
{
    MachineClass *mc = MACHINE_CLASS(klass);

    mc->desc = "Raspberry Pi Pico (dual Cortex-M0+)";
    mc->init = raspi_pico_instance_init;
    mc->min_cpus = 2;
    mc->default_cpus = 2;
    mc->max_cpus = 2;
    mc->no_parallel = 1;
    mc->no_cdrom = 1;
    mc->no_floppy = 1;
    mc->no_sdcard = 1;
    mc->no_serial = 1;
}


static const TypeInfo raspi_pico_info = {
    .name = TYPE_RASPI_PICO_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(RaspiPicoMachineState),
    .class_init = raspi_pico_class_init,
    .class_size = sizeof(RaspiPicoMachineClass),
};

static void raspi_pico_machine_init(void)
{
    type_register_static(&raspi_pico_info);
}

type_init(raspi_pico_machine_init);

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
    /* TODO: place QSPI flash model here, so it will be easy to modify board */
};

static void raspi_pico_initialize_clock(MachineState *machine, DeviceState *soc)
{
    Clock *sysclk = clock_new(OBJECT(machine), "sysclk"); /* TODO: create clock tree with XOSC and working PLL in simulator */

    clock_set_hz(sysclk, SYSCLK_FREQ);

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

    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename, RP2040_SOC_ROM_SIZE);
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
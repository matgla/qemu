/*
 * RP2040 Pads
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

#include "hw/misc/rp2040_pads.h"

#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "hw/gpio/rp2040_gpio.h"


#define RP2040_PADS_REGISTER_SIZE 0x4000

static void rp2040_pads_instance_init(Object *obj)
{
    RP2040PadsState *state = RP2040_PADS(obj);
    rp2040_pads_reset(state, true);
    rp2040_qspi_pads_reset(state, true);
}

static uint64_t rp2040_pads_read(void *opaque, hwaddr offset, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);

    return 0;
}


static void rp2040_pads_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static uint64_t rp2040_qspi_pads_read(void *opaque, hwaddr offset,
                                      unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);

    return 0;
}

static void rp2040_qspi_pads_write(void *opaque, hwaddr offset,
                                   uint64_t value, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static const MemoryRegionOps rp2040_pads_io = {
    .read = rp2040_pads_read,
    .write = rp2040_pads_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const MemoryRegionOps rp2040_qspi_pads_io = {
    .read = rp2040_qspi_pads_read,
    .write = rp2040_qspi_pads_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_pads_realize(DeviceState *dev, Error **errp)
{
    RP2040PadsState *state = RP2040_PADS(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->gpio_mmio);
    memory_region_init_io(&state->gpio_mmio, OBJECT(dev), &rp2040_pads_io, state,
        "gpio_mmio", RP2040_PADS_REGISTER_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->qspi_mmio);
    memory_region_init_io(&state->qspi_mmio, OBJECT(dev), &rp2040_qspi_pads_io, state,
        "qspi_mmio", RP2040_PADS_REGISTER_SIZE);
}

static void rp2040_pads_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 Pads";
    dc->realize = rp2040_pads_realize;
}

static const TypeInfo rp2040_pads_type_info = {
    .name           = TYPE_RP2040_PADS,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_pads_class_init,
    .instance_size  = sizeof(RP2040PadsState),
    .instance_init  = rp2040_pads_instance_init,
};

static void rp2040_pads_register_types(void)
{
    type_register_static(&rp2040_pads_type_info);
}

type_init(rp2040_pads_register_types);

void rp2040_pads_reset(RP2040PadsState *state, bool reset_state)
{
    state->pads_reset_done = !reset_state;
    state->pads_in_reset = reset_state;
}

void rp2040_qspi_pads_reset(RP2040PadsState *state, bool reset_state)
{
    state->pads_qspi_reset_done = !reset_state;
    state->pads_qspi_in_reset = reset_state;
}

int rp2040_pads_get_reset_state(RP2040PadsState *state)
{
    return state->pads_in_reset;
}

int rp2040_qspi_pads_get_reset_state(RP2040PadsState *state)
{
    return state->pads_qspi_in_reset;
}

int rp2040_pads_get_reset_done(RP2040PadsState *state)
{
    return state->pads_reset_done;
}

int rp2040_qspi_pads_get_reset_done(RP2040PadsState *state)
{
    return state->pads_qspi_reset_done;
}


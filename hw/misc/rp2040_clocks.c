/*
 * RP2040 CLOCKS
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

#include "hw/misc/rp2040_clocks.h"

#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "qemu/units.h"
#include "sysemu/block-backend.h"

#define RP2040_CLOCKS_SIZE 0x4000

#define RP2040_CLOCKS_CLK_REF_CTRL     0x30
#define RP2040_CLOCKS_CLK_REF_SELECTED 0x38
#define RP2040_CLOCKS_CLK_SYS_CTRL     0x3c
#define RP2040_CLOCKS_CLK_SYS_SELECTED 0x44

static void rp2040_clocks_instance_init(Object *obj)
{
}

static uint64_t rp2040_clocks_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040ClocksState *state = RP2040_CLOCKS(opaque);

    switch (offset) {
        case RP2040_CLOCKS_CLK_REF_CTRL:
            return state->refctrl.value;
        case RP2040_CLOCKS_CLK_REF_SELECTED:
            return state->selected_clk_ref;
        case RP2040_CLOCKS_CLK_SYS_CTRL:
            return state->sysctrl.value;
        case RP2040_CLOCKS_CLK_SYS_SELECTED:
            return state->selected_clk_sys;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
    return 0;
}


static void rp2040_clocks_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned int size)
{
    RP2040ClocksState *state = RP2040_CLOCKS(opaque);
    RP2040AccessType access = rp2040_get_access_type(offset);
    fprintf(stderr, "Write to clocks: %lx -> %lx\n", offset, value);
    offset &= 0x0fff;
    switch (offset) {
        case RP2040_CLOCKS_CLK_REF_CTRL:
            fprintf(stderr, "Value was %d, %d\n", state->refctrl.src, state->refctrl.auxsrc);

            rp2040_write_to_register(access, &state->refctrl.value, value);
            fprintf(stderr, "Value is %d, %d\n", state->refctrl.src, state->refctrl.auxsrc);
            state->selected_clk_ref = 1 << state->refctrl.src;
            return;
        case RP2040_CLOCKS_CLK_SYS_CTRL:
            rp2040_write_to_register(access, &state->sysctrl.value, value);
            state->selected_clk_sys = 1 << state->sysctrl.src;

            return;

    }


    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static const MemoryRegionOps rp2040_clocks_io = {
    .read = rp2040_clocks_read,
    .write = rp2040_clocks_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_clocks_realize(DeviceState *dev, Error **errp)
{
    RP2040ClocksState *state = RP2040_CLOCKS(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_clocks_io, state,
        "clocks_mmio", RP2040_CLOCKS_SIZE);
    state->refctrl.src = 0;
    state->sysctrl.src = 0;
    state->selected_clk_ref = 1;
    state->selected_clk_sys = 1;
}

static void rp2040_clocks_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 CLOCKS";
    dc->realize = rp2040_clocks_realize;
}

static const TypeInfo rp2040_clocks_type_info = {
    .name           = TYPE_RP2040_CLOCKS,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_clocks_class_init,
    .instance_size  = sizeof(RP2040ClocksState),
    .instance_init  = rp2040_clocks_instance_init,
};

static void rp2040_clocks_register_types(void)
{
    type_register_static(&rp2040_clocks_type_info);
}

type_init(rp2040_clocks_register_types);



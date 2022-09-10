/*
 * RP2040 XOSC
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

#include "hw/misc/rp2040_xosc.h"

#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "qemu/units.h"
#include "sysemu/block-backend.h"

#define RP2040_XOSC_SIZE 0x4000

#define RP2040_XOSC_STATUS 0x4

static void rp2040_xosc_instance_init(Object *obj)
{
}

union RP2040XOSCStatus {
    struct {
        uint32_t freq_range:2;
        uint32_t _reserved_1:10;
        uint32_t enabled:1;
        uint32_t _reserved_2:11;
        uint32_t badwrite:1;
        uint32_t _reserved_3:6;
        uint32_t stable:1;
    };
    uint32_t value;
};

typedef union RP2040XOSCStatus RP2040XOSCStatus;

static uint64_t rp2040_xosc_read(void *opaque, hwaddr offset, unsigned int size)
{
    switch (offset) {
        case RP2040_XOSC_STATUS:
            RP2040XOSCStatus status = {
                .freq_range = 0,
                .enabled = 1,
                .badwrite = 0,
                .stable = 1
            };
        return status.value;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
    return 0;
}


static void rp2040_xosc_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned int size)
{
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static const MemoryRegionOps rp2040_xosc_io = {
    .read = rp2040_xosc_read,
    .write = rp2040_xosc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_xosc_realize(DeviceState *dev, Error **errp)
{
    RP2040XOSCState *state = RP2040_XOSC(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_xosc_io, state,
        "xosc_mmio", RP2040_XOSC_SIZE);
}

static void rp2040_xosc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 XOSC";
    dc->realize = rp2040_xosc_realize;
}

static const TypeInfo rp2040_xosc_type_info = {
    .name           = TYPE_RP2040_XOSC,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_xosc_class_init,
    .instance_size  = sizeof(RP2040XOSCState),
    .instance_init  = rp2040_xosc_instance_init,
};

static void rp2040_xosc_register_types(void)
{
    type_register_static(&rp2040_xosc_type_info);
}

type_init(rp2040_xosc_register_types);



/*
 * RP2040 XIP
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

#include "hw/misc/rp2040_xip.h"

#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "qemu/units.h"
#include "sysemu/block-backend.h"


#define RP2040_XIP_SIZE (16 * MiB)

static void rp2040_xip_instance_init(Object *obj)
{
}

static uint64_t rp2040_xip_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040XipState *state = RP2040_XIP(opaque);
    uint32_t data;
    blk_pread(state->blk, offset, size, &data, 0);
    return data;
}


static void rp2040_xip_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned int size)
{
    qemu_log_mask(LOG_GUEST_ERROR, "%s: XIP is read only", __func__);
}

static const MemoryRegionOps rp2040_xip_io = {
    .read = rp2040_xip_read,
    .write = rp2040_xip_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void rp2040_xip_realize(DeviceState *dev, Error **errp)
{
    RP2040XipState *state = RP2040_XIP(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_xip_io, state,
        "xip_mmio", RP2040_XIP_SIZE);
}

static void rp2040_xip_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 XIP";
    dc->realize = rp2040_xip_realize;
}

static const TypeInfo rp2040_xip_type_info = {
    .name           = TYPE_RP2040_XIP,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_xip_class_init,
    .instance_size  = sizeof(RP2040XipState),
    .instance_init  = rp2040_xip_instance_init,
};

static void rp2040_xip_register_types(void)
{
    type_register_static(&rp2040_xip_type_info);
}

type_init(rp2040_xip_register_types);



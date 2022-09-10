/*
 * RP2040 UART
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

#include "hw/char/rp2040_uart.h"

#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "qemu/units.h"
#include "sysemu/block-backend.h"

static void rp2040_uart_instance_init(Object *obj)
{
    RP2040UartState *state = RP2040_UART(obj);
    object_initialize_child(OBJECT(obj), "reg", &state->pl011, TYPE_PL011);
}

static uint64_t rp2040_uart_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040UartState *state = RP2040_UART(opaque);

    offset &= 0x0fff;
    
    return state->pl011_mmio->ops->read(&state->pl011, offset, size);
}


static void rp2040_uart_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned int size)
{
    RP2040UartState *state = RP2040_UART(opaque);
    RP2040AccessType access = rp2040_get_access_type(offset);
    
    offset &= 0x0fff;
    if (access != RP2040_NORMAL_ACCESS) {
        uint32_t data = rp2040_uart_read(opaque, offset, size);
        rp2040_write_to_register(access, &data, value);
        state->pl011_mmio->ops->write(&state->pl011, offset, data, size);
        return;
    }
    state->pl011_mmio->ops->write(&state->pl011, offset, value, size);
}

static const MemoryRegionOps rp2040_uart_io = {
    .read = rp2040_uart_read,
    .write = rp2040_uart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_uart_realize(DeviceState *dev, Error **errp)
{
    RP2040UartState *state = RP2040_UART(dev);
    
    sysbus_realize(SYS_BUS_DEVICE(&state->pl011), errp);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_uart_io, 
        state, "mmio", 0x4000);

    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);

    state->pl011_mmio = sysbus_mmio_get_region(SYS_BUS_DEVICE(&state->pl011), 0);
}

static void rp2040_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 UART";
    dc->realize = rp2040_uart_realize;
}

static const TypeInfo rp2040_uart_type_info = {
    .name           = TYPE_RP2040_UART,
    .parent         = TYPE_PL011,
    .class_init     = rp2040_uart_class_init,
    .instance_size  = sizeof(RP2040UartState),
    .instance_init  = rp2040_uart_instance_init,
};

static void rp2040_uart_register_types(void)
{
    type_register_static(&rp2040_uart_type_info);
}

type_init(rp2040_uart_register_types);



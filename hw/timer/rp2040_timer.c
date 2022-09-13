/*
 * RP2040 Timer
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

#include "hw/timer/rp2040_timer.h"

#include "hw/irq.h"
#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"
#include "qemu/timer.h"

#define RP2040_TIMER_REGISTER_SIZE         0x4000

#define RP2040_TIMER_TIMERAWH  0x24
#define RP2040_TIMER_TIMERAWL  0x28
#define RP2040_TIMER_ALARM0    0x10
#define RP2040_TIMER_ALARM3    0x1c
#define RP2040_TIMER_ARMED     0x20

static void rp2040_timer_fire(void *opaque)
{
    fprintf(stderr, "Timer fired\n");
}

static void rp2040_timer_instance_init(Object *obj)
{
    RP2040TimerState *state = RP2040_TIMER(obj);

    for (int i = 0; i < 4; ++i) {
        state->timers[i] = timer_new_ms(QEMU_CLOCK_VIRTUAL, rp2040_timer_fire,
                                        obj);
        sysbus_init_irq(SYS_BUS_DEVICE(obj), &state->irqs[i]);
    }
}

static uint64_t rp2040_timer_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040TimerState *state = RP2040_TIMER(opaque);

    switch (offset) {
        case RP2040_TIMER_TIMERAWH:
            return (qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) >> 32);
        case RP2040_TIMER_TIMERAWL:
            return (qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) & 0xffffffff);
        case RP2040_TIMER_ARMED:
            return state->timer_armed;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);

    return 0;
}


static void rp2040_timer_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned int size)
{
    RP2040TimerState *state = RP2040_TIMER(opaque);
    // RP2040AccessType access = rp2040_get_access_type(offset);
    // offset = offset & 0x0fff;

    if (offset >= RP2040_TIMER_ALARM0 && offset <= RP2040_TIMER_ALARM3) {
        int64_t    now        = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
        offset -= 0x10;
        offset /= 4;
        fprintf(stderr, "Set timer from: %ld, to %ld\n", now, now + value);
        // timer_mod(state->timers[offset], now + value);
        state->timer_armed |= 1 << offset;
        return;
    }


    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static const MemoryRegionOps rp2040_timer_io = {
    .read = rp2040_timer_read,
    .write = rp2040_timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_timer_realize(DeviceState *dev, Error **errp)
{
    RP2040TimerState *state = RP2040_TIMER(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_timer_io, state,
        "mmio", RP2040_TIMER_REGISTER_SIZE);
}

static void rp2040_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 Timer";
    dc->realize = rp2040_timer_realize;
}

static const TypeInfo rp2040_timer_type_info = {
    .name           = TYPE_RP2040_TIMER,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_timer_class_init,
    .instance_size  = sizeof(RP2040TimerState),
    .instance_init  = rp2040_timer_instance_init,
};

static void rp2040_timer_register_types(void)
{
    type_register_static(&rp2040_timer_type_info);
}

type_init(rp2040_timer_register_types);

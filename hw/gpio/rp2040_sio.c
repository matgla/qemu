/*
 * RP2040 SIO
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

/* Reference: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf */

#include "hw/gpio/rp2040_sio.h"

#include <stdint.h>

#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qom/object.h"

#include "hw/gpio/rp2040_gpio.h"

#define RP2040_SIO_CPUID 0x000
#define RP2040_SIO_GPIO_IN 0x004
#define RP2040_SIO_GPIO_HI_IN 0x008
#define RP2040_SIO_GPIO_OUT 0x010
#define RP2040_SIO_GPIO_OUT_SET 0x014
#define RP2040_SIO_GPIO_OUT_CLR 0x018
#define RP2040_SIO_GPIO_OUT_XOR 0x01c
#define RP2040_SIO_GPIO_OE 0x020
#define RP2040_SIO_GPIO_OE_SET 0x024
#define RP2040_SIO_GPIO_OE_CRL 0x028
#define RP2040_SIO_GPIO_OE_XOR 0x02c
#define RP2040_SIO_GPIO_HI_OUT 0x030
#define RP2040_SIO_GPIO_HI_OUT_SET 0x034
#define RP2040_SIO_GPIO_HI_OUT_CLR 0x038
#define RP2040_SIO_GPIO_HI_OUT_XOR 0x03c
#define RP2040_SIO_GPIO_HI_OE 0x040
#define RP2040_SIO_GPIO_HI_OE_SET 0x044
#define RP2040_SIO_GPIO_HI_OE_CLR 0x048
#define RP2040_SIO_GPIO_HI_OE_XOR 0x04c
#define RP2040_SIO_FIFO_ST 0x050
#define RP2040_SIO_FIFO_WR 0x054
#define RP2040_SIO_FIFO_RD 0x058
#define RP2040_SIO_SPINLOCK_ST 0x05c
#define RP2040_SIO_DIV_UDIVIDEND 0x060
#define RP2040_SIO_DIV_UDIVISOR 0x064
#define RP2040_SIO_DIV_SDIVIDEND 0x068
#define RP2040_SIO_DIV_SDIVISOR 0x06c
#define RP2040_SIO_DIV_QUOTIENT 0x070
#define RP2040_SIO_DIV_REMAINDER 0x074
#define RP2040_SIO_DIV_CSR 0x078
#define RP2040_SIO_INTERP0_ACCUM0 0x080
#define RP2040_SIO_INTERP0_ACCUM1 0x084
#define RP2040_SIO_INTERP0_BASE0 0x088
#define RP2040_SIO_INTERP0_BASE1 0x08c
#define RP2040_SIO_INTERP0_BASE2 0x090
#define RP2040_SIO_INTERP0_POP_LANE0 0x094
#define RP2040_SIO_INTERP0_POP_LANE1 0x098
#define RP2040_SIO_INTERP0_POP_FULL 0x09c
#define RP2040_SIO_INTERP0_PEEK_LANE0 0x0a0
#define RP2040_SIO_INTERP0_PEEK_LANE1 0x0a4
#define RP2040_SIO_INTERP0_PEEK_FULL 0x0a8
#define RP2040_SIO_INTERP0_CTRL_LANE0 0x0ac
#define RP2040_SIO_INTERP0_CTRL_LANE1 0x0b0
#define RP2040_SIO_INTERP0_ACCUM0_ADD 0x0b4
#define RP2040_SIO_INTERP0_ACCUM1_ADD 0x0b8
#define RP2040_SIO_INTERP0_BASE_1AND0 0x0bc
#define RP2040_SIO_INTERP1_ACCUM0 0x0c0
#define RP2040_SIO_INTERP1_ACCUM1 0x0c4
#define RP2040_SIO_INTERP1_BASE0 0x0c8
#define RP2040_SIO_INTERP1_BASE1 0x0cc
#define RP2040_SIO_INTERP1_BASE2 0x0d0
#define RP2040_SIO_INTERP1_POP_LANE0 0x0d4
#define RP2040_SIO_INTERP1_POP_LANE1 0x0d8
#define RP2040_SIO_INTERP1_POP_FULL 0x0dc
#define RP2040_SIO_INTERP1_PEEK_LANE0 0x0e0
#define RP2040_SIO_INTERP1__PEEK_LANE1 0x0e4
#define RP2040_SIO_INTERP1_PEEK_FULL 0x0e8
#define RP2040_SIO_INTERP1_CTRL_LANE0 0x0ec
#define RP2040_SIO_INTERP1_CTRL_LANE1 0x0f0
#define RP2040_SIO_INTERP1_ACCUM0_ADD 0x0f4
#define RP2040_SIO_INTERP1_ACCUM1_ADD 0x0f8
#define RP2040_SIO_INTERP1_BASE_1AND0 0x0fc
#define RP2040_SIO_SPINLOCK0 0x100
#define RP2040_SIO_SPINLOCK31 0x17c

#define RP2040_SIO_INTERCORE_FIFO_SIZE 8

static void rp2040_sio_instance_init(Object *obj)
{

}

static uint64_t rp2040_sio_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040SioState *state = RP2040_SIO(opaque);
    switch (offset) {
        case RP2040_SIO_CPUID:
            return state->cpuid;
        case RP2040_SIO_GPIO_HI_IN:
            /* TODO: it's not actually true, should be checked IN&OUT */
            const uint32_t qspi_state =
                 state->gpio->qspi_status[0].infrompad << 0 |
                 state->gpio->qspi_status[1].infrompad << 1 |
                 state->gpio->qspi_status[2].infrompad << 2 |
                 state->gpio->qspi_status[3].infrompad << 3 |
                 state->gpio->qspi_status[4].infrompad << 4 |
                 state->gpio->qspi_status[5].infrompad << 5;
            return qspi_state;
        case RP2040_SIO_FIFO_ST:
            state->fifo_st.vld = !fifo32_is_empty(&state->fifo_rx);
            state->fifo_st.rdy = !fifo32_is_full(state->fifo_tx);
            return state->fifo_st._value;
        case RP2040_SIO_FIFO_RD:
            if (fifo32_is_empty(&state->fifo_rx)) {
                state->fifo_st.roe = true;
                return 0;
            }
            return fifo32_pop(&state->fifo_rx);
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
        offset);

    return 0;
}

static void rp2040_sio_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned int size)
{
    RP2040SioState *state = RP2040_SIO(opaque);
    switch (offset) {
        case RP2040_SIO_FIFO_WR:
            if (fifo32_is_full(state->fifo_tx)) {
                state->fifo_st.wof = true;
                return;
            }
            fifo32_push(state->fifo_tx, value);
            return;
        case RP2040_SIO_FIFO_ST:
            /* According to the datasheet, writing any value to FIFO_ST clears
               roe and wof flags */
            state->fifo_st.roe = false;
            state->fifo_st.wof = false;
            return;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
        offset);
}


static const MemoryRegionOps rp2040_sio_io = {
    .read = rp2040_sio_read,
    .write = rp2040_sio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};


static void rp2040_sio_realize(DeviceState *dev, Error **errp)
{
    RP2040SioState *state = RP2040_SIO(dev);
    SysBusDevice *sysbus_dev = SYS_BUS_DEVICE(dev);

    if (!state->gpio) {
        error_report("RP2040 GPIO not connected to RP2040 SIO");
        exit(EXIT_FAILURE);
    }

    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_sio_io, state,
        "io", 0x4000);
    sysbus_init_mmio(sysbus_dev, &state->mmio);

    fifo32_create(&state->fifo_rx, RP2040_SIO_INTERCORE_FIFO_SIZE);
}

static Property rp2040_sio_properties[] = {
    DEFINE_PROP_LINK("gpio", RP2040SioState, gpio, TYPE_RP2040_GPIO,
        RP2040GpioState *),
    DEFINE_PROP_UINT32("cpuid", RP2040SioState, cpuid, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_sio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, rp2040_sio_properties);
    dc->desc = "RP2040 SIO";
    dc->realize = rp2040_sio_realize;
}

static const TypeInfo rp2040_sio_type_info = {
    .name           = TYPE_RP2040_SIO,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_sio_class_init,
    .instance_size  = sizeof(RP2040SioState),
    .instance_init  = rp2040_sio_instance_init,
};

static void rp2040_sio_register_types(void)
{
    type_register_static(&rp2040_sio_type_info);
}

type_init(rp2040_sio_register_types);
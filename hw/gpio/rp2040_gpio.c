/*
 * RP2040 GPIO
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

#include "hw/gpio/rp2040_gpio.h"

#include "hw/irq.h"
#include "qemu/log.h"
#include "trace.h"
#include "hw/misc/rp2040_utils.h"

#define RP2040_IO_REGISTER_SIZE         0x8000
#define RP2040_GPIO_REGISTER_SIZE       0x190
#define RP2040_GPIO_QSPI_REGISTER_SIZE  0x58


typedef struct Rp2040GpioRegisterDesc {
    bool in_reset;
    uint32_t state_and_control_end;
    RP2040GpioStatus *status;
    RP2040GpioControl *control;
    qemu_irq *irq;
} Rp2040GpioRegisterDesc;

static uint32_t rp2040_process_gpio_read(hwaddr offset,
    const Rp2040GpioRegisterDesc *desc, const char *name)
{
    if (desc->in_reset) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: periph in reset (%s)", __func__, name);
    }
    /* Process status & control registers */
    if (offset <= desc->state_and_control_end) {
        const hwaddr index = offset >> 2;
        const bool is_stat = (index % 2) == 0;
        if (is_stat) {
            return desc->status[index >> 1]._value;
        } else {
            return desc->control[index >> 1]._value;
        }
    }

    qemu_log_mask(LOG_UNIMP, "%s: GPIO(%s) register at offset: 0x%"
        HWADDR_PRIX " not implemented.\n", __func__, name, offset);
    return 0;
}

static void rp2040_set_gpio_state(RP2040GpioStatus *status, qemu_irq irq,
                                  int value)
{
    status->outtopad = value;
    status->irqtoproc = value;
    status->irqfrompad = value;
    status->intoperi = value;
    status->infrompad = value;
    status->outtopad = value;

    qemu_set_irq(irq, value);
}

static void rp2040_process_gpio_write(hwaddr offset, uint32_t value,
    const Rp2040GpioRegisterDesc *desc, const char *name)
{
    if (desc->in_reset) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: periph in reset (%s)", __func__, name);
    }
    RP2040AccessType access = rp2040_get_access_type(offset);
    offset = offset & 0x0fff;
    /* Process status & control registers */
    if (offset <= desc->state_and_control_end) {
        const hwaddr index = offset >> 2;
        const bool is_stat = (index % 2) == 0;
        if (is_stat) {
            qemu_log_mask(LOG_GUEST_ERROR,
                "%s: Trying to write read only register at address: 0x%lx\n",
                __func__, offset);
            return;
        } else {
            const hwaddr array_index = index >> 1;
            rp2040_write_to_register(access,
                &desc->control[array_index]._value, value);
            if (desc->control[array_index].oeover == 0x03) {
                desc->status[array_index].oetopad = 1;

                desc->status[array_index].irqtoproc = 0;
                desc->status[array_index].irqfrompad = 0;
                desc->status[array_index].intoperi = 0;
                desc->status[array_index].infrompad = 0;
                desc->status[array_index].outtopad = 0;

            } else {
                desc->status[array_index].oetopad = 0;
            }

            switch (desc->control[array_index].outover) {
                case 0x02:
                    rp2040_set_gpio_state(&desc->status[array_index],
                        desc->irq[array_index], false);
                    return;
                case 0x03:
                    rp2040_set_gpio_state(&desc->status[array_index],
                        desc->irq[array_index], true);
                    return;
                }
            return;
        }
    }

    qemu_log_mask(LOG_UNIMP, "%s: GPIO(%s) register at offset: 0x%" HWADDR_PRIX
        " not implemented.\n", __func__, name, offset);
}


static void rp2040_gpio_qspi_write(void *opaque, hwaddr offset,
    uint64_t value, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .in_reset = state->qspi_in_reset,
        .state_and_control_end = 0x058,
        .status = state->qspi_status,
        .control = state->qspi_ctrl,
        .irq = state->qspi_out,
    };
    rp2040_process_gpio_write(offset, value, &desc, "QSPI");
}

static uint64_t rp2040_gpio_qspi_read(void *opaque, hwaddr offset,
    unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .in_reset = state->qspi_in_reset,
        .state_and_control_end = 0x058,
        .status = state->qspi_status,
        .control = state->qspi_ctrl,
        .irq = state->qspi_out,
    };

    return rp2040_process_gpio_read(offset, &desc, "QSPI");
}

static const MemoryRegionOps rp2040_gpio_qspi_io = {
    .read = rp2040_gpio_qspi_read,
    .write = rp2040_gpio_qspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_gpio_write(void *opaque, hwaddr offset,
    uint64_t value, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .in_reset = state->gpio_in_reset,
        .state_and_control_end = 0x0ec,
        .status = state->gpio_status,
        .control = state->gpio_ctrl,
        .irq = state->gpio_out,
    };
    rp2040_process_gpio_write(offset, value, &desc, "GPIO");
}


static uint64_t rp2040_gpio_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .in_reset = state->gpio_in_reset,
        .state_and_control_end = 0x0ec,
        .status = state->gpio_status,
        .control = state->gpio_ctrl,
        .irq = state->gpio_out,
    };

    return rp2040_process_gpio_read(offset, &desc, "GPIO");
}


static const MemoryRegionOps rp2040_gpio_io = {
    .read = rp2040_gpio_read,
    .write = rp2040_gpio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};


static void rp2040_gpio_set(void *opaque, int line, int value)
{
    fprintf(stderr, "Set gpio: %d\n", line);
    // rp2040_set_gpio_state
}

static void rp2040_gpio_qspi_set(void *opaque, int line, int value)
{
    fprintf(stderr, "Set qspi gpio: %d\n", line);
    // rp2040_set_gpio_state
}

static void rp2040_gpio_cs_qspi_set(void *opaque, int line, int value)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    /* This comes from periph, this means that has no effect if outover is 1 */
    state->qspi_status[1].outfromperi = value;

    if (state->qspi_ctrl[1].outover == 0x00) {
        qemu_set_irq(state->qspi_out[1], !!value);
    } else if (state->qspi_ctrl[1].outover == 0x01) {
        qemu_set_irq(state->qspi_out[1], !!!value);
    }

    // rp2040_set_gpio_state(&state->qspi_status[1], state->qspi_out[1], value);
}

static void rp2040_gpio_sclk_qspi_set(void *opaque, int line, int value)
{
    fprintf(stderr, "Set sclk gpio: %d\n", line);
    // rp2040_set_gpio_state
}

static void rp2040_gpio_realize(DeviceState *dev, Error **errp)
{
    RP2040GpioState *state = RP2040_GPIO(dev);
    SysBusDevice *sysbus = SYS_BUS_DEVICE(dev);

    memory_region_init(&state->container, OBJECT(dev), "gpio",
        RP2040_IO_REGISTER_SIZE);
    sysbus_init_mmio(sysbus, &state->container);

    /* Initialize GPIOs */
    memory_region_init_io(&state->gpio_mmio, OBJECT(dev), &rp2040_gpio_io,
        state, "io", 0x4000);
    memory_region_add_subregion(&state->container, 0x0000, &state->gpio_mmio);

    qdev_init_gpio_out_named(DEVICE(state), state->gpio_out, "out",
        RP2040_GPIO_PINS);
    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_set, "in",
        RP2040_GPIO_PINS);

    /* Initialize QSPI XIP GPIOs */
    memory_region_init_io(&state->qspi_mmio, OBJECT(dev), &rp2040_gpio_qspi_io,
        state, "qspi", 0x4000);
    memory_region_add_subregion(&state->container, 0x4000, &state->qspi_mmio);


    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_out[2], "qspi-out",
        RP2040_GPIO_QSPI_IO_PINS);
    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_out[1], "qspi-cs", 1);
    qemu_set_irq(state->qspi_out[1], 0);
    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_out[0], "qspi-sclk", 1);

    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_qspi_set, "qspi-in",
        RP2040_GPIO_QSPI_IO_PINS);
    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_cs_qspi_set, "qspi-cs-in", 1);
    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_sclk_qspi_set, "qspi-sclk-in", 1);

    rp2040_gpio_reset(state, true);
    rp2040_qspi_io_reset(state, true);
}

static void rp2040_gpio_unrealize(DeviceState *dev)
{
    RP2040GpioState *state = RP2040_GPIO(dev);

    /*
     * Remove cyclic dependency between the device
     * and it own memory subregions
     */
    memory_region_del_subregion(&state->container, &state->gpio_mmio);
    memory_region_del_subregion(&state->container, &state->qspi_mmio);
}

static void rp2040_gpio_instance_init(Object *obj)
{

}

static void rp2040_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 GPIO";
    dc->realize = rp2040_gpio_realize;
    dc->unrealize = rp2040_gpio_unrealize;
}

static const TypeInfo rp2040_gpio_qspi_info = {
    .name = TYPE_RP2040_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .class_init = rp2040_gpio_class_init,
    .instance_size = sizeof(RP2040GpioState),
    .instance_init = rp2040_gpio_instance_init,
};

static void rp2040_gpio_qspi_register_types(void)
{
    type_register_static(&rp2040_gpio_qspi_info);
}

type_init(rp2040_gpio_qspi_register_types)


void rp2040_gpio_reset(RP2040GpioState *state, bool reset_state)
{
    state->gpio_reset_done = !reset_state;

    if (reset_state)
    {
        for (int i = 0; i < RP2040_GPIO_PINS; ++i) {
            state->gpio_ctrl[i]._value = 0x00000000;
            state->gpio_ctrl[i].funcsel = 0x1f;
            state->gpio_status[i]._value = 0x00000000;
            state->gpio_status[i].infrompad = 1;
        }
    }

    state->gpio_in_reset = reset_state;
}

void rp2040_qspi_io_reset(RP2040GpioState *state, bool reset_state)
{
    state->qspi_reset_done = !reset_state;

    for (int i = 0; i < RP2040_GPIO_QSPI_IO_PINS; ++i) {
        state->qspi_ctrl[i]._value = 0x00000000;
        state->qspi_ctrl[i].funcsel = 0x1f;
        state->qspi_status[i]._value = 0x00000000;
        state->qspi_status[i].infrompad = 1;
    }

    state->qspi_in_reset = reset_state;
}

int rp2040_gpio_get_reset_state(RP2040GpioState *state)
{
    return state->gpio_in_reset;
}

int rp2040_qspi_io_get_reset_state(RP2040GpioState *state)
{
    return state->qspi_in_reset;
}

int rp2040_gpio_get_reset_done(RP2040GpioState *state)
{
    return state->gpio_reset_done;
}

int rp2040_qspi_io_get_reset_done(RP2040GpioState *state)
{
    return state->qspi_reset_done;
}

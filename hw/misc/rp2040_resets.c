/*
 * RP2040 Resets
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

#include "hw/misc/rp2040_resets.h"

#include "hw/irq.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "trace.h"

#include "hw/gpio/rp2040_gpio.h"
#include "hw/misc/rp2040_pads.h"
#include "hw/misc/rp2040_utils.h"
#include "hw/timer/rp2040_timer.h"

#define RP2040_RESETS_REGISTER_SIZE 0x4000

#define RP2040_RESETS_RESET         0x0
#define RP2040_RESETS_WDSEL         0x4
#define RP2040_RESETS_RESET_DONE    0x8

#define RP2040_RESETS_ADC               0
#define RP2040_RESETS_ADC_MASK          0x1
#define RP2040_RESETS_BUSCTRL           1
#define RP2040_RESETS_BUSCTRL_MASK      0x2
#define RP2040_RESETS_DMA               2
#define RP2040_RESETS_DMA_MASK          0x4
#define RP2040_RESETS_I2C0              3
#define RP2040_RESETS_I2C0_MASK         0x8
#define RP2040_RESETS_I2C1              4
#define RP2040_RESETS_I2C1_MASK         0x10
#define RP2040_RESETS_IO_BANK0          5
#define RP2040_RESETS_IO_BANK0_MASK     0x20
#define RP2040_RESETS_IO_QSPI           6
#define RP2040_RESETS_IO_QSPI_MASK      0x40
#define RP2040_RESETS_JTAG              7
#define RP2040_RESETS_JTAG_MASK         0x80
#define RP2040_RESETS_PADS_BANK0        8
#define RP2040_RESETS_PADS_BANK0_MASK   0x100
#define RP2040_RESETS_PADS_QSPI         9
#define RP2040_RESETS_PADS_QSPI_MASK    0x200
#define RP2040_RESETS_PIO0              10
#define RP2040_RESETS_PIO0_MASK         0x400
#define RP2040_RESETS_PIO1              11
#define RP2040_RESETS_PIO1_MASK         0x800
#define RP2040_RESETS_PLL_SYS           12
#define RP2040_RESETS_PLL_SYS_MASK      0x1000
#define RP2040_RESETS_PLL_USB           13
#define RP2040_RESETS_PLL_USB_MASK      0x2000
#define RP2040_RESETS_PWM               14
#define RP2040_RESETS_PWM_MASK          0x4000
#define RP2040_RESETS_RTC               15
#define RP2040_RESETS_RTC_MASK          0x8000
#define RP2040_RESETS_SPI0              16
#define RP2040_RESETS_SPI0_MASK         0x10000
#define RP2040_RESETS_SPI1              17
#define RP2040_RESETS_SPI1_MASK         0x20000
#define RP2040_RESETS_SYSCFG            18
#define RP2040_RESETS_SYSCFG_MASK       0x40000
#define RP2040_RESETS_SYSINFO           19
#define RP2040_RESETS_SYSINFO_MASK      0x80000
#define RP2040_RESETS_TBMAN             20
#define RP2040_RESETS_TBMAN_MASK        0x100000
#define RP2040_RESETS_TIMER             21
#define RP2040_RESETS_TIMER_MASK        0x200000
#define RP2040_RESETS_UART0             22
#define RP2040_RESETS_UART0_MASK        0x400000
#define RP2040_RESETS_UART1             23
#define RP2040_RESETS_UART1_MASK        0x800000
#define RP2040_RESETS_USBCTRL           24
#define RP2040_RESETS_USBCTRL_MASK      0x1000000


static void rp2040_resets_instance_init(Object *obj)
{
}

static uint32_t rp2040_resets_get_state(RP2040ResetsState *state)
{
    const int gpio_state = rp2040_gpio_get_reset_state(state->gpio)
        << RP2040_RESETS_IO_BANK0;
    const int qspi_state = rp2040_qspi_io_get_reset_state(state->gpio)
        << RP2040_RESETS_IO_QSPI;
    const int pads_state = rp2040_pads_get_reset_state(state->pads)
        << RP2040_RESETS_PADS_BANK0;
    const int pads_qspi_state = rp2040_qspi_pads_get_reset_state(state->pads)
        << RP2040_RESETS_PADS_QSPI;
    const int timer_state = rp2040_timer_get_reset_state(state->timer)
        << RP2040_RESETS_TIMER;

    const uint32_t reset_state =
        1 << RP2040_RESETS_ADC |
        1 << RP2040_RESETS_BUSCTRL |
        1 << RP2040_RESETS_DMA |
        1 << RP2040_RESETS_I2C0 |
        1 << RP2040_RESETS_I2C1 |
        gpio_state |
        qspi_state |
        1 << RP2040_RESETS_JTAG |
        pads_state |
        pads_qspi_state |
        1 << RP2040_RESETS_PIO0 |
        1 << RP2040_RESETS_PIO1 |
        1 << RP2040_RESETS_PLL_SYS |
        1 << RP2040_RESETS_PLL_USB |
        1 << RP2040_RESETS_PWM |
        1 << RP2040_RESETS_RTC |
        1 << RP2040_RESETS_SPI0 |
        1 << RP2040_RESETS_SPI1 |
        1 << RP2040_RESETS_SYSCFG |
        1 << RP2040_RESETS_SYSINFO |
        1 << RP2040_RESETS_TBMAN |
        timer_state |
        1 << RP2040_RESETS_UART0 |
        1 << RP2040_RESETS_UART1 |
        1 << RP2040_RESETS_USBCTRL;
    return reset_state;
}


static uint64_t rp2040_resets_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040ResetsState *state = RP2040_RESETS(opaque);

    switch (offset) {
        case RP2040_RESETS_RESET:
            const uint32_t reset_state = rp2040_resets_get_state(state);
            return reset_state;
        case RP2040_RESETS_RESET_DONE:
            // fprintf(stderr, "Get reset done\n");
            const int gpio_done = rp2040_gpio_get_reset_done(state->gpio)
                    << RP2040_RESETS_IO_BANK0;
            const int qspi_done = rp2040_qspi_io_get_reset_done(state->gpio)
                    << RP2040_RESETS_IO_QSPI;
            const int pads_done = rp2040_pads_get_reset_done(state->pads)
                    << RP2040_RESETS_PADS_BANK0;
            const int pads_qspi_done =
                rp2040_qspi_pads_get_reset_done(state->pads)
                    << RP2040_RESETS_PADS_QSPI;
            const int timer_done = rp2040_timer_get_reset_done(state->timer)
                << RP2040_RESETS_TIMER;

            const uint32_t reset_done =
                0 << RP2040_RESETS_ADC |
                1 << RP2040_RESETS_BUSCTRL |
                1 << RP2040_RESETS_DMA |
                1 << RP2040_RESETS_I2C0 |
                1 << RP2040_RESETS_I2C1 |
                gpio_done |
                qspi_done |
                1 << RP2040_RESETS_JTAG |
                pads_done |
                pads_qspi_done |
                1 << RP2040_RESETS_PIO0 |
                1 << RP2040_RESETS_PIO1 |
                1 << RP2040_RESETS_PLL_SYS |
                1 << RP2040_RESETS_PLL_USB |
                1 << RP2040_RESETS_PWM |
                0 << RP2040_RESETS_RTC |
                0 << RP2040_RESETS_SPI0 |
                0 << RP2040_RESETS_SPI1 |
                1 << RP2040_RESETS_SYSCFG |
                1 << RP2040_RESETS_SYSINFO |
                1 << RP2040_RESETS_TBMAN |
                timer_done |
                0 << RP2040_RESETS_UART0 |
                0 << RP2040_RESETS_UART1 |
                0 << RP2040_RESETS_USBCTRL;
        return reset_done;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);

    return 0;
}

static void rp2040_reset_unimp(const char *name, uint32_t value)
{
    qemu_log_mask(LOG_UNIMP, "%s: called reset of %s with %x\n", __func__,
                  name, value);
}

static void rp2040_resets_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned int size)
{
    RP2040ResetsState *state = RP2040_RESETS(opaque);
    RP2040AccessType access = rp2040_get_access_type(offset);
    offset = offset & 0x0fff;
    switch (offset) {
        case RP2040_RESETS_RESET:
            uint32_t reset_state = rp2040_resets_get_state(state);
            rp2040_write_to_register(access, &reset_state, value);
            rp2040_reset_unimp("adc", reset_state & RP2040_RESETS_ADC_MASK);
            rp2040_reset_unimp("busctrl", reset_state & RP2040_RESETS_BUSCTRL_MASK);
            rp2040_reset_unimp("dma", reset_state & RP2040_RESETS_DMA_MASK);
            rp2040_reset_unimp("i2c0", reset_state & RP2040_RESETS_I2C0_MASK);
            rp2040_reset_unimp("i2c1", reset_state & RP2040_RESETS_I2C1_MASK);
            rp2040_gpio_reset(state->gpio,
                reset_state & RP2040_RESETS_IO_BANK0_MASK);
            rp2040_qspi_io_reset(state->gpio,
                reset_state & RP2040_RESETS_IO_QSPI_MASK);
            rp2040_reset_unimp("jtag", reset_state & RP2040_RESETS_JTAG_MASK);
            rp2040_pads_reset(state->pads,
                reset_state & RP2040_RESETS_PADS_BANK0_MASK);
            rp2040_qspi_pads_reset(state->pads,
                reset_state & RP2040_RESETS_PADS_QSPI_MASK);
            rp2040_reset_unimp("pio0", reset_state & RP2040_RESETS_PIO0_MASK);
            rp2040_reset_unimp("pio1", reset_state & RP2040_RESETS_PIO1_MASK);
            rp2040_reset_unimp("pllsys", reset_state & RP2040_RESETS_PLL_SYS_MASK);
            rp2040_reset_unimp("pllusb", reset_state & RP2040_RESETS_PLL_USB_MASK);
            rp2040_reset_unimp("pwm", reset_state & RP2040_RESETS_PWM_MASK);
            rp2040_reset_unimp("rtc", reset_state & RP2040_RESETS_RTC_MASK);
            rp2040_reset_unimp("spi0", reset_state & RP2040_RESETS_SPI0_MASK);
            rp2040_reset_unimp("spi1", reset_state & RP2040_RESETS_SPI1_MASK);
            rp2040_reset_unimp("syscfg", reset_state & RP2040_RESETS_SYSCFG_MASK);
            rp2040_reset_unimp("sysinfo", reset_state & RP2040_RESETS_SYSINFO_MASK);
            rp2040_reset_unimp("tbman", reset_state & RP2040_RESETS_TBMAN_MASK);
            rp2040_timer_reset(state->timer,
                reset_state & RP2040_RESETS_TIMER_MASK);
            rp2040_reset_unimp("uart0", reset_state & RP2040_RESETS_UART0_MASK);
            rp2040_reset_unimp("uart1", reset_state & RP2040_RESETS_UART1_MASK);
            rp2040_reset_unimp("usbctrl", reset_state & RP2040_RESETS_USBCTRL_MASK);
        return;
        case RP2040_RESETS_RESET_DONE:
            qemu_log_mask(LOG_GUEST_ERROR, "%s: read only register: 0x%03lx\n",
                          __func__, offset);
        return;
    }
    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);
}

static const MemoryRegionOps rp2040_resets_io = {
    .read = rp2040_resets_read,
    .write = rp2040_resets_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_resets_realize(DeviceState *dev, Error **errp)
{
    RP2040ResetsState *state = RP2040_RESETS(dev);

    if (!state->gpio) {
        error_report("RP2040 GPIO not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pads) {
        error_report("RP2040 PADS not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->timer) {
        error_report("RP2040 TIMER not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }

    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_resets_io, state,
        "mmio", RP2040_RESETS_REGISTER_SIZE);
}

static void rp2040_resets_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 Resets";
    dc->realize = rp2040_resets_realize;
}

static const TypeInfo rp2040_resets_type_info = {
    .name           = TYPE_RP2040_RESETS,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .class_init     = rp2040_resets_class_init,
    .instance_size  = sizeof(RP2040ResetsState),
    .instance_init  = rp2040_resets_instance_init,
};

static void rp2040_resets_register_types(void)
{
    type_register_static(&rp2040_resets_type_info);
}

type_init(rp2040_resets_register_types);
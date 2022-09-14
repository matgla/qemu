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

#include "hw/qdev-properties.h"
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
    const uint32_t reset_state =
        resettable_is_in_reset(state->adc) << RP2040_RESETS_ADC |
        resettable_is_in_reset(state->busctrl) << RP2040_RESETS_BUSCTRL |
        resettable_is_in_reset(state->dma) << RP2040_RESETS_DMA |
        resettable_is_in_reset(state->i2c0) << RP2040_RESETS_I2C0 |
        resettable_is_in_reset(state->i2c1) << RP2040_RESETS_I2C1 |
        resettable_is_in_reset(state->gpio) << RP2040_RESETS_IO_BANK0 |
        resettable_is_in_reset(state->qspi) << RP2040_RESETS_IO_QSPI |
        state->jtag << RP2040_RESETS_JTAG |
        resettable_is_in_reset(state->pads) << RP2040_RESETS_PADS_BANK0 |
        resettable_is_in_reset(state->qspi_pads) << RP2040_RESETS_PADS_QSPI |
        resettable_is_in_reset(state->pio0) << RP2040_RESETS_PIO0 |
        resettable_is_in_reset(state->pio1) << RP2040_RESETS_PIO1 |
        resettable_is_in_reset(state->pllsys) << RP2040_RESETS_PLL_SYS |
        resettable_is_in_reset(state->pllusb) << RP2040_RESETS_PLL_USB |
        resettable_is_in_reset(state->pwm) << RP2040_RESETS_PWM |
        resettable_is_in_reset(state->rtc) << RP2040_RESETS_RTC |
        resettable_is_in_reset(state->spi0) << RP2040_RESETS_SPI0 |
        resettable_is_in_reset(state->spi1) << RP2040_RESETS_SPI1 |
        resettable_is_in_reset(state->syscfg) << RP2040_RESETS_SYSCFG |
        resettable_is_in_reset(state->sysinfo) << RP2040_RESETS_SYSINFO |
        resettable_is_in_reset(state->tbman) << RP2040_RESETS_TBMAN |
        resettable_is_in_reset(state->timer) << RP2040_RESETS_TIMER |
        resettable_is_in_reset(state->uart0) << RP2040_RESETS_UART0 |
        resettable_is_in_reset(state->uart1) << RP2040_RESETS_UART1 |
        resettable_is_in_reset(state->usbctrl) << RP2040_RESETS_USBCTRL;
    fprintf(stderr, "Get reset state: %x\n", reset_state);

    return reset_state;
}


static uint64_t rp2040_resets_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040ResetsState *state = RP2040_RESETS(opaque);

    switch (offset) {
        case RP2040_RESETS_RESET:
            return rp2040_resets_get_state(state);
        case RP2040_RESETS_RESET_DONE:
            return ~rp2040_resets_get_state(state);
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
                  offset);

    return 0;
}

static void rp2040_perform_reset(Object *obj, bool is_reset)
{
    if (is_reset) {
        if (!resettable_is_in_reset(obj)) {
            resettable_assert_reset(obj, RESET_TYPE_COLD);
        }
    } else {
        if (resettable_is_in_reset(obj)) {
            fprintf(stderr, "Unreset\n");
            resettable_release_reset(obj, RESET_TYPE_COLD);
        }
    }
}

static void rp2040_resets_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned int size)
{
    RP2040ResetsState *state = RP2040_RESETS(opaque);
    RP2040AccessType access = rp2040_get_access_type(offset);
    uint32_t reset_state = 0;

    offset = offset & 0x0fff;
    switch (offset) {
        case RP2040_RESETS_RESET:
            reset_state = rp2040_resets_get_state(state);
            rp2040_write_to_register(access, &reset_state, value);
            fprintf(stderr, "Reset state: %x, value: %lx\n", reset_state, value);

            rp2040_perform_reset(state->adc,
                reset_state & RP2040_RESETS_ADC_MASK);
            rp2040_perform_reset(state->busctrl,
                reset_state & RP2040_RESETS_BUSCTRL_MASK);
            rp2040_perform_reset(state->dma,
                reset_state & RP2040_RESETS_DMA_MASK);
            rp2040_perform_reset(state->i2c0,
                reset_state & RP2040_RESETS_I2C0_MASK);
            rp2040_perform_reset(state->i2c1,
                reset_state & RP2040_RESETS_I2C1_MASK);
            rp2040_perform_reset(state->gpio,
                reset_state & RP2040_RESETS_IO_BANK0_MASK);
            rp2040_perform_reset(state->qspi,
                reset_state & RP2040_RESETS_IO_QSPI_MASK);
            state->jtag = !!(reset_state & RP2040_RESETS_JTAG_MASK);
            // rp2040_perform_reset(state->jtag,
            //     reset_state & RP2040_RESETS_JTAG_MASK);
            rp2040_perform_reset(state->pads,
                reset_state & RP2040_RESETS_PADS_BANK0_MASK);
            rp2040_perform_reset(state->qspi_pads,
                reset_state & RP2040_RESETS_PADS_QSPI_MASK);
            rp2040_perform_reset(state->pio0,
                reset_state & RP2040_RESETS_PIO0_MASK);
            rp2040_perform_reset(state->pio1,
                reset_state & RP2040_RESETS_PIO1_MASK);
            rp2040_perform_reset(state->pllsys,
                reset_state & RP2040_RESETS_PLL_SYS_MASK);
            rp2040_perform_reset(state->pllusb,
                reset_state & RP2040_RESETS_PLL_USB_MASK);
            rp2040_perform_reset(state->pwm,
                reset_state & RP2040_RESETS_PWM_MASK);
            rp2040_perform_reset(state->rtc,
                reset_state & RP2040_RESETS_RTC_MASK);
            rp2040_perform_reset(state->spi0,
                reset_state & RP2040_RESETS_SPI0_MASK);
            rp2040_perform_reset(state->spi1,
                reset_state & RP2040_RESETS_SPI1_MASK);
            rp2040_perform_reset(state->syscfg,
                reset_state & RP2040_RESETS_SYSCFG_MASK);
            rp2040_perform_reset(state->sysinfo,
                reset_state & RP2040_RESETS_SYSINFO_MASK);
            rp2040_perform_reset(state->tbman,
                reset_state & RP2040_RESETS_TBMAN_MASK);
            rp2040_perform_reset(state->timer,
                reset_state & RP2040_RESETS_TIMER_MASK);
            rp2040_perform_reset(state->uart0,
                reset_state & RP2040_RESETS_UART0_MASK);
            rp2040_perform_reset(state->uart1,
                reset_state & RP2040_RESETS_UART1_MASK);
            rp2040_perform_reset(state->usbctrl,
                reset_state & RP2040_RESETS_USBCTRL_MASK);
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

    if (!state->adc) {
        error_report("RP2040 ADC not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->busctrl) {
        error_report("RP2040 BUSCTRL not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->dma) {
        error_report("RP2040 DMA not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->i2c0) {
        error_report("RP2040 I2C0 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->i2c1) {
        error_report("RP2040 I2C1 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->gpio) {
        error_report("RP2040 GPIO not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->qspi) {
        error_report("RP2040 QSPI not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pads) {
        error_report("RP2040 PADS not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->qspi_pads) {
        error_report("RP2040 QSPI PADS not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pio0) {
        error_report("RP2040 PIO0 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pio1) {
        error_report("RP2040 PIO1 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pllsys) {
        error_report("RP2040 PLLSYS not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pllusb) {
        error_report("RP2040 PLLUSB not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->pwm) {
        error_report("RP2040 PWM not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->rtc) {
        error_report("RP2040 RTC not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->spi0) {
        error_report("RP2040 SIO0 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->spi1) {
        error_report("RP2040 SIO1 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->syscfg) {
        error_report("RP2040 SYSCFG not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->sysinfo) {
        error_report("RP2040 SYSINFO not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->tbman) {
        error_report("RP2040 TBMAN not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->timer) {
        error_report("RP2040 TIMER not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->uart0) {
        error_report("RP2040 UART0 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->uart1) {
        error_report("RP2040 UART1 not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }
    if (!state->usbctrl) {
        error_report("RP2040 USBCTRL not connected to RP2040 RESETS");
        exit(EXIT_FAILURE);
    }

    sysbus_init_mmio(SYS_BUS_DEVICE(state), &state->mmio);
    memory_region_init_io(&state->mmio, OBJECT(dev), &rp2040_resets_io, state,
        "mmio", RP2040_RESETS_REGISTER_SIZE);
}

static Property rp2040_resets_properties[] = {
    DEFINE_PROP_LINK("adc", RP2040ResetsState, adc, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("busctrl", RP2040ResetsState, busctrl, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("dma", RP2040ResetsState, dma, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("i2c0", RP2040ResetsState, i2c0, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("i2c1", RP2040ResetsState, i2c1, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("gpio", RP2040ResetsState, gpio, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("qspi", RP2040ResetsState, qspi, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("pads", RP2040ResetsState, pads, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("qspi_pads", RP2040ResetsState, qspi_pads, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("pio0", RP2040ResetsState, pio0, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("pio1", RP2040ResetsState, pio1, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("pllsys", RP2040ResetsState, pllsys, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("pllusb", RP2040ResetsState, pllusb, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("pwm", RP2040ResetsState, pwm, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("rtc", RP2040ResetsState, rtc, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("spi0", RP2040ResetsState, spi0, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("spi1", RP2040ResetsState, spi1, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("syscfg", RP2040ResetsState, syscfg, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("sysinfo", RP2040ResetsState, sysinfo, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_LINK("tbman", RP2040ResetsState, tbman, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("timer", RP2040ResetsState, timer, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("uart0", RP2040ResetsState, uart0, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("uart1", RP2040ResetsState, uart1, TYPE_OBJECT, Object *),
    DEFINE_PROP_LINK("usbctrl", RP2040ResetsState, usbctrl, TYPE_OBJECT,
                     Object *),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_resets_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, rp2040_resets_properties);
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
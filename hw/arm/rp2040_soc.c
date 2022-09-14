/*
 * RP2040 SoC
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

#include "hw/arm/rp2040_soc.h"

#include "qemu/osdep.h"
#include "hw/qdev-clock.h"
#include "exec/address-spaces.h"
#include "hw/misc/unimp.h"
#include "hw/arm/boot.h"
#include "hw/loader.h"
#include "qemu/datadir.h"
#include "qapi/error.h"


#define RP2040_ROM_BASE_ADDRESS 0x00000000

#define RP2040_XIP_BASE_ADDRESS 0x10000000

#define RP2040_SRAM03_SIZE 0x40000
#define RP2040_SRAM4_SIZE 0x1000
#define RP2040_SRAM5_SIZE 0x1000

#define RP2040_XIP_BASE                 0x10000000
#define RP2040_XIP_NOALLOC_BASE         0x11000000
#define RP2040_XIP_NOCACHE_BASE         0x12000000
#define RP2040_XIP_NOCACHE_NOALLOC_BASE 0x13000000
#define RP2040_XIP_CTRL_BASE            0x14000000
#define RP2040_XIP_SRAM_BASE            0x15000000
#define RP2040_XIP_SRAM_END             0x15004000
#define RP2040_XIP_SSI_BASE             0x18000000
#define RP2040_SRAM_BASE                0x20000000
#define RP2040_SRAM_STRIPED_END         0x20040000
#define RP2040_SRAM4_BASE               0x20040000
#define RP2040_SRAM5_BASE               0x20041000
#define RP2040_SRAM_END                 0x20042000
#define RP2040_SRAM0_BASE               0x21000000
#define RP2040_SRAM1_BASE               0x21010000
#define RP2040_SRAM2_BASE               0x21020000
#define RP2040_SRAM3_BASE               0x21030000
#define RP2040_SYSINFO_BASE             0x40000000
#define RP2040_SYSCFG_BASE              0x40004000
#define RP2040_CLOCKS_BASE              0x40008000
#define RP2040_RESETS_BASE              0x4000c000
#define RP2040_PSM_BASE                 0x40010000
#define RP2040_IO_BANK0_BASE            0x40014000
#define RP2040_QSPI_IO_BASE             0x40018000
#define RP2040_PADS_BANK0_BASE          0x4001c000
#define RP2040_PADS_QSPI_BASE           0x40020000
#define RP2040_XOSC_BASE                0x40024000
#define RP2040_PLL_SYS_BASE             0x40028000
#define RP2040_BUSCTRL_BASE             0x40030000
#define RP2040_PLL_USB_BASE             0x4002c000
#define RP2040_BUSCTRL_BASE             0x40030000
#define RP2040_UART0_BASE               0x40034000
#define RP2040_UART1_BASE               0x40038000
#define RP2040_SPI0_BASE                0x4003c000
#define RP2040_SPI1_BASE                0x40040000
#define RP2040_I2C0_BASE                0x40044000
#define RP2040_I2C1_BASE                0x40048000
#define RP2040_ADC_BASE                 0x4004c000
#define RP2040_PWM_BASE                 0x40050000
#define RP2040_TIMER_BASE               0x40054000
#define RP2040_WATCHDOG_BASE            0x40058000
#define RP2040_RTC_BASE                 0x4005c000
#define RP2040_ROSC_BASE                0x40060000
#define RP2040_VREG_AND_CHIP_RESET_BASE 0x40064000
#define RP2040_TBMAN_BASE               0x4006c000
#define RP2040_DMA_BASE                 0x50000000
#define RP2040_USBCTRL_BASE             0x50100000
#define RP2040_USBCTRL_REGS_BASE        0x50110000
#define RP2040_PIO0_BASE                0x50200000
#define RP2040_PIO1_BASE                0x50300000
#define RP2040_XIP_AUX_BASE             0x50400000
#define RP2040_SIO_BASE                 0xd0000000

#define RP2040_PICO_BOOTROM_FILE "raspi_pico_boot.bin"


static void rp2040_load_rom(void)
{
    int n = 0;
    g_autofree char *rom_file = qemu_find_file(QEMU_FILE_TYPE_BIOS,
        RP2040_PICO_BOOTROM_FILE);

    if (!rom_file) {
        error_setg(&error_abort, "%s: bootrom image not found. "
            "Make sure that pc-bios contains '%s'"
            "or provide custom one with -kernel parameter", __func__,
            RP2040_PICO_BOOTROM_FILE);
    }
    n = load_image_targphys(rom_file, 0x0, 16 * KiB);
    if (n <= 0) {
        error_setg(&error_abort, "%s: Can't load bootrom image: %s",
            __func__, RP2040_PICO_BOOTROM_FILE);
    }
}

static void rp2040_soc_init(Object *obj)
{
    RP2040State *s = RP2040_SOC(obj);
    int i = 0;

    memory_region_init(&s->container, obj, "rp2040-container", UINT64_MAX);

    object_initialize_child(obj, "gpio", &s->gpio, TYPE_RP2040_GPIO);
    object_initialize_child(obj, "qspi_io", &s->qspi_io, TYPE_RP2040_QSPI_IO);

    /* cores initialization */
    for (i = 0; i < RP2040_SOC_NUMBER_OF_CORES; ++i) {
        g_autofree char *cpu_name = g_strdup_printf("armv6m[%d]", i);
        g_autofree char *container_name = g_strdup_printf("container[%d]", i);
        g_autofree char *sio_name = g_strdup_printf("sio[%d]", i);
        object_initialize_child(obj, cpu_name, &s->armv6m[i], TYPE_ARMV7M);
        qdev_prop_set_string(DEVICE(&s->armv6m[i]), "cpu-type",
            ARM_CPU_TYPE_NAME("cortex-m0"));

        memory_region_init(&s->core_container[i], obj,
            container_name, UINT64_MAX);

        if (i > 0) {
            g_autofree char *container_alias_name =
                g_strdup_printf("container_alias[%d]", i);
            memory_region_init_alias(&s->core_container_alias[i - 1], obj,
                container_alias_name, &s->container, 0, UINT64_MAX);
        }

        object_initialize_child(obj, sio_name, &s->sio[i], TYPE_RP2040_SIO);
    }

    /* peripherals initialization */
    object_initialize_child(obj, "pads", &s->pads, TYPE_RP2040_PADS);

    s->resets = rp2040_resets_create();
    object_property_add_child(obj, "resets", OBJECT(s->resets));

    object_initialize_child(obj, "timer", &s->timer, TYPE_RP2040_TIMER);
    object_initialize_child(obj, "ssi", &s->ssi, TYPE_RP2040_SSI);
    object_initialize_child(obj, "xip", &s->xip, TYPE_RP2040_XIP);
    object_initialize_child(obj, "xosc", &s->xosc, TYPE_RP2040_XOSC);
    object_initialize_child(obj, "clocks", &s->clocks, TYPE_RP2040_CLOCKS);
    object_initialize_child(obj, "pll_sys", &s->pll_sys, TYPE_RP2040_PLL);
    object_initialize_child(obj, "pll_usb", &s->pll_usb, TYPE_RP2040_PLL);
    object_initialize_child(obj, "uart0", &s->uart0, TYPE_RP2040_UART);
    qdev_prop_set_chr(DEVICE(&s->uart0.pl011), "chardev", serial_hd(0));
    object_initialize_child(obj, "uart1", &s->uart1, TYPE_RP2040_UART);
    qdev_prop_set_chr(DEVICE(&s->uart1.pl011), "chardev", serial_hd(1));

    s->sysclk = qdev_init_clock_in(DEVICE(s), "sysclk", NULL, NULL, 0);
    s->refclk = qdev_init_clock_in(DEVICE(s), "refclk", NULL, NULL, 0);
}

static void rp2040_soc_realize(DeviceState *dev_soc, Error **errp)
{
    RP2040State *s = RP2040_SOC(dev_soc);
    Object *o;
    int i;
    qemu_irq cs_line_gpio;

    if (!clock_has_source(s->sysclk)) {
        error_setg(errp, "System clock not wired to RP2040 SoC");
        return;
    }

    if (!s->system_memory) {
        error_setg(errp, "System memory not wired to RP2040 SoC");
        return;
    }

    memory_region_add_subregion_overlap(&s->container, 0, s->system_memory,
        1);

   /* TODO: hacks to be removed, clocks should be setted up correctly */
    clock_set_mul_div(s->refclk, 8, 1);
    clock_set_source(s->refclk, s->sysclk);

    /* Initialize boot rom */
    memory_region_init_rom(&s->rom, OBJECT(dev_soc), "RP2040.rom",
        RP2040_ROM_SIZE, &error_fatal);
    memory_region_add_subregion(s->system_memory, RP2040_ROM_BASE_ADDRESS,
        &s->rom);

    rp2040_load_rom();

    memory_region_init_ram(&s->sram03, OBJECT(dev_soc), "RP2040.sram03",
        RP2040_SRAM03_SIZE, &error_fatal);

    memory_region_add_subregion(&s->container, RP2040_SRAM_BASE,
        &s->sram03);

    memory_region_init_ram(&s->sram4, OBJECT(dev_soc), "RP2040.sram4",
        RP2040_SRAM4_SIZE, &error_fatal);

    memory_region_add_subregion(&s->container, RP2040_SRAM4_BASE,
        &s->sram4);

    memory_region_init_ram(&s->sram5, OBJECT(dev_soc), "RP2040.sram5",
        RP2040_SRAM5_SIZE, &error_fatal);

    memory_region_add_subregion(&s->container, RP2040_SRAM5_BASE,
        &s->sram5);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->xip), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->xip), 0, RP2040_XIP_BASE);

    create_unimplemented_device("XIP NOALLOC", RP2040_XIP_NOALLOC_BASE,
        0x01000000);
    create_unimplemented_device("XIP NOCACHE", RP2040_XIP_NOCACHE_BASE,
        0x01000000);
    create_unimplemented_device("XIP NOCACHE", RP2040_XIP_NOCACHE_NOALLOC_BASE,
        0x01000000);
    create_unimplemented_device("XIP CTRL", RP2040_XIP_CTRL_BASE, 0x4000);
    create_unimplemented_device("XIP SRAM", RP2040_XIP_SRAM_BASE, 0x4000);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->ssi), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ssi), 0, RP2040_XIP_SSI_BASE);

    create_unimplemented_device("SRAM0 BASE", RP2040_SRAM0_BASE, 0x10000);
    create_unimplemented_device("SRAM1 BASE", RP2040_SRAM1_BASE, 0x10000);
    create_unimplemented_device("SRAM2 BASE", RP2040_SRAM2_BASE, 0x10000);
    create_unimplemented_device("SRAM3 BASE", RP2040_SRAM3_BASE, 0x10000);
    o = OBJECT(create_unimplemented_device("SYSINFO",
                               RP2040_SYSINFO_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "sysinfo", o, errp);
    o = OBJECT(create_unimplemented_device("SYSCFG",
                              RP2040_SYSCFG_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "syscfg", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->clocks), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->clocks), 0, RP2040_CLOCKS_BASE);

    create_unimplemented_device("PSM", RP2040_PSM_BASE, 0x4000);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->gpio), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, RP2040_IO_BANK0_BASE);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->qspi_io), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->qspi_io), 0, RP2040_QSPI_IO_BASE);


    if (!sysbus_realize(SYS_BUS_DEVICE(&s->pads), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pads), 0, RP2040_PADS_BANK0_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pads), 1, RP2040_PADS_QSPI_BASE);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->xosc), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->xosc), 0, RP2040_XOSC_BASE);

    o = OBJECT(create_unimplemented_device("BUSCTRL",
        RP2040_BUSCTRL_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "busctrl", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->pll_sys), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pll_sys), 0, RP2040_PLL_SYS_BASE);
    o = OBJECT(&s->pll_sys);
    object_property_set_link(OBJECT(s->resets), "pllsys", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->pll_usb), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pll_usb), 0, RP2040_PLL_USB_BASE);
    o = OBJECT(&s->pll_usb);
    object_property_set_link(OBJECT(s->resets), "pllusb", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->uart0), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart0), 0, RP2040_UART0_BASE);
    o = OBJECT(&s->uart0);
    object_property_set_link(OBJECT(s->resets), "uart0", o, errp);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->uart1), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart1), 0, RP2040_UART1_BASE);
    o = OBJECT(&s->uart1);
    object_property_set_link(OBJECT(s->resets), "uart1", o, errp);

    o = OBJECT(create_unimplemented_device("SPI0",
        RP2040_SPI0_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "spi0", o, errp);

    o = OBJECT(create_unimplemented_device("SPI1",
        RP2040_SPI1_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "spi1", o, errp);

    o = OBJECT(create_unimplemented_device("I2C0",
        RP2040_I2C0_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "i2c0", o, errp);

    o = OBJECT(create_unimplemented_device("I2C1",
        RP2040_I2C1_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "i2c1", o, errp);

    o = OBJECT(create_unimplemented_device("ADC",
        RP2040_ADC_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "adc", o, errp);

    o = OBJECT(create_unimplemented_device("PWM",
        RP2040_PWM_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "pwm", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->timer), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, RP2040_TIMER_BASE);

    create_unimplemented_device("WATCHDOG",
        RP2040_WATCHDOG_BASE, 0x4000);
    o = OBJECT(create_unimplemented_device("RTC",
        RP2040_RTC_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "rtc", o, errp);

    create_unimplemented_device("ROSC",
        RP2040_ROSC_BASE, 0x4000);
    create_unimplemented_device("VREG AND CHIP RESET",
        RP2040_VREG_AND_CHIP_RESET_BASE, 0x8000);
    o = OBJECT(create_unimplemented_device("TBMAN",
        RP2040_TBMAN_BASE, 0x4000));
    object_property_set_link(OBJECT(s->resets), "tbman", o, errp);

    o = OBJECT(create_unimplemented_device("DMA",
        RP2040_DMA_BASE, 0x100000));
    object_property_set_link(OBJECT(s->resets), "dma", o, errp);

    o = OBJECT(create_unimplemented_device("USB CTRL",
        RP2040_USBCTRL_BASE, 0x100000));
    object_property_set_link(OBJECT(s->resets), "usbctrl", o, errp);

    create_unimplemented_device("USB CTRL REGS",
        RP2040_USBCTRL_REGS_BASE, 0x100000);
    o = OBJECT(create_unimplemented_device("PIO0",
        RP2040_PIO0_BASE, 0x100000));
    object_property_set_link(OBJECT(s->resets), "pio0", o, errp);

    o = OBJECT(create_unimplemented_device("PIO1",
        RP2040_PIO1_BASE, 0x100000));
    object_property_set_link(OBJECT(s->resets), "pio1", o, errp);

    create_unimplemented_device("XIP AUX",
        RP2040_XIP_AUX_BASE, 0x100000);

    /* Initialize cores */
    for (i = 0; i < RP2040_SOC_NUMBER_OF_CORES; ++i) {
        DeviceState *core = DEVICE(&s->armv6m[i]);
        qdev_prop_set_uint32(core, "num-irq", 32);
        qdev_prop_set_bit(core, "enable-bitband", true);
        qdev_connect_clock_in(core, "cpuclk", s->sysclk);

        if (i > 0) {
            memory_region_add_subregion_overlap(&s->core_container[i], 0,
                &s->core_container_alias[i - 1], -1);
        } else {
            memory_region_add_subregion_overlap(&s->core_container[i], 0,
                &s->container, -1);
        }

        object_property_set_link(OBJECT(core), "memory",
            OBJECT(&s->core_container[i]), errp);
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv6m[i]), errp)) {
            return;
        }

        qdev_prop_set_uint32(DEVICE(&s->sio[i]), "cpuid", i);
        object_property_set_link(OBJECT(&s->sio[i]), "gpio",
            OBJECT(&s->gpio), &error_abort);
        object_property_set_link(OBJECT(&s->sio[i]), "qspi",
            OBJECT(&s->qspi_io), &error_abort);

        if (!sysbus_realize(SYS_BUS_DEVICE(&s->sio[i]), errp)) {
            return;
        }

        memory_region_add_subregion(&s->core_container[i], RP2040_SIO_BASE,
                                    sysbus_mmio_get_region(SYS_BUS_DEVICE(
                                    &s->sio[i]), 0));
    }

    /* Connect intercore FIFOs */
    s->sio[0].fifo_tx = &s->sio[1].fifo_rx;
    s->sio[1].fifo_tx = &s->sio[0].fifo_rx;

    cs_line_gpio = qdev_get_gpio_in_named(DEVICE(&s->qspi_io), "qspi-cs-in", 0);
    qdev_connect_gpio_out_named(DEVICE(&s->ssi), "cs", 0, cs_line_gpio);

    for (int i = 0; i < RP2040_GPIO_QSPI_IO_PINS; ++i) {
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), i, qdev_get_gpio_in(
                            DEVICE(&s->armv6m), i));
    }

    o = OBJECT(&s->gpio);
    object_property_set_link(OBJECT(s->resets), "gpio", o, errp);

    o = OBJECT(&s->qspi_io);
    object_property_set_link(OBJECT(s->resets), "qspi", o, errp);

    o = OBJECT(&s->pads);
    object_property_set_link(OBJECT(s->resets), "pads", o, errp);

    o = OBJECT(&s->pads);
    object_property_set_link(OBJECT(s->resets), "qspi_pads", o, errp);

    o = OBJECT(&s->timer);
    object_property_set_link(OBJECT(s->resets), "timer", o, errp);

    if (!sysbus_realize(SYS_BUS_DEVICE(s->resets), errp)) {
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(s->resets), 0, RP2040_RESETS_BASE);
}

static Property rp2040_soc_properties[] = {
    DEFINE_PROP_LINK("memory", RP2040State, system_memory, TYPE_MEMORY_REGION,
        MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, rp2040_soc_properties);
    dc->realize = rp2040_soc_realize;
}

static const TypeInfo rp2040_soc_info = {
    .name           = TYPE_RP2040_SOC,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(RP2040State),
    .instance_init  = rp2040_soc_init,
    .class_init     = rp2040_soc_class_init,
};

static void rp2040_soc_types(void)
{
    type_register_static(&rp2040_soc_info);
}

type_init(rp2040_soc_types);

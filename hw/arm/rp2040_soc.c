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

#include "qemu/osdep.h"
#include "hw/qdev-clock.h"
#include "exec/address-spaces.h"
#include "hw/misc/unimp.h"

#include "hw/arm/rp2040_soc.h"
#include "hw/misc/rp2040_vreg.h"

// #include "qapi/error.h"

static const uint32_t uart_addr[RP2040_SOC_NUMBER_OF_UARTS] = {
    0x40034000, 
    0x40038000
};

static const uint32_t spi_addr[RP2040_SOC_NUMBER_OF_SPIS] = {
    0x4003c000,
    0x40040000
};

static const int uart_irq[RP2040_SOC_NUMBER_OF_UARTS] = {
    20, 
    21
};

static const int spi_irq[RP2040_SOC_NUMBER_OF_SPIS] = {
    18, 
    19
};

#define RP2040_SOC_CLOCKS_BASE 0x40008000
#define RP2040_SOC_RESETS_BASE 0x4000c000
#define RP2040_SOC_VREG_BASE   0x40064000
#define RP2040_SOC_XOSC_BASE   0x40024000
#define RP2040_SOC_SIO_BASE    0xd0000000
#define RP2040_SOC_SSI_BASE    0x18000000



static void rp2040_soc_init(Object *obj)
{
    RP2040State *s = RP2040_SOC(obj);
    int i = 0;
    
    /* cores initialization */
    for (i = 0; i < RP2040_SOC_NUMBER_OF_CORES; ++i)
    {
        object_initialize_child(obj, "armv6m[*]", &s->armv6m[i], TYPE_ARMV7M);
        qdev_prop_set_string(DEVICE(&s->armv6m[i]), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));
    }
    
    /* peripherals initialization */
    for (i = 0; i < RP2040_SOC_NUMBER_OF_UARTS; ++i)
    {
        object_initialize_child(obj, "uart[*]", &s->uart[i], TYPE_PL011);
    }
    
    for (i = 0; i < RP2040_SOC_NUMBER_OF_SPIS; ++i)
    {
        object_initialize_child(obj, "spi[*]", &s->spi[i], TYPE_PL022);
    }

    object_initialize_child(obj, "resets", &s->resets, TYPE_RP2040_RESETS);
    object_initialize_child(obj, "vreg", &s->vreg, TYPE_RP2040_VREG);
    object_initialize_child(obj, "clocks", &s->clocks, TYPE_RP2040_CLOCKS);
    object_initialize_child(obj, "xosc", &s->xosc, TYPE_RP2040_XOSC);
    object_initialize_child(obj, "sio", &s->sio, TYPE_RP2040_SIO);
    object_initialize_child(obj, "ssi", &s->ssi, TYPE_RP2040_SSI);

    /* clocks initialization */
    s->sysclk = qdev_init_clock_in(DEVICE(s), "sysclk", NULL, NULL, 0);
    s->refclk = qdev_init_clock_in(DEVICE(s), "refclk", NULL, NULL, 0);
}

static void rp2040_soc_realize(DeviceState *dev_soc, Error **errp)
{
    RP2040State *s = RP2040_SOC(dev_soc);
    DeviceState *core, *dev;
    SysBusDevice *busdev;
    int i;

    MemoryRegion *system_memory = get_system_memory();

        /*
     * We use s->refclk internally and only define it with qdev_init_clock_in()
     * so it is correctly parented and not leaked on an init/deinit; it is not
     * intended as an externally exposed clock.
     */
    if (clock_has_source(s->refclk)) {
        error_setg(errp, "refclk clock must not be wired up by the board code");
        return;
    }

    if (!clock_has_source(s->sysclk)) {
        error_setg(errp, "sysclk clock must be wired up by the board code");
        return;
    }

    /* TODO: hacks to be removed, clocks should be setted up correctly */
    clock_set_mul_div(s->refclk, 8, 1);
    clock_set_source(s->refclk, s->sysclk);

        /* Initialize boot rom */ 
    memory_region_init_rom(&s->rom, OBJECT(dev_soc), "RP2040.rom", 
        RP2040_SOC_ROM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, RP2040_SOC_ROM_BASE_ADDRESS, &s->rom);

    memory_region_init_ram(&s->sram, NULL, "RP2040.sram", 
        RP2040_SOC_SRAM_SIZE, &error_fatal);

    memory_region_add_subregion(system_memory, RP2040_SOC_SRAM_BASE_ADDRESS, &s->sram);

    memory_region_init_rom(&s->qspi_flash, OBJECT(dev_soc), "RP2040.qspi_flash", RP2040_SOC_XIP_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, RP2040_SOC_XIP_BASE_ADDRESS, &s->qspi_flash);


    /* Initialize cores */
    for (i = 0; i < RP2040_SOC_NUMBER_OF_CORES; ++i)
    {
        core = DEVICE(&s->armv6m[i]);
        qdev_prop_set_uint32(core, "num-irq", 32);
        qdev_prop_set_string(core, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));
        qdev_prop_set_bit(core, "enable-bitband", true);
        qdev_connect_clock_in(core, "cpuclk", s->sysclk);
        qdev_connect_clock_in(core, "refclk", s->refclk);
        object_property_set_link(OBJECT(&s->armv6m[i]), "memory",
                                OBJECT(get_system_memory()), &error_abort);
    
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv6m[i]), errp)) {
            return;
        }

        for (int i = 0; i < RP2040_SOC_NUMBER_OF_UARTS; ++i) {
            dev = DEVICE(&s->uart[i]);
            qdev_prop_set_chr(dev, "chardev", serial_hd(i));
            if (!sysbus_realize(SYS_BUS_DEVICE(&s->uart[i]), errp)) {
                return;
            }
            busdev = SYS_BUS_DEVICE(dev);
            sysbus_mmio_map(busdev, 0, uart_addr[i]);
            sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(core, uart_irq[i]));
        }

        /* attach spi controllers */
        for (int i = 0; i < RP2040_SOC_NUMBER_OF_SPIS; ++i) {
            dev = DEVICE(&s->spi[i]);
            if (!sysbus_realize(SYS_BUS_DEVICE(&s->spi[i]), errp)) {
                return;
            }
            busdev = SYS_BUS_DEVICE(dev);
            sysbus_mmio_map(busdev, 0, spi_addr[i]);
            sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(core, spi_irq[i]));
        }
    }

    /* ssi */ 
    dev = DEVICE(&s->ssi);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->ssi), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_SSI_BASE);

    create_unimplemented_device("SYSINFO",      0x40000000, 0x4000);
    create_unimplemented_device("SYSCFG",       0x40004000, 0x4000);
    // create_unimplemented_device("CLOCKS",       0x40008000, 0x4000);
    
    /* clocks */ 
    dev = DEVICE(&s->clocks);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->clocks), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_CLOCKS_BASE);

    /* resets */ 
    dev = DEVICE(&s->resets);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->resets), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_RESETS_BASE);

    create_unimplemented_device("PSM",          0x40010000, 0x4000);
    create_unimplemented_device("IO_BANK[0]",   0x40014000, 0x4000);
    create_unimplemented_device("IO_QSPI",      0x40018000, 0x4000);
    create_unimplemented_device("PADS_BANK[0]", 0x4001c000, 0x4000);
    create_unimplemented_device("PADS_QSPI",    0x40020000, 0x4000);

    // create_unimplemented_device("XOSC",         0x40024000, 0x4000);
    /* xosc register */
    dev = DEVICE(&s->xosc);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->xosc), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_XOSC_BASE);
    
    create_unimplemented_device("PLL_SYS",      0x40028000, 0x4000);
    create_unimplemented_device("PLL_USB",      0x4002c000, 0x4000);
    create_unimplemented_device("BUSCTRL",      0x40030000, 0x4000);
    create_unimplemented_device("I2C[0]",       0x40044000, 0x4000);
    create_unimplemented_device("I2C[1]",       0x40048000, 0x4000);
    create_unimplemented_device("ADC",          0x4004c000, 0x4000);
    create_unimplemented_device("PWM",          0x40050000, 0x4000);
    create_unimplemented_device("TIMER",        0x40054000, 0x4000);
    create_unimplemented_device("WATCHDOG",     0x40058000, 0x4000);
    create_unimplemented_device("RTC",          0x4005c000, 0x4000);
    create_unimplemented_device("ROSC",         0x40060000, 0x4000);

    /* VREG */
    dev = DEVICE(&s->vreg);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->vreg), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_VREG_BASE);

    // create_unimplemented_device("VREG",         0x40064000, 0x4000);
    create_unimplemented_device("TBMAN",        0x4006c000, 0x4000);
    create_unimplemented_device("DMA",          0x50000000, 0x4000);
    create_unimplemented_device("USBCTRL",      0x50100000, 0x4000);
    create_unimplemented_device("PIO[0]",       0x50200000, 0x4000);
    create_unimplemented_device("PIO[1]",       0x50300000, 0x4000);
    // create_unimplemented_device("SIO",          0xd0000000, 0x4000);
    dev = DEVICE(&s->sio);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->sio), errp)) {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, RP2040_SOC_SIO_BASE);

    create_unimplemented_device("PPB",          0xea000000, 0x4000);
}

static Property rp2040_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", RP2040State, cpu_type),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = rp2040_soc_realize;
    device_class_set_props(dc, rp2040_soc_properties);
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
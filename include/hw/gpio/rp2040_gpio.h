#ifndef RP2040_GPIO_QSPI_H
#define RP2040_GPIO_QSPI_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_GPIO_QSPI "rp2040.gpio_qspi"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040GpioQspiState, RP2040_GPIO_QSPI)

struct RP2040GpioQspiState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    qemu_irq out[5];
};

#endif /* RP2040_GPIO_QSPI_H */ 
#ifndef RP2040_RESETS_H
#define RP2040_RESETS_H

#include <stdint.h>

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_RESETS "rp2040.resets"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040ResetsState, RP2040_RESETS);

typedef struct RP2040ResetsStatus
{
    uint32_t _reserved : 8;
    uint32_t usbctrl : 1;
    uint32_t uart1 : 1;
    uint32_t uart0 : 1;
    uint32_t timer : 1;
    uint32_t tbman : 1;
    uint32_t sysinfo : 1;
    uint32_t syscfg : 1;
    uint32_t spi1 : 1;
    uint32_t spi0 : 1;
    uint32_t rtc : 1;
    uint32_t pwm : 1;
    uint32_t pll_usb : 1;
    uint32_t pll_sys : 1;
    uint32_t pio1 : 1;
    uint32_t pio0 : 1;
    uint32_t pads_qspi : 1;
    uint32_t pads_bank0 : 1;
    uint32_t jtag : 1;
    uint32_t io_qspi : 1;
    uint32_t io_bank0 : 1;
    uint32_t i2c1 : 1;
    uint32_t i2c0 : 1;
    uint32_t dma : 1;
    uint32_t busctrl : 1;
    uint32_t adc : 1;
} RP2040ResetsStatus;

struct RP2040ResetsState 
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    RP2040ResetsStatus status;
};

#endif /* RP2040_RESETS_H */
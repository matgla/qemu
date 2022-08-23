#ifndef RP2040_GPIO_QSPI_H
#define RP2040_GPIO_QSPI_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_GPIO "rp2040.gpio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040GpioState, RP2040_GPIO)

#define RP2040_GPIO_PINS      30 
#define RP2040_GPIO_QSPI_PINS 6

typedef union RP2040GpioStatus {
    struct {
        uint32_t __reserved0 : 8;
        uint32_t outfromperi : 1;
        uint32_t outtopad    : 1;
        uint32_t __reserved1 : 2;
        uint32_t oefromperi  : 1;
        uint32_t oetopad     : 1;
        uint32_t __reserved2 : 3;
        uint32_t infrompad   : 1;
        uint32_t __reserved3 : 1;
        uint32_t intoperi    : 1;
        uint32_t __reserved4 : 4;
        uint32_t irqfrompad  : 1;
        uint32_t __reserved5 : 1;
        uint32_t irqtoproc   : 1;
        uint32_t __reserved6 : 5;

    };
    uint32_t _value;
} RP2040GpioStatus;

typedef union RP2040GpioControl {
    struct {
        uint32_t funcsel       : 5; /* type RW, reset 0x1f */
        uint32_t __reserved4   : 3;
        uint32_t outover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved3   : 2;
        uint32_t oeover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved2   : 2;
        uint32_t inover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved1   : 10; 
        uint32_t irqover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved0   : 2; 
    };
    uint32_t _value;
} RP2040GpioControl;

struct RP2040GpioState {
    SysBusDevice parent_obj;

    MemoryRegion container;
    MemoryRegion gpio_mmio;
    MemoryRegion qspi_mmio;
    qemu_irq     irq; 

    RP2040GpioStatus  gpio_status[RP2040_GPIO_PINS];
    RP2040GpioControl gpio_ctrl[RP2040_GPIO_PINS];
    qemu_irq          gpio_out[RP2040_GPIO_PINS];
    
    RP2040GpioStatus  qspi_status[RP2040_GPIO_QSPI_PINS];
    RP2040GpioControl qspi_ctrl[RP2040_GPIO_QSPI_PINS];
    qemu_irq          qspi_cs;
    qemu_irq          qspi_sclk;
    qemu_irq          qspi_out[RP2040_GPIO_QSPI_PINS - 2];
};

/* 
    GPIO Registers has aliases in memory to perform operations: 
    addr + 0x1000 -> XOR operation 
    addr + 0x2000 -> SET bitmask operation 
    addr + 0x3000 -> CLEAR bitmask operation
*/

#endif /* RP2040_GPIO_QSPI_H */ 
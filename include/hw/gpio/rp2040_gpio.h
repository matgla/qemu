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

#ifndef RP2040_GPIO_H
#define RP2040_GPIO_H

#include <stdbool.h>

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_GPIO "rp2040.gpio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040GpioState, RP2040_GPIO)

#define RP2040_GPIO_PINS      30
#define RP2040_GPIO_QSPI_PINS 6
#define RP2040_GPIO_QSPI_IO_PINS 4

typedef union RP2040GpioStatus {
    struct {
        uint32_t __reserved0:8;
        uint32_t outfromperi:1;
        uint32_t outtopad:1;
        uint32_t __reserved1:2;
        uint32_t oefromperi:1;
        uint32_t oetopad:1;
        uint32_t __reserved2:3;
        uint32_t infrompad:1;
        uint32_t __reserved3:1;
        uint32_t intoperi:1;
        uint32_t __reserved4:4;
        uint32_t irqfrompad:1;
        uint32_t __reserved5:1;
        uint32_t irqtoproc:1;
        uint32_t __reserved6:5;

    };
    uint32_t _value;
} RP2040GpioStatus;

typedef union RP2040GpioControl {
    struct {
        uint32_t funcsel:5; /* type RW, reset 0x1f */
        uint32_t __reserved4:3;
        uint32_t outover:2; /* type RW, reset 0x0 */
        uint32_t __reserved3:2;
        uint32_t oeover:2; /* type RW, reset 0x0 */
        uint32_t __reserved2:2;
        uint32_t inover:2; /* type RW, reset 0x0 */
        uint32_t __reserved1:10;
        uint32_t irqover:2; /* type RW, reset 0x0 */
        uint32_t __reserved0:2;
    };
    uint32_t _value;
} RP2040GpioControl;

struct RP2040GpioState {
    SysBusDevice parent_obj;

    MemoryRegion container;
    MemoryRegion gpio_mmio;
    MemoryRegion qspi_mmio;
    qemu_irq     irq;

    bool qspi_in_reset;
    bool qspi_reset_done;
    bool gpio_in_reset;
    bool gpio_reset_done;

    RP2040GpioStatus  gpio_status[RP2040_GPIO_PINS];
    RP2040GpioControl gpio_ctrl[RP2040_GPIO_PINS];
    qemu_irq          gpio_out[RP2040_GPIO_PINS];

    RP2040GpioStatus  qspi_status[RP2040_GPIO_QSPI_PINS];
    RP2040GpioControl qspi_ctrl[RP2040_GPIO_QSPI_PINS];
    qemu_irq          qspi_out[RP2040_GPIO_QSPI_PINS];
};

void rp2040_gpio_reset(RP2040GpioState *state, bool reset_state);
void rp2040_qspi_io_reset(RP2040GpioState *state, bool reset_state);
int rp2040_gpio_get_reset_state(RP2040GpioState *state);
int rp2040_qspi_io_get_reset_state(RP2040GpioState *state);
int rp2040_gpio_get_reset_done(RP2040GpioState *state);
int rp2040_qspi_io_get_reset_done(RP2040GpioState *state);

#endif /* RP2040_GPIO_H */

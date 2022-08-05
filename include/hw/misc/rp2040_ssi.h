/*
 * Raspberry RP2040 SSI Controller
 *
 * Copyright (C) 2022 Mateusz Stadnik
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


#ifndef RP2040_SSI_H
#define RP2040_SSI_H

#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "qom/object.h"
#include "hw/ptimer.h"

typedef union RP2040SSICtrlr0 
{
    uint32_t value;
    struct {
        uint32_t dfs : 4;
        uint32_t frf : 3;
        uint32_t scph : 1;
        uint32_t scpol : 1;
        uint32_t tmod : 2;
        uint32_t slv_oe : 1;
        uint32_t slr : 1;
        uint32_t cfs : 4;
        uint32_t dfs32 : 4;
        uint32_t spi_frf : 2;
        uint32_t _reserved : 1;
        uint32_t sste : 1;
    };
} RP2040SSICtrlr0;


struct RP2040SSIState;

#define TYPE_RP2040_SSI_FLASH "rp2040.ssi.flash"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040SSIFlash, RP2040_SSI_FLASH);

struct RP2040SSIFlash 
{
    SysBusDevice parent_obj;

    struct RP2040SSIState *controller;
    bool cs;

    MemoryRegion mmio;
};

#define TYPE_RP2040_SSI "rp2040.ssi"

OBJECT_DECLARE_SIMPLE_TYPE(RP2040SSIState, RP2040_SSI);

/* RP2040 SSI controller provides read-only flash controller for XIP execution */
struct RP2040SSIState 
{
    SysBusDevice parent_obj;

    MemoryRegion container;
    MemoryRegion mmio;
    MemoryRegion flash_mmio;
    MemoryRegion mmio_ctrl;
    SSIBus *spi;

    union RP2040SSICtrlr0 ctrlr0;
    uint32_t ctrlr1;
    uint32_t rx_fifo_count;
    uint32_t tx_fifo_count;

    AddressSpace flash_address_space;
    RP2040SSIFlash flash;

    qemu_irq cs_line;

    bool cs;

    uint32_t rx_buffer[32];
    uint32_t rx_buffer_write_index;
    uint32_t rx_buffer_read_index;

    uint32_t tx_buffer[32];
    uint32_t tx_buffer_write_index;
    uint32_t tx_buffer_read_index;


    Clock *clock;
    ptimer_state *ptimer;
};

#endif /* RP2040_SIO_H */
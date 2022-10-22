/*
 * RP2040 SSI
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

#ifndef RP2040_SSI_H
#define RP2040_SSI_H

#include <stdbool.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/ptimer.h"
#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "qemu/fifo32.h"
#include "qom/object.h"

#define TYPE_RP2040_SSI "rp2040.ssi"

OBJECT_DECLARE_SIMPLE_TYPE(Rp2040SsiState, RP2040_SSI)

union Rp2040SsiCtrlR0 {
    uint32_t value;
    struct {
        uint32_t dfs : 4; /* data frame size */
        uint32_t frf : 3; /* frame format */
        uint32_t scph : 1; /* serial clock phase */
        uint32_t scpol : 1; /* serial clock polarity */
        uint32_t tmod : 2; /* 0x0 - transmit and receive
                            * 0x1 - transmit only (except frf = 0)
                            * 0x2 - receive only (except frf = 0)
                            * 0x3 - eeprom read mode */
        uint32_t slv_oe : 1; /* slave output enable */
        uint32_t slr : 1; /* shift register loop (test mode) */
        uint32_t cfs : 4; /* control frame size in clocks */
        uint32_t dfs32 : 4; /* data frame size in 32b mode in clocks */
        uint32_t spi_frf : 2; /* 0x0 - 1-bit spi
                               * 0x1 - dual-SPI
                               * 0x2 - quad-SPI */
        uint32_t _reserved : 1;
        uint32_t sste : 1; /* slave select toggle enable */
    };
};
typedef union Rp2040SsiCtrlR0 Rp2040SsiCtrlR0;

union Rp2040SsiCtrlR1 {
    uint32_t value;
    struct {
        uint32_t ndf : 16; /* number of data frames */
        uint32_t _reserved : 16;
    };
};
typedef union Rp2040SsiCtrlR1 Rp2040SsiCtrlR1;

union Rp2040SsiSsiEnr {
    uint32_t value;
    struct {
        uint32_t ssi_en : 1; /* enable SSI */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiSsiEnr Rp2040SsiSsiEnr;

union Rp2040SsiMWCR {
    uint32_t value;
    struct {
        uint32_t mwmod : 1; /* Microwire transfer mode */
        uint32_t mdd : 1; /* Microwire control */
        uint32_t mhs : 1; /* Microwire handshaking */
        uint32_t _reserved : 29;
    };
};
typedef union Rp2040SsiMWCR Rp2040SsiMWCR;

union Rp2040SsiStatus {
    uint32_t value;
    struct {
        uint32_t busy:1;
        uint32_t tfnf:1;
        uint32_t tfe:1;
        uint32_t rfne:1;
        uint32_t rff:1;
        uint32_t txe:1;
        uint32_t dcol:1;
    };
};
typedef union Rp2040SsiStatus Rp2040SsiStatus;

union Rp2040SsiBaudr {
    uint32_t value;
    struct {
        uint32_t sckdv : 16; /* SSI clock divider */
        uint32_t _reserved : 16;
    };
};
typedef union Rp2040SsiBaudr Rp2040SsiBaudr;

union Rp2040SsiTxFtlr {
    uint32_t value;
    struct {
        uint32_t tft : 8; /* transmit FIFO threshold */
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiTxFtlr Rp2040SsiTxFtlr;

union Rp2040SsiRxFtlr {
    uint32_t value;
    struct {
        uint32_t rft : 8; /* receive FIFO threshold */
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiRxFtlr Rp2040SsiRxFtlr;

union Rp2040SsiTxFlr {
    uint32_t value;
    struct {
        uint32_t tftfl : 8; /* transmit FIFO level */
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiTxFlr Rp2040SsiTxFlr;

union Rp2040SsiRxFlr {
    uint32_t value;
    struct {
        uint32_t rxftl : 8; /* receive FIFO level */
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiRxFlr Rp2040SsiRxFlr;

union Rp2040SsiSR {
    uint32_t value;
    struct {
        uint32_t busy : 1; /* SSI busy flag */
        uint32_t tfnf : 1; /* transmit FIFO not full */
        uint32_t tfe : 1; /* transmit FIFO empty */
        uint32_t rfne : 1; /* receive FIFO not empty */
        uint32_t rff : 1; /* receive FIFO full */
        uint32_t txe : 1; /* transmission error */
        uint32_t dcol : 1; /* data collision error */
        uint32_t _reserved : 25;
    };
};
typedef union Rp2040SsiSR Rp2040SsiSR;

union Rp2040SsiIMR {
    uint32_t value;
    struct {
        uint32_t txeim : 1; /* transmit FIFO empty */
        uint32_t txoim : 1; /* transmit FIFO overflow */
        uint32_t rxuim : 1; /* receive FIFO underflow */
        uint32_t rxoim : 1; /* receive FIFO overflow */
        uint32_t rxfim : 1; /* receive FIFO full */
        uint32_t mstim : 1; /* multi-master contention */
        uint32_t _reserved : 26;
    };
};
typedef union Rp2040SsiIMR Rp2040SsiIMR;

union Rp2040SsiISR {
    uint32_t value;
    struct {
        uint32_t txeis : 1; /* transmit FIFO empty */
        uint32_t txois : 1; /* transmit FIFO overflow */
        uint32_t rxuis : 1; /* receive FIFO underflow */
        uint32_t rxois : 1; /* receive FIFO overflow */
        uint32_t rxfis : 1; /* receive FIFO full */
        uint32_t mstis : 1; /* multi-master contention */
        uint32_t _reserved : 26;
    };
};
typedef union Rp2040SsiISR Rp2040SsiISR;

union Rp2040SsiRISR {
    uint32_t value;
    struct {
        uint32_t txeir : 1; /* transmit FIFO empty */
        uint32_t txoir : 1; /* transmit FIFO overflow */
        uint32_t rxuir : 1; /* receive FIFO underflow */
        uint32_t rxoir : 1; /* receive FIFO overflow */
        uint32_t rxfir : 1; /* receive FIFO full */
        uint32_t mstir : 1; /* multi-master contention */
        uint32_t _reserved : 26;
    };
};
typedef union Rp2040SsiRISR Rp2040SsiRISR;

union Rp2040SsiTxOICR {
    uint32_t value;
    struct {
        uint32_t clr : 1; /* tx overflow interrupt clear on read */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiTxOICR Rp2040SsiTxOICR;

union Rp2040SsiRxOICR {
    uint32_t value;
    struct {
        uint32_t clr : 1; /* rx overflow interrupt clear on read */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiRxOICR Rp2040SsiRxOICR;

union Rp2040SsiRxUICR {
    uint32_t value;
    struct {
        uint32_t clr : 1; /* rx underflow interrupt clear on read */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiRxUICR Rp2040SsiRxUICR;

union Rp2040SsiMSTICR {
    uint32_t value;
    struct {
        uint32_t clr : 1; /* multi-master contention interrupt clear on read */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiMSTICR Rp2040SsiMSTICR;

union Rp2040SsiICR {
    uint32_t value;
    struct {
        uint32_t clr : 1; /* all interrupts clear on read */
        uint32_t _reserved : 31;
    };
};
typedef union Rp2040SsiICR Rp2040SsiICR;

union Rp2040SsiDMACR {
    uint32_t value;
    struct {
        uint32_t tdmae : 1; /* transmit DMA enable */
        uint32_t rdmae : 1; /* receive DMA enable */
        uint32_t _reserved : 30;
    };
};
typedef union Rp2040SsiDMACR Rp2040SsiDMACR;

union Rp2040SsiDMATDLR {
    uint32_t value;
    struct {
        uint32_t dmatdl : 8; /* transmit data watermark level */
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiDMATDLR Rp2040SsiDMATDLR;

union Rp2040SsiDMARDLR {
    uint32_t value;
    struct {
        uint32_t dmardl : 8; /* receive data watermark level (DMARDLR + 1)*/
    };
};
typedef union Rp2040SsiDMARDLR Rp2040SsiDMARDLR;

struct Rp2040SsiDR0 {
    uint32_t dr;
};
typedef struct Rp2040SsiDR0 Rp2040SsiDR0;

union Rp2040SsiRxSampleDly {
    uint32_t value;
    struct {
        uint32_t rsd : 8; /* rxd sample delay in SCLK cycles*/
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiRxSampleDly Rp2040SsiRxSampleDly;

union Rp2040SsiSpiCtrlR0 {
    uint32_t value;
    struct {
        uint32_t trans_type : 2; /* 0x0 - command and address in SPI
                                  * 0x1 - command in SPI, address by FRF
                                  * 0x2 - command and address in FRF */
        uint32_t addr_l : 4; /* address 0b-60b in 4b increments */
        uint32_t _reserved0 : 2;
        uint32_t inst_l : 2; /* 0x0 - no instruction
                              * 0x1 - 4-bit instruction
                              * 0x2 - 8-bit instruction
                              * 0x3 - 16-bit instruction */
        uint32_t _reserved1 : 1;
        uint32_t wait_cycles : 5; /* delay between control frame and data */
        uint32_t spi_ddr_en : 1; /* spi enable ddr */
        uint32_t inst_ddr_en : 1; /* instruction enable ddr */
        uint32_t spi_rxds_en : 1; /* read data strobe enable */
        uint32_t _reserved2 : 5;
        uint32_t xip_cmd : 8; /* spi command in xip or append to address
                              (inst_l = 0)*/
    };
};
typedef union Rp2040SsiSpiCtrlR0 Rp2040SsiSpiCtrlR0;

union Rp2040SsiTxdDriveEdge {
    uint32_t value;
    struct {
        uint32_t tde : 8; /* txd drive edge*/
        uint32_t _reserved : 24;
    };
};
typedef union Rp2040SsiTxdDriveEdge Rp2040SsiTxdDriveEdge;


typedef struct RP2040GpioState RP2040GpioState;

struct Rp2040SsiState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    qemu_irq cs;
    bool slave_selected;

    RP2040GpioState *gpio;
    SSIBus *bus;

    Fifo32 tx_fifo;
    Fifo32 rx_fifo;

    Rp2040SsiCtrlR0 ctrlr0;
    Rp2040SsiCtrlR1 ctrlr1;
    Rp2040SsiSsiEnr enr;
    Rp2040SsiMWCR mwcr;
    Rp2040SsiBaudr baudr;
    Rp2040SsiTxFtlr tx_ftlr;
    Rp2040SsiRxFtlr rx_ftlr;
    Rp2040SsiTxFlr tx_flr;
    Rp2040SsiRxFlr rx_flr;
    Rp2040SsiSR sr;
    Rp2040SsiIMR imr;
    Rp2040SsiISR isr;
    Rp2040SsiRISR risr;
    Rp2040SsiTxOICR txoicr;
    Rp2040SsiRxOICR rxoicr;
    Rp2040SsiRxUICR rxuicr;
    Rp2040SsiMSTICR msticr;
    Rp2040SsiICR icr;
    Rp2040SsiDMACR dmacr;
    Rp2040SsiDMATDLR dmatdlr;
    Rp2040SsiDMARDLR dmardlr;
    Rp2040SsiRxSampleDly rx_sample_dly;
    Rp2040SsiSpiCtrlR0 spi_ctrlr0;
    Rp2040SsiTxdDriveEdge txd_drive_edge;
    Rp2040SsiStatus status;
};

Rp2040SsiState* rp2040_ssi_create(void);

#endif /* RP2040_SSI_H */

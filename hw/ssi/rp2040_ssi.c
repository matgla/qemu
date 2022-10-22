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

#include "hw/ssi/rp2040_ssi.h"

#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qom/object.h"
#include "hw/clock.h"
#include "hw/qdev-clock.h"
#include "hw/irq.h"

#define rp2040_ssi_error(fmt, ...)                                      \
    qemu_log_mask(LOG_GUEST_ERROR, "%s: " fmt "\n", __func__, ## __VA_ARGS__)

#define RP2040_SSI_CTRLR0           0x00
#define RP2040_SSI_CTRLR1           0x04
#define RP2040_SSI_SSIENR           0x08
#define RP2040_SSI_MWCR             0x0c
#define RP2040_SSI_SER              0x10
#define RP2040_SSI_BAUDR            0x14
#define RP2040_SSI_TXFTLR           0x18
#define RP2040_SSI_RXFTLR           0x1c
#define RP2040_SSI_TXFLR            0x20
#define RP2040_SSI_RXFLR            0x24
#define RP2040_SSI_SR               0x28
#define RP2040_SSI_IMR              0x2c
#define RP2040_SSI_ISR              0x30
#define RP2040_SSI_RISR             0x34
#define RP2040_SSI_TXOICR           0x38
#define RP2040_SSI_RXOICR           0x3c
#define RP2040_SSI_RXUICR           0x40
#define RP2040_SSI_MSTICR           0x44
#define RP2040_SSI_ICR              0x48
#define RP2040_SSI_DMACR            0x4c
#define RP2040_SSI_DMATDLR          0x50
#define RP2040_SSI_DMARDLR          0x54
#define RP2040_SSI_IDR              0x58
#define RP2040_SSI_VERSION_ID       0x5c
#define RP2040_SSI_DR0              0x60
#define RP2040_SSI_DR35             0xec
#define RP2040_SSI_RX_SAMPLE_DLY    0xf0
#define RP2040_SSI_SPI_CTRLR0       0xf4
#define RP2040_SSI_TXD_DRIVE_EDGE   0xf8


static uint64_t rp2040_ssi_read(void *opaque, hwaddr offset, unsigned int size)
{
    Rp2040SsiState *s = RP2040_SSI(opaque);

    switch (offset)
    {
        case RP2040_SSI_CTRLR0:
            return (uint32_t)s->ctrlr0.value;
        case RP2040_SSI_CTRLR1:
            return (uint32_t)s->ctrlr1.value;
        case RP2040_SSI_SSIENR:
            return (uint32_t)s->enr.value;
        case RP2040_SSI_MWCR:
            return (uint32_t)s->mwcr.value;
        case RP2040_SSI_SER:
            return (uint32_t)s->slave_selected;
        case RP2040_SSI_BAUDR:
            return (uint32_t)s->baudr.value;
        case RP2040_SSI_TXFTLR:
            return (uint32_t)s->tx_ftlr.value;
        case RP2040_SSI_RXFTLR:
            return (uint32_t)s->rx_ftlr.value;
        case RP2040_SSI_SR:
            return (uint32_t)s->sr.value;
        case RP2040_SSI_IMR:
            return (uint32_t)s->imr.value;
        case RP2040_SSI_ISR:
            return (uint32_t)s->isr.value;
        case RP2040_SSI_RISR:
            return (uint32_t)s->risr.value;
        case RP2040_SSI_TXOICR:
            return (uint32_t)s->txoicr.value;
        case RP2040_SSI_RXOICR:
            return (uint32_t)s->rxoicr.value;
        case RP2040_SSI_RXUICR:
            return (uint32_t)s->rxuicr.value;
        case RP2040_SSI_MSTICR:
            return (uint32_t)s->msticr.value;
        case RP2040_SSI_ICR:
            return (uint32_t)s->icr.value;
        case RP2040_SSI_DMACR:
            return (uint32_t)s->dmacr.value;
        case RP2040_SSI_DMATDLR:
            return (uint32_t)s->dmatdlr.value;
        case RP2040_SSI_DMARDLR:
            return (uint32_t)s->dmardlr.value;
        case RP2040_SSI_IDR:
            return 0x51535049;
        case RP2040_SSI_VERSION_ID:
            return 0x3430312a;
        case RP2040_SSI_DR0 ... RP2040_SSI_DR35:
            if (fifo32_is_empty(&s->rx_fifo)) {
                return 0;
            }
            return fifo32_pop(&s->rx_fifo);
        case RP2040_SSI_RX_SAMPLE_DLY:
            return (uint32_t)s->rx_sample_dly.value;
        case RP2040_SSI_SPI_CTRLR0:
            return (uint32_t)s->spi_ctrlr0.value;
        case RP2040_SSI_TXD_DRIVE_EDGE:
            return (uint32_t)s->txd_drive_edge.value;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx\n", __func__,
        offset);

    return 0;
}


static void rp2040_ssi_transfer(void *opaque)
{
    Rp2040SsiState *s = RP2040_SSI(opaque);
    uint8_t byte = 0;
    uint32_t to_transfer = 0;

    if (!fifo32_is_empty(&s->tx_fifo)) {
        to_transfer = fifo32_pop(&s->tx_fifo);
    }

    if (fifo32_is_empty(&s->tx_fifo)) {
        s->status.tfe = true;
    }

    if (s->slave_selected)
    {
        qemu_set_irq(s->cs, 0);
    }

    byte = ssi_transfer(s->bus, to_transfer);

    if (s->slave_selected)
    {
        qemu_set_irq(s->cs, 1);
    }
    fifo32_push(&s->rx_fifo, byte);
}

static void rp2040_ssi_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{
    Rp2040SsiState *s = RP2040_SSI(opaque);

    switch (offset)
    {
        case RP2040_SSI_CTRLR0:
            s->ctrlr0.value = (uint32_t)value;
        return;
        case RP2040_SSI_CTRLR1:
            s->ctrlr1.value = (uint32_t)value;
        return;
        case RP2040_SSI_SSIENR:
            s->enr.value = (uint32_t)value;
        return;
        case RP2040_SSI_MWCR:
            s->mwcr.value = (uint32_t)value;
        return;
        case RP2040_SSI_SER:
            s->slave_selected = (uint32_t)value;
        return;
        case RP2040_SSI_BAUDR:
            s->baudr.value = (uint32_t)value;
        return;
        case RP2040_SSI_TXFTLR:
            s->tx_ftlr.value = (uint32_t)value;
        return;
        case RP2040_SSI_RXFTLR:
            s->rx_ftlr.value = (uint32_t)value;
        return;
        case RP2040_SSI_IMR:
            s->imr.value = (uint32_t)value;
        return;
        case RP2040_SSI_DMACR:
            s->dmacr.value = (uint32_t)value;
        return;
        case RP2040_SSI_DMATDLR:
            s->dmatdlr.value = (uint32_t)value;
        return;
        case RP2040_SSI_DMARDLR:
            s->dmardlr.value = (uint32_t)value;
        return;
        case RP2040_SSI_DR0 ... RP2040_SSI_DR35:
            if (fifo32_is_full(&s->tx_fifo)) {
                return;
            }

            fifo32_push(&s->tx_fifo, value);
            s->status.tfe = false;

            rp2040_ssi_transfer(opaque);
        return;
        case RP2040_SSI_RX_SAMPLE_DLY:
            s->rx_sample_dly.value = (uint32_t)value;
        return;
        case RP2040_SSI_SPI_CTRLR0:
            s->spi_ctrlr0.value = (uint32_t)value;
        return;
        case RP2040_SSI_TXD_DRIVE_EDGE:
            s->txd_drive_edge.value = (uint32_t)value;
        return;
        case RP2040_SSI_TXFLR:
        case RP2040_SSI_RXFLR:
        case RP2040_SSI_SR:
        case RP2040_SSI_ISR:
        case RP2040_SSI_RISR:
        case RP2040_SSI_TXOICR:
        case RP2040_SSI_RXOICR:
        case RP2040_SSI_RXUICR:
        case RP2040_SSI_MSTICR:
        case RP2040_SSI_ICR:
        case RP2040_SSI_IDR:
        case RP2040_SSI_VERSION_ID:
            qemu_log_mask(LOG_GUEST_ERROR, "%s: write to read only "
                          "register: 0x%03lx, write: 0x%lx\n",
                          __func__, offset, value);
        return;
    }

    qemu_log_mask(LOG_UNIMP, "%s: unimplemented register: 0x%03lx, write: 0x%lx\n",
         __func__, offset, value);
}

static const MemoryRegionOps rp2040_ssi_ops = {
    .read = rp2040_ssi_read,
    .write = rp2040_ssi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void rp2040_ssi_init(Object *obj)
{
    Rp2040SsiState *state = RP2040_SSI(obj);
    state->slave_selected = false;
    qdev_init_gpio_out_named(DEVICE(obj), &state->cs, "cs", 1);
}

static void rp2040_ssi_realize(DeviceState *dev, Error **errp)
{
    Rp2040SsiState *s = RP2040_SSI(dev);
    SysBusDevice *sys = SYS_BUS_DEVICE(dev);
    memory_region_init_io(&s->mmio, OBJECT(dev), &rp2040_ssi_ops, s, TYPE_RP2040_SSI, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);

    s->bus = ssi_create_bus(dev, "rp2040_xip_ssi_bus");

    fifo32_create(&s->rx_fifo, 32);
    fifo32_create(&s->tx_fifo, 32);
}

static void rp2040_ssi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = rp2040_ssi_realize;
}

static const TypeInfo rp2040_ssi_info = {
    .name = TYPE_RP2040_SSI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Rp2040SsiState),
    .instance_init = rp2040_ssi_init,
    .class_init = rp2040_ssi_class_init
};

static void rp2040_ssi_register_types(void)
{
    type_register_static(&rp2040_ssi_info);
}

type_init(rp2040_ssi_register_types)
#include "qemu/osdep.h"

#include "hw/misc/rp2040_ssi.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"
#include "hw/ssi/ssi.h"
#include "qapi/error.h"
#include "hw/irq.h"
#include "hw/clock.h"

#define rp2040_ssi_error(fmt, ...)                                      \
    qemu_log_mask(LOG_GUEST_ERROR, "%s: " fmt "\n", __func__, ## __VA_ARGS__)

static Property rp2040_ssi_flash_properties[] = {
    DEFINE_PROP_BOOL("cs", RP2040SSIFlash, cs, 0),
    DEFINE_PROP_LINK("controller", RP2040SSIFlash, controller, TYPE_RP2040_SSI, 
        RP2040SSIState *),
    DEFINE_PROP_END_OF_LIST(),
};


static void rp2040_ssi_flash_realize(DeviceState *dev, Error **errp)
{
    RP2040SSIFlash *s = RP2040_SSI_FLASH(dev);

    if (!s->controller) {
        error_setg(errp, TYPE_RP2040_SSI_FLASH ": 'controller' not connected");
        return;
    }

    qemu_set_irq(s->controller->cs_line, false);

}

static void rp2040_ssi_flash_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 SSI flash device";
    dc->realize = rp2040_ssi_flash_realize;
    device_class_set_props(dc, rp2040_ssi_flash_properties);
}

static const TypeInfo rp2040_ssi_flash_info = {
    .name           = TYPE_RP2040_SSI_FLASH,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(RP2040SSIFlash),
    .class_init     = rp2040_ssi_flash_class_init,
};

static uint64_t rp2040_ssi_flash_read(void *opaque, hwaddr addr, unsigned size)
{
    RP2040SSIFlash *flash = opaque;
    RP2040SSIState *ssi = flash->controller;

    fprintf(stderr, "Flash read from address: %lx\n", addr);
    return ssi_transfer(ssi->spi, 0);
}

static void rp2040_ssi_flash_write(void *opaque, hwaddr addr, uint64_t data, 
    unsigned size)
{
    fprintf(stderr, "Flash write to address: %lx\n", addr);
}

static const MemoryRegionOps rp2040_ssi_flash_ops = {
    .read = rp2040_ssi_flash_read, 
    .write = rp2040_ssi_flash_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

#define RP2040_SSI_CTRLR0 0x00
#define RP2040_SSI_CTRLR1 0x04
#define RP2040_SSI_TXFTLR 0x18
#define RP2040_SSI_RXFTLR 0x1c
#define RP2040_SSI_TXFLR  0x20
#define RP2040_SSI_RXFLR  0x24
#define RP2040_SSI_DR0    0x60
#define RP2040_SSI_SSIENR 0x08

static uint64_t ssi_read_ctrl(void *opaque, hwaddr offset, unsigned int size)
{
    return 0;
}
static uint64_t ssi_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040SSIState *s = RP2040_SSI(opaque);

    fprintf(stderr, "SSI read from address: %lx with size %d\n", offset, size);
    switch (offset)
    {
        case RP2040_SSI_CTRLR0:
        {
            return (uint32_t)s->ctrlr0.value; 
        }
        case RP2040_SSI_CTRLR1:
        {
            return (uint32_t)s->ctrlr1;
        }
        case RP2040_SSI_TXFTLR:
        {
            return 0; 
        }
        case RP2040_SSI_RXFTLR:
        {
            return 0;
        }
        case RP2040_SSI_TXFLR:
        {
            return 0; 
        }
        case RP2040_SSI_RXFLR:
        {
            return (uint32_t)s->rx_fifo_count;
        }
        case RP2040_SSI_DR0:
        {
            uint8_t to_return = s->rx_buffer[s->rx_buffer_read_index];
            fprintf(stderr, "Read from %d, v: %x\n", s->rx_buffer_read_index, to_return);
            ++s->rx_buffer_read_index;
            if (s->rx_buffer_read_index == 16)
            {
                s->rx_buffer_read_index = 0;
            }
            --s->rx_fifo_count;
            return to_return;
        }
    }
    return 0xffffffff;
}

static void print_ctrlr0(RP2040SSICtrlr0 *c)
{
    fprintf(stderr, "SSI_CTRLR0 status: {\n");
    fprintf(stderr, "                       DataFrameSize (DFS): %d\n", c->dfs);
    fprintf(stderr, "                       FrameFormat (FRF)  : %d\n", c->frf);
    fprintf(stderr, "                       SerialPolarityPhase (scph) : %d\n", c->scph);
    fprintf(stderr, "                       SerialPolarityClock (scpol): %d\n", c->scpol);
}

static void rp2040_ssi_transfer(void *opaque) 
{
    RP2040SSIState *s = RP2040_SSI(opaque);
    fprintf(stderr, "Transfer ----------------------, cs: %d %d\n", s->cs, s->tx_buffer_read_index);
    if (s->rx_buffer_write_index == 16)
    {
        s->rx_buffer_write_index = 0; // raise interrupt and status
    }
    uint8_t byte = 0;
    if (!s->cs)
    {
        fprintf(stderr, "Read from tx buffer pos(%d): %d\n",s->tx_buffer_read_index,  s->tx_buffer[s->tx_buffer_read_index]);
        byte = ssi_transfer(s->spi, s->tx_buffer[s->tx_buffer_read_index]);
    }
    s->rx_buffer[s->rx_buffer_write_index] = byte;
    ++s->rx_buffer_write_index;
    ++s->rx_fifo_count;

    ++s->tx_buffer_read_index;
    if (s->tx_buffer_read_index == 16)
    {
        s->tx_buffer_read_index = 0;
    }
    --s->tx_fifo_count;
}

static void ssi_write_ctrl(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{
}

static void ssi_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{
    RP2040SSIState *s = RP2040_SSI(opaque);
    
    fprintf(stderr, "SSI writes to: %lx, value: %lx\n", offset, value);

    switch (offset)
    {
        case RP2040_SSI_CTRLR0:
        {
            s->ctrlr0.value = (uint32_t)value; 
            print_ctrlr0(&s->ctrlr0);
            return;
        }
        case RP2040_SSI_CTRLR1:
        {
            s->ctrlr1 = (uint32_t)value; 
            return;
        }
        case RP2040_SSI_DR0:
        {   
            if (s->tx_buffer_write_index == 16)
            {
                s->tx_buffer_write_index = 0; // raise interrupt and status
            }
            fprintf(stderr, "Write to tx buffer pos(%d): %ld\n",s->tx_buffer_write_index,  value);
            s->tx_buffer[s->tx_buffer_write_index] = value;
            ++s->tx_buffer_write_index;
            ++s->tx_fifo_count;
            rp2040_ssi_transfer(opaque);
            // ptimer_transaction_begin(s->ptimer);
            // ptimer_run(s->ptimer, 0);
            // ptimer_transaction_commit(s->ptimer);
            return;
        }
        case RP2040_SSI_SSIENR:
        {
            if (value == 0x00)
            {
                s->rx_buffer_write_index = 0;
                s->rx_buffer_read_index = 0;
                s->rx_fifo_count = 0;
            }
        }

    }
}

static const MemoryRegionOps ssi_ops = {
    .read = ssi_read, 
    .write = ssi_write, 
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};


static const MemoryRegionOps ssi_ctrl_ops = {
    .read = ssi_read_ctrl, 
    .write = ssi_write_ctrl, 
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void rp2040_ssi_init(Object *obj)
{
    RP2040SSIState *s = RP2040_SSI(obj); 
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init(&s->container, obj, TYPE_RP2040_SSI, 0x10000000);
    sysbus_init_mmio(sys, &s->container);
    


    memory_region_init_io(&s->mmio, obj, &ssi_ops, s, TYPE_RP2040_SSI, 0x10000000);
    sysbus_init_mmio(sys, &s->mmio);

    fprintf(stderr, "Initialize flash object\n");
    object_initialize_child(obj, "flash", &s->flash, TYPE_RP2040_SSI_FLASH);

    s->clock = qdev_init_clock_in(DEVICE(obj), "clock", NULL, NULL, 0);
    clock_set_hz(s->clock, 100000UL);

    s->ptimer = ptimer_init(rp2040_ssi_transfer, s,
                            PTIMER_POLICY_WRAP_AFTER_ONE_PERIOD |
                            PTIMER_POLICY_NO_COUNTER_ROUND_DOWN |
                            PTIMER_POLICY_NO_IMMEDIATE_RELOAD);

    ptimer_transaction_begin(s->ptimer);
    ptimer_set_period_from_clock(s->ptimer, s->clock, 1);
    ptimer_transaction_commit(s->ptimer);

    qemu_set_irq(s->cs_line, false);
    qemu_set_irq(s->cs_line, true);
    qemu_set_irq(s->cs_line, false);
    qemu_set_irq(s->cs_line, true);
    qemu_set_irq(s->cs_line, false);

}

static void rp2040_ssi_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    RP2040SSIState *s = RP2040_SSI(dev);
    RP2040SSIFlash *flash = &s->flash;
    
    s->spi = ssi_create_bus(dev, "rp2040_xip_ssi");

    /* Initialize SSI register space */
    memory_region_init_io(&s->mmio, OBJECT(s), &ssi_ops, s, TYPE_RP2040_SSI, 0x4000);
    sysbus_init_mmio(sbd, &s->mmio);

    memory_region_init_io(&s->flash_mmio, OBJECT(s), &rp2040_ssi_flash_ops, s, 
        TYPE_RP2040_SSI_FLASH, 16 * 1024 * 1024);

    memory_region_init_io(&s->mmio_ctrl, OBJECT(s), &ssi_ctrl_ops, s, TYPE_RP2040_SSI, 0x4000);

    object_property_set_link(OBJECT(flash), "controller", OBJECT(s), errp);

    sysbus_realize(SYS_BUS_DEVICE(flash), errp);

    memory_region_add_subregion(&s->container, 0x8000000, &s->mmio);
    memory_region_add_subregion(&s->container, 0x4000000, &s->mmio_ctrl);
    memory_region_add_subregion(&s->container, 0x000000000, &flash->mmio);

    /* Interrupts initialization */

    sysbus_init_irq(sbd, &s->cs_line); 
    fprintf(stderr, "Set cs line\n");
}

static void rp2040_ssi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = rp2040_ssi_realize;
}

static const TypeInfo rp2040_ssi_info = {
    .name = TYPE_RP2040_SSI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040SSIState),
    .instance_init = rp2040_ssi_init,
    .class_init = rp2040_ssi_class_init
};

static void rp2040_ssi_register_types(void)
{
    type_register_static(&rp2040_ssi_flash_info);
    type_register_static(&rp2040_ssi_info);

}

type_init(rp2040_ssi_register_types)

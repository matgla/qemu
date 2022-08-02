#include "qemu/osdep.h"

#include "hw/misc/rp2040_ssi.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"
#include "hw/ssi/ssi.h"
#include "qapi/error.h"

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
            --s->rx_fifo_count;
            return 0;
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
            ++s->rx_fifo_count;
            return;
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

static void rp2040_ssi_init(Object *obj)
{
    RP2040SSIState *s = RP2040_SSI(obj);
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init_io(&s->container, obj, &ssi_ops, s, TYPE_RP2040_SSI, 0x000);
    sysbus_init_mmio(sys, &s->mmio);
    
    memory_region_init_io(&s->mmio, obj, &ssi_ops, s, TYPE_RP2040_SSI, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);

    fprintf(stderr, "Initialize flash object\n");
    object_initialize_child(obj, "flash", &s->flash, TYPE_RP2040_SSI_FLASH);
}

static void rp2040_ssi_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    RP2040SSIState *s = RP2040_SSI(dev);
    RP2040SSIFlash *flash = &s->flash;
    
    s->spi = ssi_create_bus(dev, NULL);

    /* Initialize SSI register space */
    memory_region_init_io(&s->mmio, OBJECT(s), &ssi_ops, s, TYPE_RP2040_SSI, 0x4000);
    sysbus_init_mmio(sbd, &s->mmio);

    memory_region_init_io(&s->flash_mmio, OBJECT(s), &rp2040_ssi_flash_ops, s, 
        TYPE_RP2040_SSI_FLASH, 16 * 1024 * 1024);

    object_property_set_link(OBJECT(flash), "controller", OBJECT(s), errp);

    sysbus_realize(SYS_BUS_DEVICE(flash), errp);

    memory_region_add_subregion(&s->mmio_flash, 0, &flash->mmio);

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

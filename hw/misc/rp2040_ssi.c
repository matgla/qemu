#include "qemu/osdep.h"

#include "hw/misc/rp2040_ssi.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"

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

    fprintf(stderr, "Read from address: %lx with size %d\n", offset, size);
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
    memory_region_init_io(&s->mmio, obj, &ssi_ops, s, TYPE_RP2040_SSI, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);
}



static void rp2040_ssi_class_init(ObjectClass *klass, void *data)
{
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
    type_register_static(&rp2040_ssi_info);
}

type_init(rp2040_ssi_register_types);
#include "qemu/osdep.h"

#include "hw/misc/rp2040_vreg.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"

#define VREG_VREG       0x0
#define VREG_BOD        0x4
#define VREG_CHIP_RESET 0x8

static uint64_t vreg_read(void *opaque, hwaddr offset, unsigned int size)
{
    fprintf(stderr, "Read from address: %lx with size %d\n", offset, size);
    switch (offset)
    {
        case VREG_VREG: 
        {

        } break;
        case VREG_BOD: 
        {

        } break;
        case VREG_CHIP_RESET: 
        {
            return 0x00000000;
        } break;
    }
    return 0;
}

static void vreg_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{

}

static const MemoryRegionOps vreg_ops = {
    .read = vreg_read, 
    .write = vreg_write, 
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void rp2040_vreg_init(Object *obj)
{
    RP2040VregState *s = RP2040_VREG(obj);
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init_io(&s->mmio, obj, &vreg_ops, s, TYPE_RP2040_VREG, 0x100);
    sysbus_init_mmio(sys, &s->mmio);
}



static void rp2040_vreg_class_init(ObjectClass *klass, void *data)
{
}

static const TypeInfo rp2040_vreg_info = {
    .name = TYPE_RP2040_VREG,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040VregState),
    .instance_init = rp2040_vreg_init,
    .class_init = rp2040_vreg_class_init
};

static void rp2040_vreg_register_types(void)
{
    type_register_static(&rp2040_vreg_info);
}

type_init(rp2040_vreg_register_types);
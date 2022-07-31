#include "qemu/osdep.h"

#include "hw/misc/rp2040_resets.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"

#define RESETS_RESET      0x0
#define RESETS_WDSEL      0x4
#define RESETS_RESET_DONE 0x8

static uint64_t resets_read(void *opaque, hwaddr offset, unsigned int size)
{
    fprintf(stderr, "Read from address: %lx with size %d\n", offset, size);
    switch (offset)
    {
        case RESETS_RESET: 
        {

        } break;
        case RESETS_WDSEL: 
        {

        } break;
        case RESETS_RESET_DONE: 
        {
            return 0xffffffff;
        } break;
    }
    return 0;
}

static void resets_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{

}

static const MemoryRegionOps resets_ops = {
    .read = resets_read, 
    .write = resets_write, 
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void rp2040_resets_init(Object *obj)
{
    RP2040ResetsState *s = RP2040_RESETS(obj);
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init_io(&s->mmio, obj, &resets_ops, s, TYPE_RP2040_RESETS, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);
}



static void rp2040_resets_class_init(ObjectClass *klass, void *data)
{
}

static const TypeInfo rp2040_resets_info = {
    .name = TYPE_RP2040_RESETS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040ResetsState),
    .instance_init = rp2040_resets_init,
    .class_init = rp2040_resets_class_init
};

static void rp2040_resets_register_types(void)
{
    type_register_static(&rp2040_resets_info);
}

type_init(rp2040_resets_register_types);
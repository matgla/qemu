#include "qemu/osdep.h"

#include "hw/misc/rp2040_xosc.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"

static uint64_t xosc_read(void *opaque, hwaddr offset, unsigned int size)
{
    fprintf(stderr, "Read from address: %lx with size %d\n", offset, size);
    return 0xffffffff;
}

static void xosc_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size)
{

}

static const MemoryRegionOps xosc_ops = {
    .read = xosc_read, 
    .write = xosc_write, 
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void rp2040_xosc_init(Object *obj)
{
    RP2040XOSCState *s = RP2040_XOSC(obj);
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init_io(&s->mmio, obj, &xosc_ops, s, TYPE_RP2040_XOSC, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);
}



static void rp2040_xosc_class_init(ObjectClass *klass, void *data)
{
}

static const TypeInfo rp2040_xosc_info = {
    .name = TYPE_RP2040_XOSC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040XOSCState),
    .instance_init = rp2040_xosc_init,
    .class_init = rp2040_xosc_class_init
};

static void rp2040_xosc_register_types(void)
{
    type_register_static(&rp2040_xosc_info);
}

type_init(rp2040_xosc_register_types);
#include "qemu/osdep.h"

#include "hw/misc/rp2040_sio.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/guest-random.h"

#define RP2040_SIO_CPUID 0x000
#define RP2040_GPIO_IN 0x004
#define RP2040_GPIO_HI_IN 0x008
#define RP2040_GPIO_OUT 0x010
#define RP2040_GPIO_OUT_SET 0x014
#define RP2040_GPIO_OUT_CLR 0x018
#define RP2040_GPIO_OUT_XOR 0x01c
#define RP2040_GPIO_OE 0x020
#define RP2040_GPIO_OE_SET 0x024
#define RP2040_GPIO_OE_CRL 0x028
#define RP2040_GPIO_OE_XOR 0x02c
#define RP2040_GPIO_HI_OUT 0x030
#define RP2040_GPIO_HI_OUT_SET 0x034
#define RP2040_GPIO_HI_OUT_CLR 0x038
#define RP2040_GPIO_HI_OUT_XOR 0x03c
#define RP2040_GPIO_HI_OE 0x040
#define RP2040_GPIO_HI_OE_SET 0x044
#define RP2040_GPIO_HI_OE_CLR 0x048
#define RP2040_GPIO_HI_OE_XOR 0x04c
#define RP2040_FIFO_ST 0x050
#define RP2040_FIFO_WR 0x054
#define RP2040_FIFO_RD 0x058
#define RP2040_SPINLOCK_ST 0x05c
#define RP2040_DIV_UDIVIDEND 0x060
#define RP2040_DIV_UDIVISOR 0x064
#define RP2040_DIV_SDIVIDEND 0x068
#define RP2040_DIV_SDIVISOR 0x06c
#define RP2040_DIV_QUOTIENT 0x070
#define RP2040_DIV_REMAINDER 0x074
#define RP2040_DIV_CSR 0x078
#define RP2040_INTERP0_ACCUM0 0x080
#define RP2040_INTERP0_ACCUM1 0x084
#define RP2040_INTERP0_BASE0 0x088
#define RP2040_INTERP0_BASE1 0x08c
#define RP2040_INTERP0_BASE2 0x090
#define RP2040_INTERP0_POP_LANE0 0x094
#define RP2040_INTERP0_POP_LANE1 0x098
#define RP2040_INTERP0_POP_FULL 0x09c
#define RP2040_INTERP0_PEEK_LANE0 0x0a0
#define RP2040_INTERP0_PEEK_LANE1 0x0a4
#define RP2040_INTERP0_PEEK_FULL 0x0a8
#define RP2040_INTERP0_CTRL_LANE0 0x0ac
#define RP2040_INTERP0_CTRL_LANE1 0x0b0
#define RP2040_INTERP0_ACCUM0_ADD 0x0b4
#define RP2040_INTERP0_ACCUM1_ADD 0x0b8
#define RP2040_INTERP0_BASE_1AND0 0x0bc
#define RP2040_INTERP1_ACCUM0 0x0c0
#define RP2040_INTERP1_ACCUM1 0x0c4
#define RP2040_INTERP1_BASE0 0x0c8
#define RP2040_INTERP1_BASE1 0x0cc
#define RP2040_INTERP1_BASE2 0x0d0
#define RP2040_INTERP1_POP_LANE0 0x0d4
#define RP2040_INTERP1_POP_LANE1 0x0d8
#define RP2040_INTERP1_POP_FULL 0x0dc
#define RP2040_INTERP1_PEEK_LANE0 0x0e0
#define RP2040_INTERP1__PEEK_LANE1 0x0e4
#define RP2040_INTERP1_PEEK_FULL 0x0e8
#define RP2040_INTERP1_CTRL_LANE0 0x0ec
#define RP2040_INTERP1_CTRL_LANE1 0x0f0
#define RP2040_INTERP1_ACCUM0_ADD 0x0f4
#define RP2040_INTERP1_ACCUM1_ADD 0x0f8
#define RP2040_INTERP1_BASE_1AND0 0x0fc
#define RP2040_SPINLOCK0 0x100
#define RP2040_SPINLOCK31 0x17c

static uint64_t sio_read(void *opaque, hwaddr offset, unsigned int size)
{
    fprintf(stderr, "Read from address: %lx with size %d\n", offset, size);
    switch (offset)
    {

    case RP2040_SIO_CPUID:
    {
        return 0; // TODO: return correct cpu id
    }

    case RP2040_GPIO_IN:
    {
    }

    case RP2040_GPIO_HI_IN:
    {
        return 0xffffffff;
    }

    case RP2040_GPIO_OUT:
    {
    }

    case RP2040_GPIO_OUT_SET:
    {
    }

    case RP2040_GPIO_OUT_CLR:
    {
    }

    case RP2040_GPIO_OUT_XOR:
    {
    }

    case RP2040_GPIO_OE:
    {
    }

    case RP2040_GPIO_OE_SET:
    {
    }

    case RP2040_GPIO_OE_CRL:
    {
    }

    case RP2040_GPIO_OE_XOR:
    {
    }

    case RP2040_GPIO_HI_OUT:
    {
    }

    case RP2040_GPIO_HI_OUT_SET:
    {
    }

    case RP2040_GPIO_HI_OUT_CLR:
    {
    }

    case RP2040_GPIO_HI_OUT_XOR:
    {
    }

    case RP2040_GPIO_HI_OE:
    {
    }

    case RP2040_GPIO_HI_OE_SET:
    {
    }

    case RP2040_GPIO_HI_OE_CLR:
    {
    }

    case RP2040_GPIO_HI_OE_XOR:
    {
    }

    case RP2040_FIFO_ST:
    {
        return 0;
    }

    case RP2040_FIFO_WR:
    {
        return 0;
    }

    case RP2040_FIFO_RD:
    {
        return 0;
    }

    case RP2040_SPINLOCK_ST:
    {
    }

    case RP2040_DIV_UDIVIDEND:
    {
    }

    case RP2040_DIV_UDIVISOR:
    {
    }

    case RP2040_DIV_SDIVIDEND:
    {
    }

    case RP2040_DIV_SDIVISOR:
    {
    }

    case RP2040_DIV_QUOTIENT:
    {
    }

    case RP2040_DIV_REMAINDER:
    {
    }

    case RP2040_DIV_CSR:
    {
    }

    case RP2040_INTERP0_ACCUM0:
    {
    }

    case RP2040_INTERP0_ACCUM1:
    {
    }

    case RP2040_INTERP0_BASE0:
    {
    }

    case RP2040_INTERP0_BASE1:
    {
    }

    case RP2040_INTERP0_BASE2:
    {
    }

    case RP2040_INTERP0_POP_LANE0:
    {
    }

    case RP2040_INTERP0_POP_LANE1:
    {
    }

    case RP2040_INTERP0_POP_FULL:
    {
    }

    case RP2040_INTERP0_PEEK_LANE0:
    {
    }

    case RP2040_INTERP0_PEEK_LANE1:
    {
    }

    case RP2040_INTERP0_PEEK_FULL:
    {
    }

    case RP2040_INTERP0_CTRL_LANE0:
    {
    }

    case RP2040_INTERP0_CTRL_LANE1:
    {
    }

    case RP2040_INTERP0_ACCUM0_ADD:
    {
    }

    case RP2040_INTERP0_ACCUM1_ADD:
    {
    }

    case RP2040_INTERP0_BASE_1AND0:
    {
    }

    case RP2040_INTERP1_ACCUM0:
    {
    }

    case RP2040_INTERP1_ACCUM1:
    {
    }

    case RP2040_INTERP1_BASE0:
    {
    }

    case RP2040_INTERP1_BASE1:
    {
    }

    case RP2040_INTERP1_BASE2:
    {
    }

    case RP2040_INTERP1_POP_LANE0:
    {
    }

    case RP2040_INTERP1_POP_LANE1:
    {
    }

    case RP2040_INTERP1_POP_FULL:
    {
    }

    case RP2040_INTERP1_PEEK_LANE0:
    {
    }

    case RP2040_INTERP1__PEEK_LANE1:
    {
    }
    case RP2040_INTERP1_PEEK_FULL:
    {
    }

    case RP2040_INTERP1_CTRL_LANE0:
    {
    }

    case RP2040_INTERP1_CTRL_LANE1:
    {
    }

    case RP2040_INTERP1_ACCUM0_ADD:
    {
    }

    case RP2040_INTERP1_ACCUM1_ADD:
    {
    }

    case RP2040_INTERP1_BASE_1AND0:
    {
    }
    }
    if (offset >= RP2040_SPINLOCK0 || offset <= RP2040_SPINLOCK31)
    {
        fprintf(stderr, "SpinLock not implemented\n");
    }

    return 0xffffffff;
}

static void sio_write(void *opaque, hwaddr offset,
                      uint64_t value, unsigned int size)
{
}

static const MemoryRegionOps sio_ops = {
    .read = sio_read,
    .write = sio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4};

static void rp2040_sio_init(Object *obj)
{
    RP2040SIOState *s = RP2040_SIO(obj);
    SysBusDevice *sys = SYS_BUS_DEVICE(obj);

    // TODO: check register size, or maybe calculate after full implementation
    memory_region_init_io(&s->mmio, obj, &sio_ops, s, TYPE_RP2040_SIO, 0x4000);
    sysbus_init_mmio(sys, &s->mmio);
}

static void rp2040_sio_class_init(ObjectClass *klass, void *data)
{
}

static const TypeInfo rp2040_sio_info = {
    .name = TYPE_RP2040_SIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040SIOState),
    .instance_init = rp2040_sio_init,
    .class_init = rp2040_sio_class_init};

static void rp2040_sio_register_types(void)
{
    type_register_static(&rp2040_sio_info);
}

type_init(rp2040_sio_register_types);
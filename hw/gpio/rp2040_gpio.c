#include "qemu/osdep.h"
#include "hw/irq.h"
#include "trace.h"

#include "hw/gpio/rp2040_gpio.h"


typedef union RP2040GpioStatus {
    struct {
        uint32_t __reserved6   : 8;
        uint32_t out_from_peri : 1; /* type RO, reset 0x0 */
        uint32_t out_to_pad    : 1; /* type RO, reset 0x0 */
        uint32_t __reserved5   : 2;
        uint32_t oe_from_peri  : 1; /* type RO, reset 0x0 */
        uint32_t oe_to_pad     : 1; /* type RO, reset 0x0 */
        uint32_t __reserved4   : 4;
        uint32_t in_from_pad   : 1; /* type RO, reset 0x0 */
        uint32_t __reserved3   : 1;
        uint32_t in_to_peri    : 1; /* type RO, reset 0x0 */
        uint32_t __reserved2   : 4;
        uint32_t irq_from_pad  : 1; /* type RO, reset 0x0 */
        uint32_t __reserved1   : 1; 
        uint32_t irq_to_proc   : 1; /* type RO, reset 0x0 */
        uint32_t __reserved0   : 4; 
    };
    uint32_t value;
} RP2040GpioStatus;

typedef union RP2040GpioControl {
    struct {
        uint32_t funcsel       : 5; /* type RW, reset 0x1f */
        uint32_t __reserved4   : 3;
        uint32_t outover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved3   : 2;
        uint32_t oeover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved2   : 2;
        uint32_t inover        : 2; /* type RW, reset 0x0 */
        uint32_t __reserved1   : 10; 
        uint32_t irqover       : 2; /* type RW, reset 0x0 */
        uint32_t __reserved0   : 2; 
    };
    uint32_t value;
} RP2040GpioControl;

static void rp2040_gpio_qspi_write(void *opaque, hwaddr offset, 
    uint64_t value, unsigned int size)
{
    RP2040GpioQspiState *s = RP2040_GPIO_QSPI(opaque);
    fprintf(stderr, "Write to GPIO QSPI section offset: %lx, value: %lx\n", offset, value);

    switch (offset)
    {
        case 0x00:
        case 0x08:
        case 0x10:
        case 0x18:
        case 0x20:
        case 0x28:
        {
            fprintf(stderr, "Warning: try to write RO register\n");
            return;
        }
        case 0x04:
        case 0x0c:
        case 0x14:
        case 0x1c:
        case 0x24:
        case 0x2c:
        {
            hwaddr i = offset >> 3;
            RP2040GpioControl ctrl;
            ctrl.value = value;
            fprintf(stderr, "Writing to control register of: %ld\n", i);
            fprintf(stderr, "Ctrl {.irqover = %d, .inover = %d, .oeover = %d, .outver = %d, funcsel = %d}\n",
                ctrl.irqover, ctrl.inover, ctrl.oeover, ctrl.outover, ctrl.funcsel);
            
            if ( ctrl.outover == 0x03)
            {
                qemu_set_irq(s->out[i], 1);
            }
            else if (ctrl.outover == 0x02) 
            {
                fprintf(stderr, "Tu nie dziala\n");
                qemu_set_irq(s->out[i], 0);
            }

            return;
        }
        
    }
}

static uint64_t rp2040_gpio_qspi_read(void *opaque, hwaddr offset, unsigned int size)
{
    fprintf(stderr, "Read from GPIO QSPI section offset: %lx\n", offset);
    switch (offset)
    {
        case 0x00:
        case 0x08:
        case 0x10:
        case 0x18:
        case 0x20:
        case 0x28:
        {
            fprintf(stderr, "Warning: try to write RO register\n");
            return 0;
        }
        case 0x04:
        case 0x0c:
        case 0x14:
        case 0x1c:
        case 0x24:
        case 0x2c:
        {
            return 0;
        }
    }
    return 0;
}

static const MemoryRegionOps rp2040_gpio_qspi_io = {
    .read = rp2040_gpio_qspi_read,
    .write = rp2040_gpio_qspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_gpio_qspi_set(void *opaque, int line, int value)
{
}


static void rp2040_gpio_qspi_instance_init(Object *obj)
{
    RP2040GpioQspiState *s = RP2040_GPIO_QSPI(obj);

    memory_region_init_io(&s->mmio, obj, &rp2040_gpio_qspi_io, s,
        TYPE_RP2040_GPIO_QSPI, 0x54);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    qdev_init_gpio_in(DEVICE(s), rp2040_gpio_qspi_set, 5);
    qdev_init_gpio_out(DEVICE(s), s->out, 5);


}

static void rp2040_gpio_qspi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 GPIO QSPI";
}

static const TypeInfo rp2040_gpio_qspi_info = {
    .name = TYPE_RP2040_GPIO_QSPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040GpioQspiState),
    .instance_init = rp2040_gpio_qspi_instance_init, 
    .class_init = rp2040_gpio_qspi_class_init,
};

static void rp2040_gpio_qspi_register_types(void)
{
    type_register_static(&rp2040_gpio_qspi_info);
}

type_init(rp2040_gpio_qspi_register_types)

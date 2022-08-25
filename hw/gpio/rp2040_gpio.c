#include "qemu/osdep.h"
#include "hw/irq.h"
#include "trace.h"
#include "qemu/log.h"

#include "hw/gpio/rp2040_gpio.h"

#define RP2040_IO_REGISTER_SIZE         0x8000
#define RP2040_GPIO_REGISTER_SIZE       0x190
#define RP2040_GPIO_QSPI_REGISTER_SIZE  0x58


typedef struct Rp2040GpioRegisterDesc
{
    uint32_t state_and_control_end;
    RP2040GpioStatus *status;
    RP2040GpioControl *control;
    qemu_irq *irq;
} Rp2040GpioRegisterDesc;

static uint32_t rp2040_process_gpio_read(hwaddr offset, const Rp2040GpioRegisterDesc* desc, const char *name)
{
    if (offset <= desc->state_and_control_end) /* Process status & control registers */
    {
        const hwaddr index = offset >> 2;
        const bool is_stat = (index % 2) == 0;
        if (is_stat)
        {
            return desc->status[index >> 1]._value;
        }
        else 
        {
            return desc->control[index >> 1]._value;
        }
    }

    qemu_log_mask(LOG_UNIMP, "GPIO(%s) register at offset: 0x%" HWADDR_PRIX " not implemented.\n", name, offset);
    return 0;
}

typedef enum {
    RP2040_NORMAL_ACCESS,
    RP2040_XOR_ON_WRITE,
    RP2040_SET_ON_WRITE,
    RP2040_CLEAN_ON_WRITE,
} RP2040AccessType;

static RP2040AccessType rp2040_get_access_type(const hwaddr addr)
{
    if ((addr & 0x3000) == 0x0000)
    {
        return RP2040_NORMAL_ACCESS;
    }
    
    if (addr & 0x1000) 
    {
        return RP2040_XOR_ON_WRITE;
    }
    
    if (addr & 0x2000)
    {
        return RP2040_SET_ON_WRITE;
    }

    return RP2040_CLEAN_ON_WRITE;
}

static void rp2040_write_to_register(RP2040AccessType type, uint32_t *reg, uint32_t value)
{
    switch (type)
    {
        case RP2040_NORMAL_ACCESS:
        {
            *reg = value;
        } break;
        case RP2040_XOR_ON_WRITE:
        {
            *reg ^= value;
        } break;
        case RP2040_SET_ON_WRITE:
        {
            *reg |= value;
        } break;
        case RP2040_CLEAN_ON_WRITE:
        {
            *reg &= ~(value);
        } break;
    }
}

static void rp2040_process_gpio_write(hwaddr offset, uint32_t value, const Rp2040GpioRegisterDesc* desc, const char *name)
{
    RP2040AccessType access = rp2040_get_access_type(offset);
    offset = offset & 0x0fff;

    if (offset <= desc->state_and_control_end) /* Process status & control registers */
    {
        const hwaddr index = offset >> 2;
        const bool is_stat = (index % 2) == 0;
        if (is_stat)
        {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: Trying to write read only register at address: 0x%lx\n", __func__, offset);
            return;
        }
        else 
        {
            const hwaddr array_index = index >> 1;

            rp2040_write_to_register(access, &desc->control[array_index]._value, value);

            if (desc->control[array_index].oeover == 0x03)
            {
                desc->status[array_index].oetopad = 1;

                desc->status[array_index].irqtoproc = 0;
                desc->status[array_index].irqfrompad = 0;
                desc->status[array_index].intoperi = 0;
                desc->status[array_index].infrompad = 0;
                desc->status[array_index].outtopad = 0;

                switch (desc->control[array_index].outover)
                {
                    case 0x00:
                    {
                        // how to trigger periph
                        return;
                    }
                    case 0x01:
                    {
                        // how to trigger periph
                        return;
                    }
                    case 0x02:
                    {
                        desc->status[array_index].outtopad = 0;

                        qemu_set_irq(desc->irq[array_index], 0);
                        return;
                    }
                    case 0x03:
                    {
                        desc->status[array_index].irqtoproc = 1;
                        desc->status[array_index].irqfrompad = 1;
                        desc->status[array_index].intoperi = 1;
                        desc->status[array_index].infrompad = 1;
                        desc->status[array_index].outtopad = 1;
                        qemu_set_irq(desc->irq[array_index], 1);
                        return;
                    }
                }
            }
            else 
            {
                desc->status[array_index].oetopad = 0;
            }
            return;
        }
    }

    qemu_log_mask(LOG_UNIMP, "GPIO(%s) register at offset: 0x%" HWADDR_PRIX " not implemented.\n", name, offset);
}


static void rp2040_gpio_qspi_write(void *opaque, hwaddr offset, 
    uint64_t value, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .state_and_control_end = 0x058,
        .status = state->qspi_status,
        .control = state->qspi_ctrl,
        .irq = state->qspi_out,
    };
    rp2040_process_gpio_write(offset, value, &desc, "QSPI");
}

static uint64_t rp2040_gpio_qspi_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .state_and_control_end = 0x058,
        .status = state->qspi_status,
        .control = state->qspi_ctrl,
        .irq = state->qspi_out,
    };

    return rp2040_process_gpio_read(offset, &desc, "QSPI");
}

static const MemoryRegionOps rp2040_gpio_qspi_io = {
    .read = rp2040_gpio_qspi_read,
    .write = rp2040_gpio_qspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void rp2040_gpio_write(void *opaque, hwaddr offset, 
    uint64_t value, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .state_and_control_end = 0x0ec,
        .status = state->gpio_status,
        .control = state->gpio_ctrl,
        .irq = state->gpio_out,
    };
    rp2040_process_gpio_write(offset, value, &desc, "GPIO");
}


static uint64_t rp2040_gpio_read(void *opaque, hwaddr offset, unsigned int size)
{
    RP2040GpioState *state = RP2040_GPIO(opaque);
    const Rp2040GpioRegisterDesc desc = {
        .state_and_control_end = 0x0ec,
        .status = state->gpio_status,
        .control = state->gpio_ctrl,
        .irq = state->gpio_out,
    };

    return rp2040_process_gpio_read(offset, &desc, "GPIO");
}


static const MemoryRegionOps rp2040_gpio_io = {
    .read = rp2040_gpio_read,
    .write = rp2040_gpio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};


static void rp2040_gpio_set(void *opaque, int line, int value)
{
}

static void rp2040_gpio_qspi_set(void *opaque, int line, int value)
{
}

static void rp2040_gpio_realize(DeviceState *dev, Error **errp)
{
    RP2040GpioState *state = RP2040_GPIO(dev);
    SysBusDevice *sysbus = SYS_BUS_DEVICE(dev);

    memory_region_init(&state->container, OBJECT(dev), "gpio", RP2040_IO_REGISTER_SIZE);
    sysbus_init_mmio(sysbus, &state->container);

    /* Initialize GPIOs */
    memory_region_init_io(&state->gpio_mmio, OBJECT(dev), &rp2040_gpio_io, state,
        "io", 0x4000);
    memory_region_add_subregion(&state->container, 0x0000, &state->gpio_mmio);

    qdev_init_gpio_out_named(DEVICE(state), state->gpio_out, "out", RP2040_GPIO_PINS);
    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_set, "in", RP2040_GPIO_PINS);

    /* Initialize QSPI XIP GPIOs */
    memory_region_init_io(&state->qspi_mmio, OBJECT(dev), &rp2040_gpio_qspi_io, state,
        "qspi", 0x4000);
    memory_region_add_subregion(&state->container, 0x4000, &state->qspi_mmio);


    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_out[0], "qspi-out", RP2040_GPIO_QSPI_PINS - 2);
    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_cs, "qspi-cs", 1);
    qdev_init_gpio_out_named(DEVICE(state), &state->qspi_sclk, "qspi-sclk", 1);
    qdev_init_gpio_in_named(DEVICE(state), rp2040_gpio_qspi_set, "qspi-in", RP2040_GPIO_QSPI_PINS - 2);

    for (int i = 0; i < RP2040_GPIO_PINS; ++i)
    {
        state->gpio_ctrl[i]._value = 0x00000000;
        state->gpio_ctrl[i].funcsel = 0x1f;
        state->gpio_status[i]._value = 0x00000000;
    }

    for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i)
    {
        state->qspi_ctrl[i]._value = 0x00000000;
        state->qspi_ctrl[i].funcsel = 0x1f;
        state->qspi_status[i]._value = 0x00000000;
    }
}

static void rp2040_gpio_unrealize(DeviceState *dev)
{
    RP2040GpioState *state = RP2040_GPIO(dev);

    /* Remove cyclic dependency between the device and it own memory subregions */
    memory_region_del_subregion(&state->container, &state->gpio_mmio);
    memory_region_del_subregion(&state->container, &state->qspi_mmio);
}

static void rp2040_gpio_instance_init(Object *obj)
{

}

static void rp2040_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "RP2040 GPIO";
    dc->realize = rp2040_gpio_realize;
    dc->unrealize = rp2040_gpio_unrealize;
}

static const TypeInfo rp2040_gpio_qspi_info = {
    .name = TYPE_RP2040_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040GpioState),
    .instance_init = rp2040_gpio_instance_init, 
    .class_init = rp2040_gpio_class_init,
};

static void rp2040_gpio_qspi_register_types(void)
{
    type_register_static(&rp2040_gpio_qspi_info);
}

type_init(rp2040_gpio_qspi_register_types)

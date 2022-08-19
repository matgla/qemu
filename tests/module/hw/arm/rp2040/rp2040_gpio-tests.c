#include "qmodule_test.h"

#include <stdarg.h>

#include "qemu/osdep.h"
#include "qom/object.h"
#include "hw/irq.h"
#include "qapi/error.h"
#include "exec/address-spaces.h"
#include "sysemu/runstate.h"

#include "hw/gpio/rp2040_gpio.h"

QMT_DEFINE_FAKE_VOID3(irq_spy_handler, void *, int, int);

typedef struct {
    RP2040GpioState sut;
} RP2040GpioTestsFixture;


typedef union RP2040TestGpioControl {
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
    uint32_t _value;
} RP2040TestGpioControl;


static void test_initialize_sut(RP2040GpioState *sut, MemoryRegion *test_memory) 
{
    MemoryRegion *mr = NULL; 

    object_initialize_child(OBJECT(test_memory), "sut", sut, TYPE_RP2040_GPIO);
    sysbus_realize(SYS_BUS_DEVICE(sut), &error_fatal);

    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(sut), 0);
    memory_region_add_subregion_overlap(test_memory, 0, mr, 0);
}

static void test_finalize_sut(RP2040GpioState *sut, MemoryRegion *test_memory) 
{
    MemoryRegion *mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(sut), 0);
    memory_region_del_subregion(test_memory, mr);
}


static void test_write_to_output_should_raise_irq(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    qemu_irq irq_spy;

    /* Test Data */
    RP2040TestGpioControl set_output_to_high = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x03,
        .inover = 0x00,
        .irqover = 0x00
    };

    /* Perform GPIO test */
    for (int i = 0; i < RP2040_GPIO_PINS; ++i) 
    {
        irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
        qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "out", i, irq_spy);
        qmt_memory_write(0x04 + i * 0x08, &set_output_to_high, sizeof(RP2040TestGpioControl));
        qemu_free_irq(irq_spy);
    }

    /* Perform QSPI test */
    for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i) 
    {
        irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
        qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "qspi-out", i, irq_spy);
        qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_high, sizeof(RP2040TestGpioControl));
        qemu_free_irq(irq_spy);
    }
}


static void rp2040_gpio_tests_setup(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    qmt_initialize();
    test_initialize_sut(&fixture->sut, (MemoryRegion *)data);
}

static void rp2040_gpio_tests_teardown(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    test_finalize_sut(&fixture->sut, (MemoryRegion *)data);
    qmt_verify_and_release();
}

void qmt_register_testcases(MemoryRegion *test_memory)
{
    g_test_add("/rp2040/gpio/test_write_to_output_should_raise_irq", RP2040GpioTestsFixture, 
        test_memory, rp2040_gpio_tests_setup, test_write_to_output_should_raise_irq,
        rp2040_gpio_tests_teardown);


} 
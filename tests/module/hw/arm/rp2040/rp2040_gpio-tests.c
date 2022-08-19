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
    const RP2040GpioControl set_output_to_high = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x03,
        .inover = 0x00,
        .irqover = 0x00
    };

    const RP2040GpioControl set_output_to_high_but_not_enable = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x02,
        .inover = 0x00,
        .irqover = 0x00
    };

    const RP2040GpioControl set_output_to_low = {
        .funcsel = 0x1f,
        .outover = 0x02,
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
        qmt_memory_write(0x04 + i * 0x08, &set_output_to_high, sizeof(RP2040GpioControl));
        qmt_memory_write(0x04 + i * 0x08, &set_output_to_high_but_not_enable, sizeof(RP2040GpioControl));
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
        qmt_memory_write(0x04 + i * 0x08, &set_output_to_low, sizeof(RP2040GpioControl));

        qemu_free_irq(irq_spy);
    }

    /* Perform QSPI test */
    for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i) 
    {
        irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
        qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "qspi-out", i, irq_spy);
        qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_high, sizeof(RP2040GpioControl));
        qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_high_but_not_enable, sizeof(RP2040GpioControl));

        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
        qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_low, sizeof(RP2040GpioControl));
        qemu_free_irq(irq_spy);
    }
}

// static void test_invert_interrupt(RP2040GpioTestsFixture *fixture, gconstpointer data)
// {
//     qemu_irq irq_spy;

//     /* Test Data */
//     const RP2040TestGpioControl set_output_to_high = {
//         .funcsel = 0x1f,
//         .outover = 0x03,
//         .oeover = 0x03,
//         .inover = 0x00,
//         .irqover = 0x00
//     };

//     const RP2040TestGpioControl set_output_to_low = {
//         .funcsel = 0x1f,
//         .outover = 0x03,
//         .oeover = 0x02,
//         .inover = 0x00,
//         .irqover = 0x00
//     };


//     /* Perform GPIO test */
//     for (int i = 0; i < RP2040_GPIO_PINS; ++i) 
//     {
//         irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
//         QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
//         qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "out", i, irq_spy);
//         qmt_memory_write(0x04 + i * 0x08, &set_output_to_high, sizeof(RP2040TestGpioControl));

//         QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
//         qmt_memory_write(0x04 + i * 0x08, &set_output_to_low, sizeof(RP2040TestGpioControl));

//         qemu_free_irq(irq_spy);
//     }

//     /* Perform QSPI test */
//     for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i) 
//     {
//         irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
//         QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
//         qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "qspi-out", i, irq_spy);
//         qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_high, sizeof(RP2040TestGpioControl));

//         QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
//         qmt_memory_write(0x04 + i * 0x08 + 0x4000, &set_output_to_low, sizeof(RP2040TestGpioControl));
//         qemu_free_irq(irq_spy);
//     }
// }

static void test_initialized_state(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    uint32_t status;
    uint32_t control;
    /* Perform GPIO test */ 
    for (int i = 0; i < RP2040_GPIO_PINS; ++i) 
    {
        qmt_memory_read(i * 0x08, &status, sizeof(uint32_t));
        g_assert_cmpint(status, ==, 0x00000000u);

        qmt_memory_read(0x04 + i * 0x08, &control, sizeof(uint32_t));
        g_assert_cmpint(control, ==, 0x0000001fu);
    }

    /* Perform QSPI test */
    for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i) 
    {
        qmt_memory_read(i * 0x08 + 0x4000, &status, sizeof(uint32_t));
        g_assert_cmpint(status, ==, 0x00000000u);

        qmt_memory_read(0x04 + i * 0x08 + 0x4000, &control, sizeof(uint32_t));
        g_assert_cmpint(control, ==, 0x0000001fu);
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

    g_test_add("/rp2040/gpio/test_initialized_state", RP2040GpioTestsFixture, 
        test_memory, rp2040_gpio_tests_setup, test_initialized_state,
        rp2040_gpio_tests_teardown);
    // g_test_add("/rp2040/gpio/test_write_interrupt_output", RP2040GpioTestsFixture, 
    //     test_memory, rp2040_gpio_tests_setup, test_invert_interrupt,
    //     rp2040_gpio_tests_teardown);

} 
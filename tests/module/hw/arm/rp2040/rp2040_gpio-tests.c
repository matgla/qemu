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


#define RP2040_ATOMIC_XOR_OFFSET 0x1000
#define RP2040_ATOMIC_SET_OFFSET 0x2000
#define RP2040_ATOMIC_CLEAR_OFFSET 0x3000

#define RP2040_GPIO_CTRL_FUNCSEL_BITS 0x0000001f
#define RP2040_GPIO_CTRL_OUTOVER_BITS 0x00000300
#define RP2040_GPIO_CTRL_OEOVER_BITS  0x00003000

static void register_bitmask_xor(hwaddr base_address, const uint32_t bitmask)
{
    qmt_memory_write(base_address + RP2040_ATOMIC_XOR_OFFSET, &bitmask, sizeof(bitmask));
}

static void register_bitmask_set(hwaddr base_address, const uint32_t bitmask)
{
    qmt_memory_write(base_address + RP2040_ATOMIC_SET_OFFSET, &bitmask, sizeof(bitmask));
}

static void register_bitmask_clear(hwaddr base_address,  const uint32_t bitmask)
{
    qmt_memory_write(base_address + RP2040_ATOMIC_CLEAR_OFFSET, &bitmask, sizeof(bitmask));
}

static void register_set_with_mask(hwaddr base_address, const uint32_t value, const uint32_t mask)
{
    uint32_t reg;
    qmt_memory_read(base_address, &reg, sizeof(reg));
    register_bitmask_xor(base_address, (reg ^ value) & mask);
}

static void test_write_to_output_should_raise_irq(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    qemu_irq irq_spy;

    /* Test Data */
    const RP2040GpioControl enable_output = {
        .funcsel = 0x00,
        .outover = 0x00,
        .oeover = 0x03,
        .inover = 0x00,
        .irqover = 0x00
    };

    const RP2040GpioControl set_output_high = {
        .funcsel = 0x00,
        .outover = 0x03,
        .oeover = 0x00,
        .inover = 0x00,
        .irqover = 0x00
    };

    const RP2040GpioControl set_output_low = {
        .funcsel = 0x00,
        .outover = 0x02,
        .oeover = 0x00,
        .inover = 0x00,
        .irqover = 0x00
    };


    /* Perform GPIO test */
    for (int i = 0; i < RP2040_GPIO_PINS; ++i) 
    {
        uint32_t reg;
        uint32_t status;
        hwaddr ctrl_addr = 0x04 + i * 0x08;
        hwaddr status_addr = i * 0x08;
        irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
        qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "out", i, irq_spy);
        register_bitmask_set(ctrl_addr, enable_output._value);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000301f);

        qmt_memory_read(status_addr, &status, sizeof(uint32_t));
        g_assert_cmphex(status, ==, 0x00002000);
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
        register_bitmask_set(ctrl_addr, set_output_high._value);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000331f);
        qmt_memory_read(status_addr, &status, sizeof(uint32_t));
        g_assert_cmphex(status, ==, 0x050a2200); /* value after filtering out reserved fields */

        register_bitmask_clear(ctrl_addr, RP2040_GPIO_CTRL_OUTOVER_BITS);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000301f);
        qmt_memory_read(status_addr, &status, sizeof(uint32_t));
        g_assert_cmphex(status, ==, 0x00002000);
        
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
        register_bitmask_set(ctrl_addr, set_output_low._value);

        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000321f);

        qmt_memory_read(status_addr, &status, sizeof(uint32_t));
        g_assert_cmphex(status, ==, 0x00002000);

        qemu_free_irq(irq_spy);
    }

    /* Perform QSPI test */
    for (int i = 0; i < RP2040_GPIO_QSPI_PINS - 2; ++i) 
    {
        uint32_t reg;
        hwaddr ctrl_addr = 0x04 + i * 0x08 + 0x4000;
        irq_spy = qemu_allocate_irq(irq_spy_handler, NULL, i);
        qdev_connect_gpio_out_named(DEVICE(&fixture->sut), "qspi-out", i, irq_spy);
        register_bitmask_set(ctrl_addr, enable_output._value);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000301f);
        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(1));
        register_bitmask_set(ctrl_addr, set_output_high._value);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000331f);

        register_bitmask_clear(ctrl_addr, RP2040_GPIO_CTRL_OUTOVER_BITS);
        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000301f);

        QMT_EXPECT_CALL(irq_spy_handler, qmt_expect_null(), qmt_expect_int(i), qmt_expect_int(0));
        register_bitmask_set(ctrl_addr, set_output_low._value);

        qmt_memory_read(ctrl_addr, &reg, sizeof(uint32_t));
        g_assert_cmphex(reg, ==, 0x0000321f);

        qemu_free_irq(irq_spy);
    }
}

static void test_aliased_access_gpio(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    const RP2040GpioControl test_data = {
        .funcsel = 0x12,
        .outover = 0x02,
        .oeover = 0x01,
        .inover = 0x03,
        .irqover = 0x02
    };

    const RP2040GpioControl test_data_2 = {
        .funcsel = 0x12,
        .outover = 0x03,
        .oeover = 0x00,
        .inover = 0x03,
        .irqover = 0x01
    };

    const RP2040GpioControl test_data_3 = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x03,
        .inover = 0x03,
        .irqover = 0x03
    };

    hwaddr addr = 0x04 + 0x00 * 0x08;
    uint32_t reg;

    register_bitmask_set(addr, test_data._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x2003121f);

    register_set_with_mask(addr, 0x12, RP2040_GPIO_CTRL_FUNCSEL_BITS);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x20031212);

    register_bitmask_xor(addr, test_data_2._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x30001100);

    register_set_with_mask(addr, test_data_3._value, 0xffffffff);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x3003331f);

    register_bitmask_clear(addr, test_data_3._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x00000000);
}

static void test_aliased_access_gpio_qspi(RP2040GpioTestsFixture *fixture, gconstpointer data)
{
    const RP2040GpioControl test_data = {
        .funcsel = 0x12,
        .outover = 0x02,
        .oeover = 0x01,
        .inover = 0x03,
        .irqover = 0x02
    };

    const RP2040GpioControl test_data_2 = {
        .funcsel = 0x12,
        .outover = 0x03,
        .oeover = 0x00,
        .inover = 0x03,
        .irqover = 0x01
    };

    const RP2040GpioControl test_data_3 = {
        .funcsel = 0x1f,
        .outover = 0x03,
        .oeover = 0x03,
        .inover = 0x03,
        .irqover = 0x03
    };

    hwaddr addr = 0x04 + 0x02 * 0x08 + 0x4000;
    uint32_t reg;

    register_bitmask_set(addr, test_data._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x2003121f);

    register_set_with_mask(addr, 0x12, RP2040_GPIO_CTRL_FUNCSEL_BITS);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x20031212);

    register_bitmask_xor(addr, test_data_2._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x30001100);

    register_set_with_mask(addr, test_data_3._value, 0xffffffff);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x3003331f);

    register_bitmask_clear(addr, test_data_3._value);
    qmt_memory_read(addr, &reg, sizeof(uint32_t));
    g_assert_cmphex(reg, ==, 0x00000000);
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

    g_test_add("/rp2040/gpio/test_aliased_access_gpio", RP2040GpioTestsFixture, 
        test_memory, rp2040_gpio_tests_setup, test_aliased_access_gpio,
        rp2040_gpio_tests_teardown);

    g_test_add("/rp2040/gpio/test_aliased_access_gpio_qspi", RP2040GpioTestsFixture, 
        test_memory, rp2040_gpio_tests_setup, test_aliased_access_gpio_qspi,
        rp2040_gpio_tests_teardown);
} 
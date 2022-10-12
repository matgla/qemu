/*
 * RP2040 Resets module tests
 *
 * Copyright (c) 2022 Mateusz Stadnik <matgla@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "qmodule_test.h"

#include "hw/misc/rp2040_resets.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "hw/qdev-properties.h"

#define TYPE_OBJECT_MOCK "object_mock"
OBJECT_DECLARE_SIMPLE_TYPE(ObjectMock, OBJECT_MOCK)

struct ObjectMock {
    SysBusDevice parent_obj;
    MemoryRegion mmio;
};

#define NUMBER_OF_DEVICES 25

typedef struct {
    RP2040ResetsState* sut;
    ObjectMock mock[NUMBER_OF_DEVICES];
} RP2040ResetsTestsFixture;

static QMT_DEFINE_FAKE_VOID(object_mock_reset_enter, (Object *, obj),
                                                     (ResetType, type));
static QMT_DEFINE_FAKE_VOID(object_mock_reset_hold, (Object *, obj));
static QMT_DEFINE_FAKE_VOID(object_mock_reset_exit, (Object *, obj));

static void object_mock_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->desc = "ObjectMock";
    rc->phases.enter = object_mock_reset_enter;
    rc->phases.hold = object_mock_reset_hold;
    rc->phases.exit = object_mock_reset_exit;
}

static const TypeInfo object_mock_info = {
    .name           = TYPE_OBJECT_MOCK,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(ObjectMock),
    .class_init     = &object_mock_class_init
};

static void register_test_types(void)
{
    type_register_static(&object_mock_info);
}

type_init(register_test_types);


static void test_initialize_sut(RP2040ResetsTestsFixture *fixture,
                                MemoryRegion *test_memory)
{
    MemoryRegion *mr;
    fixture->sut = rp2040_resets_create();

    for (int i = 0; i < NUMBER_OF_DEVICES; ++i) {
        g_autofree char *name = g_strdup_printf("object_mock[%d]", i);
        object_initialize_child(OBJECT(test_memory), name,
            &fixture->mock[i], TYPE_OBJECT_MOCK);
        sysbus_realize(SYS_BUS_DEVICE(&fixture->mock[i]), &error_abort);
    }


    object_property_set_link(OBJECT(fixture->sut), "adc",
                             OBJECT(&fixture->mock[0]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "busctrl",
                             OBJECT(&fixture->mock[1]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "dma",
                             OBJECT(&fixture->mock[2]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "i2c0",
                             OBJECT(&fixture->mock[3]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "i2c1",
                             OBJECT(&fixture->mock[4]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "gpio",
                             OBJECT(&fixture->mock[5]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "qspi",
                             OBJECT(&fixture->mock[6]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pads",
                             OBJECT(&fixture->mock[8]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "qspi_pads",
                             OBJECT(&fixture->mock[9]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pio0",
                             OBJECT(&fixture->mock[10]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pio1",
                             OBJECT(&fixture->mock[11]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pllsys",
                             OBJECT(&fixture->mock[12]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pllusb",
                             OBJECT(&fixture->mock[13]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "pwm",
                             OBJECT(&fixture->mock[14]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "rtc",
                             OBJECT(&fixture->mock[15]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "spi0",
                             OBJECT(&fixture->mock[16]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "spi1",
                             OBJECT(&fixture->mock[17]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "syscfg",
                             OBJECT(&fixture->mock[18]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "sysinfo",
                             OBJECT(&fixture->mock[19]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "tbman",
                             OBJECT(&fixture->mock[20]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "timer",
                             OBJECT(&fixture->mock[21]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "uart0",
                             OBJECT(&fixture->mock[22]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "uart1",
                             OBJECT(&fixture->mock[23]), &error_abort);
    object_property_set_link(OBJECT(fixture->sut), "usbctrl",
                             OBJECT(&fixture->mock[24]), &error_abort);


    // for (int i = 0; i < NUMBER_OF_DEVICES; ++i) {
    //     QMT_EXPECT_CALL(object_mock_reset_enter,
    //                     qmt_expect_ptr(&fixture->mock[i]),
    //                     qmt_expect_int(RESET_TYPE_COLD));
    //     QMT_EXPECT_CALL(object_mock_reset_hold,
    //                     qmt_expect_ptr(&fixture->mock[i]));
    // }

    sysbus_realize(SYS_BUS_DEVICE(fixture->sut), &error_abort);

    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(fixture->sut), 0);
    memory_region_add_subregion_overlap(test_memory, 0, mr, 0);
}

static void test_finalize_sut(RP2040ResetsTestsFixture *fixture,
                              MemoryRegion *test_memory)
{

}

#define RP2040_RESET_ADDR 0x0
#define RP2040_RESET_DONE_ADDR 0x8



static void test_reset_devices(RP2040ResetsTestsFixture *fixture,
                               gconstpointer data)
{
    uint32_t reset_state = 0;
    uint32_t reset_done = 0;
    uint32_t expected_reset_state = 0;
    uint32_t expected_reset_done = 0xffffffff;
    qmt_memory_read(RP2040_RESET_ADDR, &reset_state, sizeof(reset_state));
    g_assert_cmphex(reset_state, ==, expected_reset_state);
    qmt_memory_read(RP2040_RESET_DONE_ADDR, &reset_done, sizeof(reset_done));
    g_assert_cmphex(reset_done, ==, expected_reset_done);


    /* Reset all devices one by one */
    for (int i = 0; i < 24; ++i) {
        reset_state |= 1 << i;

        if (i != 7) { /* JTAG is handled in different way */
            QMT_EXPECT_CALL(object_mock_reset_enter,
                            qmt_expect_ptr(&fixture->mock[i]),
                            qmt_expect_int(RESET_TYPE_COLD));
            QMT_EXPECT_CALL(object_mock_reset_hold,
                            qmt_expect_ptr(&fixture->mock[i]));
        }
        qmt_memory_write(RP2040_RESET_ADDR, &reset_state, sizeof(reset_state));
        expected_reset_state <<= 1;
        expected_reset_state |= 1;
        g_assert_cmphex(reset_state, ==, expected_reset_state);
        qmt_memory_read(RP2040_RESET_DONE_ADDR, &reset_done, sizeof(reset_done));
        expected_reset_done &= ~(1 << i);
        g_assert_cmphex(reset_done, ==, expected_reset_done);
        qmt_verify_and_clear_expectations();
    }

}

static void rp2040_resets_tests_setup(RP2040ResetsTestsFixture *fixture,
    gconstpointer data)
{
    qmt_initialize();
    test_initialize_sut(fixture, (MemoryRegion *)data);
}

static void rp2040_resets_tests_teardown(RP2040ResetsTestsFixture *fixture,
    gconstpointer data)
{
    test_finalize_sut(fixture, (MemoryRegion *)data);
    qmt_verify_and_release();
}

void qmt_register_testcases(MemoryRegion *test_memory)
{
    g_test_add("/rp2040/resets/test_reset_devices",
        RP2040ResetsTestsFixture, test_memory, rp2040_resets_tests_setup,
        test_reset_devices, rp2040_resets_tests_teardown);
}

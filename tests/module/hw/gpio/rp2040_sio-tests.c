/*
 * RP2040 SIO module tests
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

#include "hw/gpio/rp2040_gpio.h"
#include "hw/gpio/rp2040_sio.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"


#define RP2040_NUMBER_OF_SIO 2

#define RP2040_NEXT_SIO_OFFSET 0x10000

#define RP2040_SIO_FIFO_ST 0x050
#define RP2040_SIO_FIFO_WR 0x054
#define RP2040_SIO_FIFO_RD 0x058

typedef struct {
    RP2040GpioState gpio;
    RP2040SioState sut[RP2040_NUMBER_OF_SIO];
} RP2040SioTestsFixture;

static void test_initialize_sut(RP2040SioTestsFixture *fixture, MemoryRegion *test_memory)
{
    RP2040SioState *suts = fixture->sut;
    Error **errp = &error_abort;

    for (int i = 0; i < RP2040_NUMBER_OF_SIO; ++i) {
        MemoryRegion *mr = NULL;
        RP2040SioState *sut = &suts[i];
        g_autofree char *name = g_strdup_printf("sut[%d]", i);
        object_initialize_child(OBJECT(test_memory), name, sut,
            TYPE_RP2040_SIO);
        qdev_prop_set_uint32(DEVICE(sut), "cpuid", i);
        object_property_set_link(OBJECT(sut), "gpio",
            OBJECT(&fixture->gpio), errp);
        sysbus_realize(SYS_BUS_DEVICE(sut), &error_fatal);
        mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(sut), 0);
        memory_region_add_subregion_overlap(test_memory,
            i * RP2040_NEXT_SIO_OFFSET, mr, 0);
    }

    suts[0].fifo_tx = &suts[1].fifo_rx;
    suts[1].fifo_tx = &suts[0].fifo_rx;
}

static void test_finalize_sut(RP2040SioState *suts, MemoryRegion *test_memory)
{
    for (int i = 0; i < RP2040_NUMBER_OF_SIO; ++i) {
        RP2040SioState *sut = &suts[i];
        MemoryRegion *mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(sut), 0);
        memory_region_del_subregion(test_memory, mr);
    }
}

static void test_cpuid(RP2040SioTestsFixture *fixture, gconstpointer data)
{
    uint32_t cpuid;
    qmt_memory_read(0x00, &cpuid, sizeof(cpuid));
    g_assert_cmpuint(cpuid, ==, 0);

    qmt_memory_read(0x00 + RP2040_NEXT_SIO_OFFSET, &cpuid, sizeof(cpuid));
    g_assert_cmpuint(cpuid, ==, 1);
}

static void test_intercore_fifo_roe_flag(RP2040SioTestsFixture *fixture,
    gconstpointer data)
{
    /* Writing of any value should clear flags */
    const int clear_flags_1 = 0x00;
    const int clear_flags_2 = 0xff;

    int read_data = 0;
    uint32_t fifo_st = 0;

    /* FIFO flags are correctly initialized */
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);

    /* Reading from empty FIFO raises ROE */
    qmt_memory_read(RP2040_SIO_FIFO_RD,
                    &read_data, sizeof(uint32_t));
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x0000000a);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);

    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET,
                    &read_data, sizeof(uint32_t));
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x0000000a);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x0000000a);

    /* Writing any value to FIFO_ST clears ROE */
    qmt_memory_write(RP2040_SIO_FIFO_ST,
                     &clear_flags_1, sizeof(uint32_t));
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x0000000a);

    qmt_memory_write(RP2040_SIO_FIFO_ST + RP2040_NEXT_SIO_OFFSET,
                     &clear_flags_2, sizeof(uint32_t));
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
}

static void test_intercore_fifo_rw(RP2040SioTestsFixture *fixture,
    gconstpointer data)
{
    /* Writing of any value should clear flags */
    const uint32_t test_data_1 = 0x12345689;
    const uint32_t test_data_2 = 0xabcdef01;
    const uint32_t test_data_3 = 0xff;
    const uint32_t test_data_4 = 0xaa;

    uint32_t read_data = 0;
    uint32_t fifo_st = 0;

    /* FIFO flags are correctly initialized */
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);

    /* Test single data */
    qmt_memory_write(RP2040_SIO_FIFO_WR, &test_data_1, sizeof(uint32_t));
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_RD,
                    &read_data, sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_1);

    /* Write data to FIFOs */
    qmt_memory_write(RP2040_SIO_FIFO_WR, &test_data_2, sizeof(uint32_t));

    /* After first write VLD should be raised */
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);


    /* Write rest of data for FIFOs */
    qmt_memory_write(RP2040_SIO_FIFO_WR, &test_data_3, sizeof(uint32_t));
    qmt_memory_write(RP2040_SIO_FIFO_WR, &test_data_4, sizeof(uint32_t));
    qmt_memory_write(RP2040_SIO_FIFO_WR, &test_data_1, sizeof(uint32_t));

    qmt_memory_write(RP2040_SIO_FIFO_WR + RP2040_NEXT_SIO_OFFSET,
        &test_data_4, sizeof(uint32_t));
    qmt_memory_write(RP2040_SIO_FIFO_WR + RP2040_NEXT_SIO_OFFSET,
        &test_data_3, sizeof(uint32_t));
    qmt_memory_write(RP2040_SIO_FIFO_WR + RP2040_NEXT_SIO_OFFSET,
        &test_data_2, sizeof(uint32_t));
    qmt_memory_write(RP2040_SIO_FIFO_WR + RP2040_NEXT_SIO_OFFSET,
        &test_data_1, sizeof(uint32_t));

    /* Verify reading except last */
    qmt_memory_read(RP2040_SIO_FIFO_RD, &read_data, sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_4);
    qmt_memory_read(RP2040_SIO_FIFO_RD, &read_data, sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_3);
    qmt_memory_read(RP2040_SIO_FIFO_RD, &read_data, sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_2);

    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_2);
    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_3);
    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_4);

    /* Verify states before reading RDY and VLD is set */
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);

    /* Verify VLD clears after last read */
    qmt_memory_read(RP2040_SIO_FIFO_RD, &read_data, sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_1);

    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);

    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_1);

    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);
    qmt_memory_read(RP2040_NEXT_SIO_OFFSET + RP2040_SIO_FIFO_ST,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000002);


    /* Verify RDY is raised when FIFO is full */
    for (int i = 0; i < 8; ++i) {
        qmt_memory_write(RP2040_SIO_FIFO_WR,
            &test_data_1, sizeof(uint32_t));
    }
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000000);

    qmt_memory_read(RP2040_SIO_FIFO_ST + RP2040_NEXT_SIO_OFFSET,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);

    for (int i = 0; i < 8; ++i) {
        qmt_memory_write(RP2040_SIO_FIFO_WR + RP2040_NEXT_SIO_OFFSET,
            &test_data_1, sizeof(uint32_t));
    }
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000001);

    qmt_memory_read(RP2040_SIO_FIFO_ST + RP2040_NEXT_SIO_OFFSET,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000001);

    /* Verify RDY clears after first read */
    qmt_memory_read(RP2040_SIO_FIFO_RD + RP2040_NEXT_SIO_OFFSET, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_1);
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);

    qmt_memory_read(RP2040_SIO_FIFO_ST + RP2040_NEXT_SIO_OFFSET,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000001);

    qmt_memory_read(RP2040_SIO_FIFO_RD, &read_data,
        sizeof(uint32_t));
    g_assert_cmpuint(read_data, ==, test_data_1);
    qmt_memory_read(RP2040_SIO_FIFO_ST, &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);

    qmt_memory_read(RP2040_SIO_FIFO_ST + RP2040_NEXT_SIO_OFFSET,
                    &fifo_st, sizeof(uint32_t));
    g_assert_cmpuint(fifo_st, ==, 0x00000003);
}



static void rp2040_sio_tests_setup(RP2040SioTestsFixture *fixture,
    gconstpointer data)
{
    qmt_initialize();
    test_initialize_sut(fixture, (MemoryRegion *)data);
}

static void rp2040_sio_tests_teardown(RP2040SioTestsFixture *fixture,
    gconstpointer data)
{
    test_finalize_sut(fixture->sut, (MemoryRegion *)data);
    qmt_verify_and_release();
}

void qmt_register_testcases(MemoryRegion *test_memory)
{
    g_test_add("/rp2040/sio/test_cpuid",
        RP2040SioTestsFixture, test_memory, rp2040_sio_tests_setup,
        test_cpuid, rp2040_sio_tests_teardown);

    g_test_add("/rp2040/sio/test_intercore_fifo_rw",
        RP2040SioTestsFixture, test_memory, rp2040_sio_tests_setup,
        test_intercore_fifo_rw, rp2040_sio_tests_teardown);

    g_test_add("/rp2040/sio/test_intercore_fifo_roe_flag",
        RP2040SioTestsFixture, test_memory, rp2040_sio_tests_setup,
        test_intercore_fifo_roe_flag, rp2040_sio_tests_teardown);


}

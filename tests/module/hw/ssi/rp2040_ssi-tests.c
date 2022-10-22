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

#include "hw/ssi/rp2040_ssi.h"

#include "qapi/error.h"
#include "qom/object.h"


static QMT_DEFINE_FAKE_VOID(cs_spy, (void *, a), (int, b), (int, c));

typedef struct {
    Rp2040SsiState sut;
} Rp2040SsiTestsFixture;

static void test_initialize_sut(Rp2040SsiTestsFixture *fixture,
                                MemoryRegion *test_memory)
{
    MemoryRegion *mr = NULL;
    Rp2040SsiState *sut = &fixture->sut;
    object_initialize_child(OBJECT(test_memory), "sut", sut, TYPE_RP2040_SSI);
    sysbus_realize(SYS_BUS_DEVICE(sut), &error_fatal);

    mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(sut), 0);
    memory_region_add_subregion_overlap(test_memory, 0, mr, 0);

    resettable_cold_reset_fn(OBJECT(sut));
}

static void test_finalize_sut(Rp2040SsiTestsFixture *fixture,
                              MemoryRegion *test_memory)
{
    MemoryRegion *mr = sysbus_mmio_get_region(SYS_BUS_DEVICE(&fixture->sut), 0);
    memory_region_del_subregion(test_memory, mr);
}

static void rp2040ssi_tests_setup(Rp2040SsiTestsFixture *fixture,
                                  gconstpointer data)
{
    qmt_initialize();
    test_initialize_sut(fixture, (MemoryRegion *)data);
}

static void rp2040ssi_tests_teardown(Rp2040SsiTestsFixture *fixture,
                                     gconstpointer data)
{
    test_finalize_sut(fixture, (MemoryRegion *)data);
    qmt_verify_and_release();
}

static void test_return_idr(Rp2040SsiTestsFixture *fixture,
                            gconstpointer data)
{
    uint32_t version;
    qmt_memory_read(0x58, &version, sizeof(version));
    g_assert_cmphex(version, ==, 0x51535049);
}

static void test_return_version_id(Rp2040SsiTestsFixture *fixture,
                                   gconstpointer data)
{
    uint32_t version;
    qmt_memory_read(0x5c, &version, sizeof(version));
    g_assert_cmphex(version, ==, 0x3430312a);
}

void qmt_register_testcases(MemoryRegion *test_memory)
{
    g_test_add("/rp2040/ssi/read_identification_register",
        Rp2040SsiTestsFixture, test_memory, rp2040ssi_tests_setup,
        test_return_idr, rp2040ssi_tests_teardown);
    g_test_add("/rp2040/ssi/read_version_id",
        Rp2040SsiTestsFixture, test_memory, rp2040ssi_tests_setup,
        test_return_version_id, rp2040ssi_tests_teardown);
}


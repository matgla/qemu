/*
 *  QModuleTest main function
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

#include <stddef.h>

#include "qemu/osdep.h"
#include "sysemu/sysemu.h"
#include "qemu-main.h"
#include "monitor/monitor.h"
#include "qemu/module.h"
#include "sysemu/arch_init.h"
#include "block/export.h"
#include "block/export.h"
#include "sysemu/cpus.h"
#include "exec/cpu-common.h"
#include "exec/address-spaces.h"
#include "exec/memory.h"
#include "exec/page-vary.h"

static void initialize_for_module_test(void)
{
    monitor_init_globals_core();
    finalize_target_page_bits();
    qemu_init_cpu_loop(); /* initializes io thread mutex */
    qemu_mutex_lock_iothread();
}

int main(int argc, char *argv[])
{
    int rc = 0;

    initialize_for_module_test();
    module_call_init(MODULE_INIT_QOM);

    g_test_init(&argc, &argv, NULL);
    qmt_register_testcases(qmt_get_test_memory());
    rc = g_test_run();

    monitor_cleanup();
    return rc;
}

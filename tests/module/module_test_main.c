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
    qemu_init_cpu_loop(); // initializes io thread mutex 
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
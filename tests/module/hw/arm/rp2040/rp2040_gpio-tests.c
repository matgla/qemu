#include "module_test_init.h"

#include <stdarg.h>

#include "qemu/osdep.h"
#include "qom/object.h"
#include "hw/irq.h"


#include "hw/gpio/rp2040_gpio.h"

struct Compare;

typedef bool(*comparator_call)(void *expected, void *argument);
typedef void(*release_call)(struct Compare *arg);

typedef struct Compare {
    void *expected; 
    comparator_call function;
    release_call release;
} Compare;


typedef struct Expectation {
    int expected;
    int count;
    int comparators_size;
    Compare **comparators;
    void *obj;
} Expectation;

// static Expectation *get_expectations(void)
// {
//     static Expectation ex = {
//         .expected = 0,
//         .count = 0,
//         .comparators_size = 0,
//         .comparators = NULL,
//         .obj = NULL,
//     };
//     return &ex;
// }

// static bool compare_int(void *expected, void *arg)
// {
//     int *v = (int*)arg;
//     int *e = (int*)expected;
//     return *v == *e;
// }

// static bool compare_null(void *expected, void *arg)
// {
//     return expected == arg;
// }

// static bool compare_int(void *expected, void *arg)
// {
//     int *e = (int *)expected;
//     int *a = (int *)arg;
//     return *e == *a;
// }

// static void do_nothing(Compare *arg)
// {
// }

// static void free_compare(Compare *arg)
// {
//     g_free(arg->expected);
// }


// static Compare* expect_null(void)
// {
//     Compare *compare = g_malloc0(sizeof(Compare));
//     compare->expected = NULL;
//     compare->function = compare_null;
//     compare->release = do_nothing;
//     return compare;
// }

// static Compare* expect_int(int v)
// {
//     Compare *compare = g_malloc0(sizeof(Compare));
//     compare->expected = (int *)g_malloc0(sizeof(int));
//     memcpy(compare->expected, &v, sizeof(int));
//     compare->function = compare_int;
//     compare->release = free_compare;
//     return compare;
// }

// static void elo_irq(void *opaque, int n, int level)
// {
//     fprintf(stderr, "IRQ Called\n");
//     g_assert(get_expectations()->comparators[0]->function(get_expectations()->comparators[0]->expected, opaque));
//     g_assert(get_expectations()->comparators[1]->function(get_expectations()->comparators[1]->expected, &n));
//     g_assert(get_expectations()->comparators[2]->function(get_expectations()->comparators[2]->expected, &level));
// }

// static void verify_and_release(void)
// {
//     Expectation *e = get_expectations();
//     for (int i = 0; i < e->comparators_size; ++i)
//     {
//         Compare *c = e->comparators[i];
//         c->release(c);
//         g_free(c);
//     }
//     g_free(e->comparators);
// }

// static void expect_call(void* ptr, int n, ...)
// {
//     Expectation *e = get_expectations();
//     Expectation new = {
//         .expected = 1,
//         .count = 0,
//         .comparators_size = n,
//         .comparators = NULL,
//         .obj = ptr
//     };


//     va_list vaptr;
//     va_start(vaptr, n);
//     new.comparators = (Compare**)malloc(sizeof(Compare *) * n);
//     for (int i = 0; i < n; ++i)
//     {
//         new.comparators[i] = va_arg(vaptr, Compare*);
//     }
//     va_end(vaptr);
//     *e = new;
// }

// static void test_fail_gpio(void)
// {
//     RP2040GpioState* sut = (RP2040GpioState*)object_new(TYPE_RP2040_GPIO);
//     // qemu_irq irq_trigger = qemu_allocate_irq(elo_irq, NULL, 0);
//     // qdev_connect_gpio_out_named(DEVICE(sut), "out", 0, irq_trigger);
//     // expect_call(&elo_irq, 3, expect_null(), expect_int(0), expect_int(10));
//     // qemu_set_irq(sut->gpio_out[0], 10);
//     // verify_and_release();
//     fprintf(stderr, "unref: %p\n", sut);
//     object_unref(sut);
// }

int module_test_main(int argc, char *argv[])
{
    // g_test_init(&argc, &argv, NULL);
    // g_test_add_func("/rp2040/gpio/should_fail", test_fail_gpio);
    // return g_test_run();
    return 0;
}
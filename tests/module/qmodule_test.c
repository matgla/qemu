/*
 * QModuleTest Framework
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

#include <glib.h>

#include <stdio.h>
#include <string.h>

#include "qemu/rcu.h"
#include "exec/address-spaces.h"

#define QMT_EXPECT_CALL_ANY -1

struct QMTExpectation {
    int matched_count;
    int expected_count;
    int places_count;
    QMTExpectationPlace *places;
    int argument_count;
    QMTCompare *comparators;
    char **expected_argument_print;
    void *return_value;
    QMTExpectationOnCall on_call;
};


struct QMTExpectationPlace {
    const char *file;
    int line;
};

struct QMTExpectations {
    int size;
    QMTExpectationSet *sets;
};

struct QMTRegisteredCall;
typedef struct QMTRegisteredCall QMTRegisteredCall;

struct QMTExpectationSet {
    void *function;
    int expectations_count;
    QMTExpectation *expectations;
    int registered_call_count;
    QMTRegisteredCall *registered_calls;
    int arguments_count;
    QMTPrintCall *printers;
    const char *name;
};

struct QMTRegisteredCall {
    QMTArgument *arguments;
    int argument_count;
};

/*######################################################*/
/*#                   PRIVATE API                      #*/
/*######################################################*/

static void qmt_compare_free(QMTCompare *c)
{
    if (c->release) {
        c->release(c->expected.data);
    }
}

static void qmt_expectation_free(QMTExpectation *e)
{
    g_free(e->places);
    for (int i = 0; i < e->argument_count; ++i) {
        qmt_compare_free(&e->comparators[i]);
        g_free(e->expected_argument_print[i]);
    }
    g_free(e->comparators);
    g_free(e->expected_argument_print);
}

static void qmt_registered_calls_free(QMTRegisteredCall *rc)
{
    for (int i = 0; i < rc->argument_count; ++i) {
        g_free(rc->arguments[i].data);
    }
    g_free(rc->arguments);
}

static void qmt_expectation_set_free(QMTExpectationSet *set)
{
    for (int i = 0; i < set->registered_call_count; ++i) {
        qmt_registered_calls_free(&set->registered_calls[i]);
    }
    g_free(set->registered_calls);
    for (int i = 0; i < set->expectations_count; ++i) {
        qmt_expectation_free(&set->expectations[i]);
    }
    g_free(set->expectations);
    g_free(set->printers);
}

static bool qmt_compare_comparator(const QMTCompare *a, const QMTCompare *b)
{
    return a->expected.size == b->expected.size
        && memcmp(a->expected.data, b->expected.data, a->expected.size) == 0
        && a->function == b->function;
}

static bool qmt_compare_comparators(const QMTCompare *a, const QMTCompare *b,
    const int arguments_count)
{
    for (int i = 0; i < arguments_count; ++i) {
        if (!qmt_compare_comparator(&a[i], &b[i])) {
            return false;
        }
    }
    return true;
}

static bool qmt_compare_expectation_comparators(const QMTExpectation *a,
    const QMTExpectation *b, const int arguments_count)
{
    return qmt_compare_comparators(a->comparators, b->comparators,
        arguments_count);
}

static bool qmt_expectation_contains_place(const QMTExpectation *a,
    const QMTExpectationPlace *place)
{
    for (int i = 0; i < a->places_count; ++i) {
        if (a->places[i].file != place->file) {
            return false;
        } else if (a->places[i].line != place->line) {
            return false;
        }
    }
    return true;
}

static QMTExpectations *qmt_get_expectations(void)
{
    static QMTExpectations ex;
    return &ex;
}

QMTExpectation *_qmt_expect_call(const char *name, const char *file,
    int line, void *func, int n, ...)
{
    QMTExpectation expectation;
    QMTExpectationPlace place = {.file = file, .line = line};
    QMTExpectationSet *set = _qmt_find_expectations_for_function(func);
    if (set == NULL) {
        QMTExpectations *expectations = qmt_get_expectations();
        ++expectations->size;
        expectations->sets = g_renew(QMTExpectationSet, expectations->sets,
            expectations->size);
        set = &expectations->sets[expectations->size - 1];
        set->function = func;
        set->expectations_count = 0;
        set->expectations = NULL;
        set->registered_call_count = 0;
        set->registered_calls = NULL;
        set->arguments_count = n;
        set->name = name;
        set->printers = g_new0(QMTPrintCall, n);
    }


    expectation.expected_count = 1;
    expectation.matched_count = 0;
    expectation.places = g_new0(QMTExpectationPlace, 1);
    expectation.places[0] = place;
    expectation.places_count = 1;
    expectation.return_value = NULL;
    expectation.argument_count = n;
    expectation.on_call = NULL;
    expectation.comparators = g_new0(QMTCompare, n);
    expectation.expected_argument_print = g_new0(char *, n);
    va_list vaptr;
    va_start(vaptr, n);
    for (int i = 0; i < n; ++i) {
        QMTCompare *compare = &expectation.comparators[i];
        QMTExpectationData *e = va_arg(vaptr, QMTExpectationData *);
        *compare = e->compare;
        set->printers[i] = e->printer;
        expectation.expected_argument_print[i] = g_strdup(e->expect_print);
        g_free(e->expect_print);
        g_free(e);
    }
    va_end(vaptr);


    for (int i = 0; i < set->expectations_count; ++i) {
        QMTExpectation *e = &set->expectations[i];
        if (qmt_compare_expectation_comparators(e, &expectation, n)) {
            ++e->expected_count;

            if (!qmt_expectation_contains_place(e, &place)) {
                e->places = g_renew(QMTExpectationPlace, e->places,
                    e->places_count + 1);
                e->places[e->places_count] = place;
                ++e->places_count;
            }

            qmt_expectation_free(&expectation);

            return &set->expectations[i];
        }
    }

    ++set->expectations_count;
    set->expectations = g_renew(QMTExpectation, set->expectations,
        set->expectations_count);
    set->expectations[set->expectations_count - 1] = expectation;

    return &set->expectations[set->expectations_count - 1];
}

QMTExpectationSet *_qmt_find_expectations_for_function(void *function)
{
    QMTExpectations *expectations = qmt_get_expectations();
    if (expectations->sets == NULL) {
        return NULL;
    }

    for (int i = 0; i < expectations->size; ++i) {
        if (expectations->sets[i].function == function) {
            return &expectations->sets[i];
        }
    }
    return NULL;
}

static char *qmt_print_expectation(const QMTExpectation *e,
    const int arguments_count, const char *name)
{
    char *msg = (char *)"";
    char *part = g_strdup_printf("%s(", name);

    for (int j = 0; j < arguments_count; ++j) {
        part = g_strconcat(part, e->expected_argument_print[j], NULL);
        if (j < arguments_count - 1) {
            part = g_strconcat(part, ", ", NULL);
        }
    }
    msg = g_strconcat(msg, part, ")\n", NULL);
    msg = g_strconcat(msg, "From:\n", NULL);
    for (int i = 0; i < e->places_count; ++i) {
        part = g_strdup_printf("  %s:%d\n",
            e->places[i].file, e->places[i].line);
        msg = g_strconcat(msg, part, NULL);
    }

    part = g_strdup_printf("    Expected calls : %d\n    Actual calls   : %d",
        e->expected_count, e->matched_count);
    msg = g_strconcat(msg, part, NULL);
    return msg;
}

static char *qmt_print_expectations(const QMTExpectationSet *set)
{
    char *msg = g_strdup_printf("Expectation list:\n");
    if (set->expectations_count == 0) {
        msg = g_strconcat(msg, "-", NULL);
        return msg;
    }
    for (int i = 0; i < set->expectations_count; ++i) {
        const QMTExpectation *e = &set->expectations[i];
        msg = g_strconcat(msg,
            qmt_print_expectation(e, set->arguments_count, set->name), NULL);
    }
    return msg;
}

static char *qmt_print_call(const char* name, const QMTExpectationSet *set,
    int arguments_count, const QMTArgument *arguments)
{
    char *msg = g_strdup_printf("%s(", name);

    for (int i = 0; i < arguments_count; ++i) {
        msg = g_strconcat(msg, set->printers[i](&arguments[i]), NULL);
        if (i < arguments_count - 1) {
            msg = g_strconcat(msg, ", ", NULL);
        }
    }

    msg = g_strconcat(msg, ")", NULL);
    return msg;
}

static char *qmt_print_registered_calls(const struct QMTExpectationSet *set)
{
    char *registered_calls = NULL;
    char *part = NULL;
    registered_calls = g_strdup_printf("Registered calls:\n");
    for (int i = 0; i < set->registered_call_count; ++i) {
        const struct QMTRegisteredCall *call = &set->registered_calls[i];
        part = g_strdup_printf("  %s(", set->name);
        registered_calls = g_strjoin(NULL, registered_calls, part, NULL);
        for (int j = 0; j < set->arguments_count; ++j) {
            part = set->printers[j](&call->arguments[j]);
            if (j < set->arguments_count - 1) {
                part = g_strconcat(part, ", ", NULL);
            }
            registered_calls = g_strconcat(registered_calls, part, NULL);
        }
        registered_calls = g_strconcat(registered_calls, ")\n", NULL);
    }
    return registered_calls;
}

static char *qmt_print_unknown(const QMTArgument* argument)
{
    char *msg = (char *)("");
    char *part;


    uint8_t *data = (uint8_t *)argument->data;
    msg = g_strdup_printf("{");
    for (int i = 0; i < argument->size; ++i) {
        part = g_strdup_printf("0x%02x", data[i]);
        msg = g_strconcat(msg, part, NULL);
        if (i < argument->size - 1) {
            msg = g_strconcat(msg, ",", NULL);
        }
    }
    msg = g_strconcat(msg, "}", NULL);

    return msg;
}

static void qmt_assert_expectations_msg(const char *name, const char *file,
    int line, const QMTExpectationSet *set, int arguments_count,
    const QMTArgument *arguments)
{
    g_autofree char *expectation_list = (char *)"";
    g_autofree char *output_print = NULL;
    output_print = g_strdup_printf("Unexpected call: ");

    if (set != NULL) {
        output_print = g_strconcat(output_print,
            qmt_print_call(name, set, arguments_count, arguments), "\n", NULL);
        output_print = g_strconcat(output_print,
            qmt_print_expectations(set), "\n", NULL);
        output_print = g_strconcat(output_print,
            qmt_print_registered_calls(set), "\n", NULL);
    } else {
        output_print = g_strdup_printf("%s %s(", output_print, name);
        for (int i = 0; i < arguments_count; ++i) {
            output_print = g_strconcat(output_print,
                qmt_print_unknown(&arguments[i]), NULL);
            if (i < arguments_count - 1) {
                output_print = g_strconcat(output_print, ",", NULL);
            }
        }
        output_print = g_strconcat(output_print, ")", NULL);
        output_print = g_strconcat(output_print, "\nExpectation list:\n  -",
            NULL);
    }

    g_assertion_message(G_LOG_DOMAIN, __FILE__, __LINE__, __func__,
        output_print);
}

static bool qmt_expectation_match(const QMTExpectation *e,
    const QMTArgument *arguments, const int arguments_count)
{
    for (int i = 0; i < arguments_count; ++i) {
        QMTCompare *compare = &e->comparators[i];
        if (!compare->function(&compare->expected, &arguments[i])) {
            return false;
        }
    }
    return true;
}

static void qmt_register_call(QMTExpectationSet *set,
    const int number_of_arguments, const QMTArgument *arguments)
{
    QMTRegisteredCall *call;
    set->registered_calls = g_renew(QMTRegisteredCall, set->registered_calls,
        set->registered_call_count + 1);
    call = &set->registered_calls[set->registered_call_count];
    call->argument_count = number_of_arguments;
    call->arguments = g_new0(QMTArgument, number_of_arguments);
    for (int i = 0; i < number_of_arguments; ++i) {
        call->arguments[i].data = g_malloc0(arguments[i].size);
        memcpy(call->arguments[i].data, arguments[i].data, arguments[i].size);
        call->arguments[i].size = arguments[i].size;
    }
    ++set->registered_call_count;
}

const void *_qmt_process_call(const char *name, const char *file, int line,
    void *func, int n, const QMTArgument *arguments)
{
    QMTExpectationSet *set = _qmt_find_expectations_for_function(func);
    bool matched = false;
    if (set == NULL) {
        qmt_assert_expectations_msg(name, file, line, set, n, arguments);
    }

    for (int i = 0; i < set->expectations_count; ++i) {
        QMTExpectation *e = &set->expectations[i];
        if (qmt_expectation_match(e, arguments, n)) {
            ++e->matched_count;
            matched = true;
            if (e->on_call) {
                e->on_call();
            }
            return e->return_value;
        }
    }

    if (!matched) {
        qmt_assert_expectations_msg(name, file, line, set, n, arguments);
    }

    qmt_register_call(set, n, arguments);
    return NULL;
}

void _qmt_will_return(QMTExpectation *expectation, void *arg, size_t size)
{
    if (expectation->return_value != NULL) {
        g_free(expectation->return_value);
    }
    expectation->return_value = g_malloc0(size);
    memcpy(expectation->return_value, arg, size);
}


/*********************************************/
/*          Test memory management           */
/*********************************************/

MemoryRegion *qmt_get_test_memory(void)
{
    static MemoryRegion test_memory;
    return &test_memory;
}

static void qmt_test_memory_initialize(void)
{
    memory_region_init(qmt_get_test_memory(), NULL, "system", UINT64_MAX);
    address_space_init(&address_space_memory, qmt_get_test_memory(), "memory");
}

static void qmt_test_memory_finalize(void)
{
    object_unparent(OBJECT(qmt_get_test_memory()));
    address_space_destroy(&address_space_memory);
    drain_call_rcu();
    /* Some cleanups is triggered through RCU */
}


void qmt_memory_write(const hwaddr addr, const void *buf, const hwaddr size)
{
    address_space_write(&address_space_memory, addr, MEMTXATTRS_UNSPECIFIED,
        buf, size);
}

void qmt_memory_read(const hwaddr addr, void *buf, const hwaddr size)
{
    address_space_read(&address_space_memory, addr, MEMTXATTRS_UNSPECIFIED,
        buf, size);
}

/*********************************************/
/*              Expectations                 */
/*********************************************/

void qmt_set_expect_calls_count(QMTExpectation *expectation,
    int expected_calls_count)
{
    expectation->expected_count = expected_calls_count;
}

void qmt_set_expect_calls_count_any(QMTExpectation *expectation)
{
    expectation->expected_count = QMT_EXPECT_CALL_ANY;
}

/* Printers */

static char *qmt_print_ptr(const QMTArgument *value)
{
    char *out = NULL;
    if (value->data) {
        out = g_strdup_printf("%p", value->data);
    } else {
        out = g_strdup_printf("NULL");
    }
    return out;
}

static char *qmt_print_int(const QMTArgument *value)
{
    char *out = NULL;
    out = g_strdup_printf("%d", *(int *)value->data);
    return out;
}

/* Comparators */
static bool qmt_compare_ptr(const QMTArgument *lhs, const QMTArgument *rhs)
{
    return lhs->size == rhs->size
        && memcmp(lhs->data, rhs->data, lhs->size) == 0;
}

static bool qmt_compare_int(const QMTArgument *lhs, const QMTArgument *rhs)
{
    return lhs->size == rhs->size && *(int *)(lhs->data) == *(int *)(rhs->data);
}

static bool qmt_compare_any(const QMTArgument *lhs, const QMTArgument *rhs)
{
    return true;
}

/* Expectations */
struct QMTExpectationData *qmt_expect_any(void)
{
    struct QMTExpectationData *e = g_new0(QMTExpectationData, 1);
    e->compare.expected.data = NULL;
    e->compare.expected.size = 0;
    e->compare.function = qmt_compare_any;
    e->compare.release = NULL;
    e->printer = qmt_print_unknown;
    e->expect_print = g_strdup_printf("expect_any()");
    return e;
}

static QMTCompare qmt_expect_ptr_impl(void *value)
{
    QMTCompare compare;
    compare.expected.data = g_malloc0(sizeof(void *));
    compare.expected.size = sizeof(void *);
    compare.function = qmt_compare_ptr;
    compare.release = g_free;
    memcpy(compare.expected.data, &value, sizeof(value));
    return compare;
}

QMTExpectationData *qmt_expect_ptr(void *ptr)
{
    QMTExpectationData *e = g_new0(QMTExpectationData, 1);
    e->compare = qmt_expect_ptr_impl(ptr);
    e->expect_print = g_strdup_printf("expect_ptr(%p)", ptr);
    e->printer = qmt_print_ptr;
    return e;
}


QMTExpectationData *qmt_expect_null(void)
{
    QMTExpectationData *e = g_new0(QMTExpectationData, 1);
    e->compare = qmt_expect_ptr_impl(NULL);
    e->expect_print = g_strdup_printf("expect_null()");
    e->printer = qmt_print_ptr;
    return e;
}

QMTExpectationData *qmt_expect_int(int value)
{
    QMTExpectationData *e = g_new0(QMTExpectationData, 1);
    e->compare.expected.data = g_malloc0(sizeof(value));
    e->compare.expected.size = sizeof(value);
    e->compare.function = qmt_compare_int;
    e->compare.release = g_free;

    e->expect_print = g_strdup_printf("expect_int(%d)", value);
    e->printer = qmt_print_int;
    memcpy(e->compare.expected.data, &value, sizeof(value));
    return e;
}

/*********************************************/
/*            Test case management           */
/*********************************************/

void qmt_initialize(void)
{
    QMTExpectations *expectations = qmt_get_expectations();
    expectations->sets = NULL;
    expectations->size = 0;
    qmt_test_memory_initialize();
}

void qmt_verify_and_clear_expectations(void)
{
    QMTExpectations *expectations = qmt_get_expectations();
    for (int i = 0; i < expectations->size; ++i) {
        QMTExpectationSet *set = &expectations->sets[i];
        for (int j = 0; j < set->expectations_count; ++j) {
            QMTExpectation *expectation = &set->expectations[j];
            if (expectation->expected_count == QMT_EXPECT_CALL_ANY) {
                continue;
            }

            if (expectation->matched_count != expectation->expected_count) {
                g_autofree char *expectation_msg = NULL;
                expectation_msg = g_strdup_printf(
                        "Expectation not fulfilled: ");
                expectation_msg = g_strconcat(expectation_msg,
                    qmt_print_expectation(expectation,
                        set->arguments_count, set->name),
                    NULL);
                expectation_msg = g_strconcat(expectation_msg, "\n",
                    qmt_print_registered_calls(set), NULL);

                g_assertion_message(G_LOG_DOMAIN, __FILE__, __LINE__,
                    set->name, expectation_msg);
            }
        }
    }

    for (int i = 0; i < expectations->size; ++i) {
        qmt_expectation_set_free(&expectations->sets[i]);
    }
    g_free(expectations->sets);
    expectations->size = 0;
    expectations->sets = NULL;
}

void qmt_set_on_call(QMTExpectation *e, QMTExpectationOnCall on_call)
{
    e->on_call = on_call;
}

void qmt_verify_and_release(void)
{
    qmt_verify_and_clear_expectations();
    qmt_test_memory_finalize();
}

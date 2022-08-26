#ifndef _QMODULE_TEST_H
#define _QMODULE_TEST_H

#include <stdbool.h>
#include <stddef.h>

#include "qemu/osdep.h"
#include "exec/memory.h"

/*######################################################*/
/*#                   PRIVATE API                      #*/
/*######################################################*/

/*********************************************/
/*           Forward declarations            */
/*********************************************/

struct QMTArgument;
struct QMTCompare;
struct QMTExpectation;
struct QMTExpectationData;
struct QMTExpectationPlace;
struct QMTExpectations;
struct QMTExpectationSet;


#define _QMODULE_COUNT_ARGS_127( \
      _0,   _1,   _2,   _3,   _4,   _5,   _6,   _7,   _8,   _9, \
     _10,  _11,  _12,  _13,  _14,  _15,  _16,  _17,  _18,  _19, \
     _20,  _21,  _22,  _23,  _24,  _25,  _26,  _27,  _28,  _29, \
     _30,  _31,  _32,  _33,  _34,  _35,  _36,  _37,  _38,  _39, \
     _40,  _41,  _42,  _43,  _44,  _45,  _46,  _47,  _48,  _49, \
     _50,  _51,  _52,  _53,  _54,  _55,  _56,  _57,  _58,  _59, \
     _60,  _61,  _62,  _63,  _64,  _65,  _66,  _67,  _68,  _69, \
     _70,  _71,  _72,  _73,  _74,  _75,  _76,  _77,  _78,  _79, \
     _80,  _81,  _82,  _83,  _84,  _85,  _86,  _87,  _88,  _89, \
     _90,  _91,  _92,  _93,  _94,  _95,  _96,  _97,  _98,  _99, \
    _100, _101, _102, _103, _104, _105, _106, _107, _108, _109, \
    _110, _111, _112, _113, _114, _115, _116, _117, _118, _119, \
    _120, _121, _122, _123, _124, _125, _126, _127, count, ...) count

#define QMODULE_COUNT_ARGS(...) _QMODULE_COUNT_ARGS_127(,##__VA_ARGS__, \
    127, 126, 125, 124, 123, 122, 121, 120, \
    119, 118, 117, 116, 115, 114, 113, 112, 111, 110, \
    109, 108, 107, 106, 105, 104, 103, 102, 101, 100, \
    99,  98,  97,  96,  95,  94,  93,  92,  91,  90, \
    89,  88,  87,  86,  85,  84,  83,  82,  81,  80, \
    79,  78,  77,  76,  75,  74,  73,  72,  71,  70, \
    69,  68,  67,  66,  65,  64,  63,  62,  61,  60, \
    59,  58,  57,  56,  55,  54,  53,  52,  51,  50, \
    49,  48,  47,  46,  45,  44,  43,  42,  41,  40, \
    39,  38,  37,  36,  35,  34,  33,  32,  31,  30, \
    29,  28,  27,  26,  25,  24,  23,  22,  21,  20, \
    19,  18,  17,  16,  15,  14,  13,  12,  11,  10, \
    9,   8,   7,   6,   5,   4,   3,   2,   1,   0)

/**
 * @brief Create expectation for function call
 * 
 * @param name - name of function to create expectation
 * @param file - file where expectation is created
 * @param line - line where expectation is created
 * @param func - pointer to function to be tracked 
 * @param n - number of function arguments verifiers
 * @param ... - function arguments verifiers 
 * @return struct QMTExpectation* 
 */
struct QMTExpectation *_qmt_expect_call(const char *name, const char *file, int line, void *func, int n, ...);

/**
 * @brief Searching expectations for given function 
 * 
 * @param function - pointer to function for which expectations are requested to be returned
 * @return struct QMTExpectationSet* 
 */
struct QMTExpectationSet *_qmt_find_expectations_for_function(void *function);

/**
 * @brief Process call of expected function
 * 
 * @param name - name of function
 * @param file - file where tracked function is created
 * @param line - line where tracked function is created
 * @param func - address of tracked function
 * @param n - number of arguments from function call
 * @param arguments - array of arguments from function call
 */
void _qmt_process_call(const char *name, const char *file, int line, void *func, int n, const struct QMTArgument *arguments);


/*######################################################*/
/*#                   PUBLIC API                       #*/
/*######################################################*/

/*********************************************/
/*          Test memory management           */
/*********************************************/

/**
 * @brief Getter for memory base for tests
 * 
 * @return pointer to memory base created for module tests
 */
MemoryRegion *qmt_get_test_memory(void);

/**
 * @brief Writes to test memory at specific address
 * 
 * @param addr - memory address to write
 * @param buf - data for a write operation
 * @param size - size of data in bytes
 */
void qmt_memory_write(const hwaddr addr, const void *buf, const hwaddr size); 

/**
 * @brief Reads from test memory at specific address
 * 
 * @param addr - memory address to read
 * @param buf - buffer for a read operation
 * @param size - size of data to be read
 */
void qmt_memory_read(const hwaddr addr, void *buf, const hwaddr size); 

/*********************************************/
/*              Expectations                 */
/*********************************************/

/**
 * @brief Set number of expected calls for QMTExpectation
 * 
 * @param expectation - expectation to be modified
 * @param expected_calls_count - expected number of calls
 */
void qmt_set_expect_calls_count(struct QMTExpectation *expectation, int expected_calls_count);

/**
 * @brief Set number of expected calls to any value
 * 
 * @param expectation - expectation to be modified
 */
void qmt_set_expect_calls_count_any(struct QMTExpectation *expectation);


/**
 * @brief MACRO to automatically fills arguments for _qmt_expect_call
 * 
 * @param func - function to be tracked
 * @param ... - verifiers for function arguments
 */
#define QMT_EXPECT_CALL(func, ...) _qmt_expect_call(#func, __FILE__, __LINE__, func, QMODULE_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)


/**
 * @brief Expect any argument
 * 
 * @return struct QMTExpectationData* 
 */
struct QMTExpectationData *qmt_expect_any(void);

/**
 * @brief Expect argument to be pointer with value
 * 
 * @param - expected value of pointer
 * @return struct QMTExpectationData* 
 */
struct QMTExpectationData *qmt_expect_ptr(void *ptr);

/**
 * @brief Expect argument to be pointer with null value
 * 
 * @return struct QMTExpectationData* 
 */
struct QMTExpectationData *qmt_expect_null(void);

/**
 * @brief Expect argument to be int with value
 * 
 * @param value - expected value of argument
 * @return struct QMTExpectationData* 
 */
struct QMTExpectationData *qmt_expect_int(int value);

/*********************************************/
/*            Test case management           */
/*********************************************/

/**
 * @brief Entry point for module tests registration. Needs to be implemented by test suite.
 * 
 * @param test_memory - Test memory to be used for read/write operations, provided by module test framework
 */
void qmt_register_testcases(MemoryRegion *test_memory);

/**
 * @brief Module tests initialization. Needs to be called at each test begin.
 * 
 */
void qmt_initialize(void);

/**
 * @brief Module test finalization. 
 * Needs to be called at each test end. 
 * Verifies created expectation and releases memory allocated by @ref qmt_expect_call.
 * Make finalization of created device tree starting from test_memory. 
 * 
 */
void qmt_verify_and_release(void);

/**
 * @brief Verifies already created expecations and frees memory from them.
 * Used to be called within test case. 
 * 
 */
void qmt_verify_and_clear_expectations(void);

typedef struct QMTArgument {
    int size; 
    void *data;
} QMTArgument;


/*********************************************/
/*         Fake functions generators         */
/*********************************************/

#define QMT_DEFINE_FAKE_VOID3(name, t0, t1, t2) \
    void name(t0 a0, t1 a1, t2 a2) { \
        QMTArgument args[3] = { \
            {.size = sizeof(t0), .data = &a0}, \
            {.size = sizeof(t1), .data = &a1}, \
            {.size = sizeof(t2), .data = &a2}, \
        }; \
        _qmt_process_call(#name, __FILE__, __LINE__, &name, 3, args);\
    }  

#endif

#ifndef TEST_CASES_H
#define TEST_CASES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TC_NO_RESULT,
    TC_PASS,
    TC_FAIL,
    TC_WARNING,
    TC_IN_PROGRESS,
    TC_RETRY
} TestCaseState;


void test_cases_init(void);
void test_cases_update(void);
TestCaseState test_cases_eval(void);
void test_cases_start(uint8_t idx);
void test_cases_end(void);
void test_cases_monitor_inputs(void);

extern TestCaseState tcResults[5];
extern uint8_t currentTest;
extern bool testActive;

extern const size_t NUM_TEST_CASES;

#endif // TEST_CASES_H

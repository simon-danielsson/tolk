#include "tests.h"

#ifdef TEST

uint _tests_failures = 0, _tests_total = 0;

internal void test1(void) { test_assert("test1", 0); }
internal void test2(void) { test_assert("test2", 0); }
internal void test3(void) { test_assert("test3", 1); }

typedef void (*test_fn)(void);

global test_fn tests[] = {
    test1,
    test2,
    test3,
    NULL,
};

__attribute__((constructor)) internal void _run_tests(void) {
    TEST_DIV;
    while (tests[_tests_total]) {
        tests[_tests_total]();
        _tests_total++;
    }
    TEST_DIV;
    printf("Failure rate: %.02f%%\n",
            ((float)_tests_failures / _tests_total) * 100);
    exit(0);
}

#endif

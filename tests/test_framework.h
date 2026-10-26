/*
 * test_framework.h — minimal unit-test framework
 *
 * Provides CHECK() assertions and a test-runner structure.
 * No external libraries required.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>

/* Global counters updated by the CHECK macro */
extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

/**
 * CHECK — assert that 'expr' is true.
 * Prints PASS/FAIL with file, line, and expression.
 */
#define CHECK(expr) do {                                         \
    g_tests_run++;                                               \
    if (expr) {                                                  \
        g_tests_passed++;                                        \
        printf("  [PASS] %s\n", #expr);                         \
    } else {                                                     \
        g_tests_failed++;                                        \
        printf("  [FAIL] %s  (at %s:%d)\n", #expr,             \
               __FILE__, __LINE__);                              \
    }                                                            \
} while (0)

/**
 * CHECK_EQ — assert that two integer values are equal.
 */
#define CHECK_EQ(a, b) do {                                      \
    g_tests_run++;                                               \
    if ((a) == (b)) {                                            \
        g_tests_passed++;                                        \
        printf("  [PASS] %s == %s  (%d)\n", #a, #b, (int)(a)); \
    } else {                                                     \
        g_tests_failed++;                                        \
        printf("  [FAIL] %s == %s  (got %d, expected %d)  "     \
               "(at %s:%d)\n",                                   \
               #a, #b, (int)(a), (int)(b), __FILE__, __LINE__); \
    }                                                            \
} while (0)

/**
 * TEST_SECTION — print a section header.
 */
#define TEST_SECTION(name) \
    printf("\n=== %s ===\n", name)

/**
 * test_summary — print final pass/fail counts.
 * Returns 0 if all tests passed, 1 otherwise.
 */
static inline int test_summary(void)
{
    printf("\n");
    printf("Results: %d/%d tests passed", g_tests_passed, g_tests_run);
    if (g_tests_failed == 0) {
        printf("  — ALL PASSED\n");
    } else {
        printf("  — %d FAILED\n", g_tests_failed);
    }
    return (g_tests_failed == 0) ? 0 : 1;
}

#endif /* TEST_FRAMEWORK_H */

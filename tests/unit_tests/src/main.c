#include <zephyr/ztest.h>
#include "app_logic.h"

ZTEST(logic_tests, test_celsius_to_fahrenheit)
{
    /* Test 0C -> 32F */
    zassert_within(celsius_to_fahrenheit(0.0f), 32.0f, 0.01f, "0C should be 32F");
    /* Test 100C -> 212F */
    zassert_within(celsius_to_fahrenheit(100.0f), 212.0f, 0.01f, "100C should be 212F");
    /* Test -40C -> -40F */
    zassert_within(celsius_to_fahrenheit(-40.0f), -40.0f, 0.01f, "-40C should be -40F");
}

ZTEST(logic_tests, test_history_index_wrapping)
{
    /* Normal increment */
    zassert_equal(get_next_history_index(0, 5), 1, "0 -> 1");
    /* Wrapping */
    zassert_equal(get_next_history_index(4, 5), 0, "4 -> 0 (wrap)");
    /* Empty case */
    zassert_equal(get_next_history_index(0, 0), 0, "Empty list safety");
}

ZTEST_SUITE(logic_tests, NULL, NULL, NULL, NULL, NULL);

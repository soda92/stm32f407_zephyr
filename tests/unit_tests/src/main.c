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

ZTEST(logic_tests, test_display_formatting)
{
    char buf[32];
    
    /* Test Celsius Formatting */
    format_temp_display(buf, sizeof(buf), 25.5f, false);
    zassert_str_equal(buf, "Temp: 25.50 C", "C format failed [%s]", buf);

    /* Test Fahrenheit Formatting */
    format_temp_display(buf, sizeof(buf), 0.0f, true);
    zassert_str_equal(buf, "Temp: 32.00 F", "F format failed [%s]", buf);

    /* Test History Header */
    format_history_header(buf, sizeof(buf), 0, 10);
    zassert_str_equal(buf, "- HIST [1/10] -", "Header format failed");
}

ZTEST(logic_tests, test_record_validation)
{
    float valid = 25.0f;
    float invalid;
    uint32_t raw_invalid = 0xFFFFFFFF;
    memcpy(&invalid, &raw_invalid, 4);

    zassert_true(is_valid_record(valid), "Normal float should be valid");
    zassert_false(is_valid_record(invalid), "0xFFFFFFFF should be invalid");
}

ZTEST_SUITE(logic_tests, NULL, NULL, NULL, NULL, NULL);

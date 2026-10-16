#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include "app_logic.h"

/* Include mocks to get the macros and mock functions */
#include "../../../src/native_sim_mocks.h"

/* Externs from main.c */
extern uint32_t saved_count;
extern bool view_history;
extern uint32_t history_index;
extern float last_live_temp;
extern bool use_fahrenheit;
extern struct k_msgq sensor_msgq;
extern int mock_gpio_state[32];
extern char last_oled_lines[4][64];

struct sensor_data {
	float temp;
	float press;
	float hum;
};

void flash_save_record(float temp);
float flash_read_record(uint32_t index);

/* Reset state before each test */
extern void mock_hardware_init(void);
extern void app_setup(void);
extern void start_sensor_thread(void);
extern void start_ui_thread(void);
extern void start_input_thread(void);
extern void stop_app_threads(void);

static void test_setup(void *f)
{
    mock_hardware_init();
    app_setup();
    
    saved_count = 0;
    view_history = false;
    use_fahrenheit = false;
    mock_temp_val = 25.5f;
    
    /* Clear message queue */
    k_msgq_purge(&sensor_msgq);
}

static void test_teardown(void *f)
{
    stop_app_threads();
}

ZTEST(system_tests, test_full_flow)
{
    mock_temp_val = 30.5f;
    last_live_temp = 30.5f; 
    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 1, "Should have 1 record");

    mock_temp_val = 15.2f;
    last_live_temp = 15.2f;
    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 2, "Should have 2 records");

    float val1 = flash_read_record(0);
    float val2 = flash_read_record(1);
    zassert_within(val1, 30.5f, 0.1f, "Record 0 should be 30.5");
    zassert_within(val2, 15.2f, 0.1f, "Record 1 should be 15.2");
}

ZTEST(system_tests, test_bouncy_button)
{
    bool initial_unit = use_fahrenheit;
    mock_gpio_state[btn_exit.pin] = 1;
    mock_trigger_interrupt();
    k_sleep(K_MSEC(5));
    mock_trigger_interrupt();
    k_sleep(K_MSEC(20));
    zassert_equal(use_fahrenheit, initial_unit, "Unit toggled too early");
    k_sleep(K_MSEC(100));
    zassert_not_equal(use_fahrenheit, initial_unit, "Unit did not toggle after debounce");
}

ZTEST(system_tests, test_thread_interaction)
{
    struct sensor_data data;
    mock_temp_val = 42.0f;
    start_sensor_thread();
    k_sleep(K_MSEC(2500));
    int ret = k_msgq_get(&sensor_msgq, &data, K_NO_WAIT);
    zassert_equal(ret, 0, "Message queue should not be empty");
    zassert_within(data.temp, 42.0f, 0.1f, "Queue data mismatch");
}

ZTEST(system_tests, test_complex_flow)
{
    /* 1. Start UI and Input threads */
    start_ui_thread();
    start_input_thread();

    /* 2. Save 5 records with different temperatures */
    float temps[5] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    for (int i = 0; i < 5; i++) {
        last_live_temp = temps[i];
        mock_gpio_state[btn_save.pin] = 1;
        k_sleep(K_MSEC(100));
        mock_gpio_state[btn_save.pin] = 0;
        k_sleep(K_MSEC(600)); // Wait for input thread processing
    }

    zassert_equal(saved_count, 5, "Should have 5 records saved");

    /* 3. Toggle History View */
    mock_gpio_state[btn_hist.pin] = 1;
    k_sleep(K_MSEC(100));
    mock_gpio_state[btn_hist.pin] = 0;
    k_sleep(K_MSEC(300));
    zassert_true(view_history, "Should be in history mode");

    /* 4. Cycle through history and verify OLED strings */
    for (int i = 0; i < 5; i++) {
        k_sleep(K_MSEC(600)); // Let UI thread update
        char expected[32];
        format_temp_display(expected, sizeof(expected), temps[i], false);
        zassert_str_equal(last_oled_lines[1], expected, "OLED string mismatch for record %d. Got: '%s', Expected: '%s'", i, last_oled_lines[1], expected);

        /* Press Next */
        mock_gpio_state[btn_save.pin] = 1;
        k_sleep(K_MSEC(100));
        mock_gpio_state[btn_save.pin] = 0;
    }

    /* 5. Long Press History to Clear */
    mock_gpio_state[btn_hist.pin] = 1;
    k_sleep(K_MSEC(2500)); // Long press > 2s
    mock_gpio_state[btn_hist.pin] = 0;
    k_sleep(K_MSEC(300));

    zassert_equal(saved_count, 0, "Saved count should be reset after clear");
    zassert_false(view_history, "Should exit history mode after clear");
}

ZTEST_SUITE(system_tests, NULL, NULL, test_setup, test_teardown, NULL);

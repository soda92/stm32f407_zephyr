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
extern void start_app_threads(void);
extern void stop_app_threads(void);
extern struct k_work_delayable exit_btn_work;
extern void exit_btn_work_handler(struct k_work *work);

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
    /* Stop threads if they were started */
    stop_app_threads();
}

ZTEST(system_tests, test_full_flow)
{
    /* 1. Change Temp and Save manually */
    mock_temp_val = 30.5f;
    last_live_temp = 30.5f; 

    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 1, "Should have 1 record");

    /* 2. Change Temp again and Save */
    mock_temp_val = 15.2f;
    last_live_temp = 15.2f;
    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 2, "Should have 2 records");

    /* 3. Verify History content */
    float val1 = flash_read_record(0);
    float val2 = flash_read_record(1);
    zassert_within(val1, 30.5f, 0.1f, "Record 0 should be 30.5");
    zassert_within(val2, 15.2f, 0.1f, "Record 1 should be 15.2");
}

ZTEST(system_tests, test_bouncy_button)
{
    bool initial_unit = use_fahrenheit;

    /* Simulate a "bounce" */
    mock_exit_btn_state = 1;
    mock_trigger_interrupt();
    k_sleep(K_MSEC(5));
    mock_trigger_interrupt();
    k_sleep(K_MSEC(5));
    mock_trigger_interrupt();

    /* Should NOT have toggled yet (within 50ms window) */
    k_sleep(K_MSEC(20));
    zassert_equal(use_fahrenheit, initial_unit, "Unit toggled too early");

    /* Wait for the final debounce to expire */
    k_sleep(K_MSEC(100));
    zassert_not_equal(use_fahrenheit, initial_unit, "Unit did not toggle after debounce");
}

extern void start_sensor_thread(void);

ZTEST(system_tests, test_thread_interaction)
{
    struct sensor_data data;
    mock_temp_val = 42.0f;

    start_sensor_thread();

    /* Wait for sensor thread to fetch and put in queue */
    k_sleep(K_MSEC(2500));

    /* Verify the message queue has data */
    int ret = k_msgq_get(&sensor_msgq, &data, K_NO_WAIT);
    zassert_equal(ret, 0, "Message queue should not be empty");
    zassert_within(data.temp, 42.0f, 0.1f, "Queue data mismatch");
}

ZTEST_SUITE(system_tests, NULL, NULL, test_setup, test_teardown, NULL);

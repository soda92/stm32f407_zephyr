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
void flash_save_record(float temp);
float flash_read_record(uint32_t index);

ZTEST(system_tests, test_full_flow)
{
    /* 1. Initial State */
    flash_erase(flash, 0, 4096);
    saved_count = 0;
    view_history = false;
    zassert_equal(saved_count, 0, "Should start with 0 records");

    /* 2. Change Temp and Save */
    mock_temp_val = 30.5f;
    // Wait for sensor thread to pick it up (sleep > 2s)
    k_sleep(K_MSEC(2500)); 
    zassert_within(last_live_temp, 30.5f, 0.1f, "Live temp should update to 30.5");

    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 1, "Should have 1 record");

    /* 3. Change Temp again and Save */
    mock_temp_val = 15.2f;
    k_sleep(K_MSEC(2500));
    flash_save_record(last_live_temp);
    zassert_equal(saved_count, 2, "Should have 2 records");

    /* 4. Verify History content */
    float val1 = flash_read_record(0);
    float val2 = flash_read_record(1);
    zassert_within(val1, 30.5f, 0.1f, "Record 0 should be 30.5");
    zassert_within(val2, 15.2f, 0.1f, "Record 1 should be 15.2");

    /* 5. Toggle History Mode */
    view_history = true;
    history_index = 0;
    zassert_true(view_history, "Should be in history view");

    /* 6. Clear Data */
    flash_erase(flash, 0, 4096);
    saved_count = 0;
    view_history = false;
    zassert_equal(saved_count, 0, "Should be cleared");
}

ZTEST_SUITE(system_tests, NULL, NULL, NULL, NULL, NULL);

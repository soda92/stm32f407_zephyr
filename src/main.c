#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "app_logic.h"

LOG_MODULE_REGISTER(main);

/* --- Configuration --- */
#define STACK_SIZE 2048
#define PRIORITY 7
#define DEBOUNCE_DELAY_MS 50
#define MAX_RECORDS 1024
#define RECORD_SIZE sizeof(float)

/* --- Data Structures --- */
struct sensor_data {
	float temp;
	float press;
	float hum;
};

/* --- Global State --- */
#ifdef CONFIG_ZTEST
bool use_fahrenheit = false;
bool blink_led = true;
bool view_history = false;
uint32_t saved_count = 0;
uint32_t history_index = 0;
float last_live_temp = 0.0f;
#else
static bool use_fahrenheit = false;
static bool blink_led = true;
static bool view_history = false;
static uint32_t saved_count = 0;
static uint32_t history_index = 0;
static float last_live_temp = 0.0f;
#endif

/* --- Mock State --- */
#ifdef CONFIG_BOARD_NATIVE_SIM
float mock_temp_val = 25.5f;
uint8_t mock_flash_storage[4096];
#ifdef CONFIG_ZTEST
const struct device *flash = (void*)0x3;
#endif
#endif

/* --- Message Queues --- */
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

/* --- Hardware Mocks (for native_sim) --- */
#include "native_sim_mocks.h"

/* --- Hardware Devices (Real) --- */
#ifndef CONFIG_BOARD_NATIVE_SIM
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *bme280  = DEVICE_DT_GET(DT_ALIAS(temp0));
static const struct device *display = DEVICE_DT_GET(DT_ALIAS(oled0));
static const struct device *flash   = DEVICE_DT_GET(DT_ALIAS(flash0));

/* Buttons */
static const struct gpio_dt_spec btn_exit = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_save = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_led  = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_hist = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);
#endif

/* --- Flash Logic --- */
void flash_init_count(void)
{
	float val;
	saved_count = 0;
	while (saved_count < MAX_RECORDS) {
		flash_read(flash, saved_count * RECORD_SIZE, &val, RECORD_SIZE);
		if (!is_valid_record(val)) break;
		saved_count++;
	}
	LOG_INF("Found %d saved records", saved_count);
}

void flash_save_record(float temp)
{
	if (saved_count >= MAX_RECORDS) {
		LOG_WRN("Flash full, erasing history...");
		flash_erase(flash, 0, 4096);
		saved_count = 0;
	}
	flash_write(flash, saved_count * RECORD_SIZE, &temp, RECORD_SIZE);
	saved_count++;
	LOG_INF("Saved Temp %.2f to flash at index %d", (double)temp, saved_count - 1);
}

float flash_read_record(uint32_t index)
{
	float temp = 0.0f;
	if (index < saved_count) {
		flash_read(flash, index * RECORD_SIZE, &temp, RECORD_SIZE);
	}
	return temp;
}

/* --- Button Handlers --- */
struct k_work_delayable exit_btn_work;
void exit_btn_work_handler(struct k_work *work)
{
	if (gpio_pin_get_dt(&btn_exit)) {
		use_fahrenheit = !use_fahrenheit;
		LOG_INF("UI: Toggle C/F");
	}
}

static struct gpio_callback exit_btn_cb_data;
void exit_btn_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&exit_btn_work, K_MSEC(DEBOUNCE_DELAY_MS));
}

/* --- Producer Thread --- */
void sensor_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_value temp;
	struct sensor_data data = {0};
	while (1) {
		if (sensor_sample_fetch(bme280) == 0) {
			sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			data.temp = (float)sensor_value_to_double(&temp);
			last_live_temp = data.temp;
			k_msgq_put(&sensor_msgq, &data, K_NO_WAIT);
		}
		k_msleep(2000);
	}
}

/* --- UI/OLED Thread --- */
void ui_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_data data;
	char str[32];
	
	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready");
		return;
	}

	cfb_framebuffer_init(display);
	cfb_framebuffer_clear(display, true);
	display_blanking_off(display);
	/* Use the first available font */
	cfb_print(display, "ZEPHYR PORT", 0, 0);
	cfb_framebuffer_finalize(display);
	/* Find the smallest font (usually index 0) */
	int num_fonts = cfb_get_numof_fonts(display);
	for (int i = 0; i < num_fonts; i++) {
		uint8_t font_width, font_height;
		cfb_get_font_size(display, i, &font_width, &font_height);
		LOG_INF("Font[%d]: %dx%d", i, font_width, font_height);
	}
	cfb_framebuffer_set_font(display, 0);

	while (1) {
		k_msgq_get(&sensor_msgq, &data, K_MSEC(500));
		cfb_framebuffer_clear(display, false);

		if (!view_history) {
			cfb_print(display, "-LIVE VIEW-", 0, 0);
			format_temp_display(str, sizeof(str), last_live_temp, use_fahrenheit);
			cfb_print(display, str, 0, 15);
			cfb_print(display, "PA1:SAVE PC5:HIST", 0, 34);
		} else {
			float h_temp = flash_read_record(history_index);
			format_history_header(str, sizeof(str), history_index, saved_count);
			cfb_print(display, str, 0, 0);
			format_temp_display(str, sizeof(str), h_temp, use_fahrenheit);
			cfb_print(display, str, 0, 12);
			cfb_print(display, "PA1:NEXT PC5:EXIT", 0, 54);
		}

		cfb_framebuffer_finalize(display);
		if (blink_led) gpio_pin_toggle_dt(&led);
	}
}

/* --- Input Thread --- */
void input_thread_entry(void *p1, void *p2, void *p3)
{
	while (1) {
		if (gpio_pin_get_dt(&btn_led)) {
			blink_led = !blink_led;
			if (!blink_led) gpio_pin_set_dt(&led, 0);
			k_msleep(500);
		}
		if (gpio_pin_get_dt(&btn_save)) {
			if (!view_history) flash_save_record(last_live_temp);
			else history_index = get_next_history_index(history_index, saved_count);
			k_msleep(500);
		}
		if (gpio_pin_get_dt(&btn_hist)) {
			uint32_t start = k_uptime_get_32();
			while (gpio_pin_get_dt(&btn_hist)) {
				if (k_uptime_get_32() - start > 2000) {
					LOG_INF("Erasing History...");
					flash_erase(flash, 0, 4096);
					saved_count = 0; view_history = false;
					break;
				}
				k_msleep(10);
			}
			view_history = !view_history;
			if (view_history && saved_count > 0) history_index = 0;
			k_msleep(200);
		}
		k_msleep(50);
	}
}

K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(ui_tid,     STACK_SIZE, ui_thread_entry,     NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(input_tid,  STACK_SIZE, input_thread_entry,  NULL, NULL, NULL, PRIORITY, 0, 0);

#ifndef CONFIG_ZTEST
int main(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&btn_exit, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_save, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_led,  GPIO_INPUT);
	gpio_pin_configure_dt(&btn_hist, GPIO_INPUT);

	flash_init_count();

	k_work_init_delayable(&exit_btn_work, exit_btn_work_handler);
	gpio_pin_interrupt_configure_dt(&btn_exit, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&exit_btn_cb_data, exit_btn_isr, BIT(btn_exit.pin));
	gpio_add_callback(btn_exit.port, &exit_btn_cb_data);

	return 0;
}
#endif

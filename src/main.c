#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main);

/* --- Configuration --- */
#define STACK_SIZE 2048
#define PRIORITY 7
#define DEBOUNCE_DELAY_MS 50

/* --- Data Structures --- */
struct sensor_data {
	float temp;
	float press;
	float hum;
};

/* --- Global State --- */
static bool use_fahrenheit = false;
static bool blink_led = true;
static char last_button_msg[20] = "None";

/* --- Message Queues --- */
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

/* --- Hardware Devices --- */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *bme280  = DEVICE_DT_GET(DT_ALIAS(temp0));
static const struct device *display = DEVICE_DT_GET(DT_ALIAS(oled0));
static const struct device *flash   = DEVICE_DT_GET(DT_ALIAS(flash0));

/* Buttons */
static const struct gpio_dt_spec btn_exit = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_save = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_led  = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_hist = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

/* --- Debounce Logic (Work Queue) --- */
struct k_work_delayable exit_btn_work;

void exit_btn_work_handler(struct k_work *work)
{
	if (gpio_pin_get_dt(&btn_exit) == 1) {
		use_fahrenheit = !use_fahrenheit;
		snprintf(last_button_msg, sizeof(last_button_msg), "EXIT (C/F)");
		LOG_INF("Debounced: Toggle C/F -> %s", use_fahrenheit ? "F" : "C");
	}
}

static struct gpio_callback exit_btn_cb_data;
void exit_btn_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&exit_btn_work, K_MSEC(DEBOUNCE_DELAY_MS));
}

/* --- Producer Thread: Sensor Reading --- */
void sensor_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_value temp, press, hum;
	struct sensor_data data;

	if (!device_is_ready(bme280)) return;

	while (1) {
		if (sensor_sample_fetch(bme280) == 0) {
			sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &press);
			sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &hum);

			data.temp  = (float)sensor_value_to_double(&temp);
			data.press = (float)sensor_value_to_double(&press);
			data.hum   = (float)sensor_value_to_double(&hum);

			k_msgq_put(&sensor_msgq, &data, K_NO_WAIT);
		}
		k_msleep(2000);
	}
}

/* --- OLED/UI Thread using CFB --- */
void ui_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_data data = {0};
	char str[32];

	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready");
		return;
	}

	if (cfb_framebuffer_init(display)) {
		LOG_ERR("Framebuffer initialization failed!");
		return;
	}

	cfb_framebuffer_clear(display, true);
	cfb_display_blanking_off(display);

	/* Use the first available font */
	cfb_print(display, "ZEPHYR PORT", 0, 0);
	cfb_framebuffer_finalize(display);

	while (1) {
		/* Wait for data from sensor thread */
		k_msgq_get(&sensor_msgq, &data, K_FOREVER);

		float display_temp = use_fahrenheit ? (data.temp * 9.0f / 5.0f + 32.0f) : data.temp;

		cfb_framebuffer_clear(display, false);
		
		snprintf(str, sizeof(str), "Temp: %.2f %c", (double)display_temp, use_fahrenheit ? 'F' : 'C');
		cfb_print(display, str, 0, 0);

		snprintf(str, sizeof(str), "Pres: %.1f hPa", (double)data.press);
		cfb_print(display, str, 0, 16);

		snprintf(str, sizeof(str), "Humi: %.1f %%", (double)data.hum);
		cfb_print(display, str, 0, 32);

		snprintf(str, sizeof(str), "Btn: %s", last_button_msg);
		cfb_print(display, str, 0, 48);

		cfb_framebuffer_finalize(display);

		if (blink_led) {
			gpio_pin_toggle_dt(&led);
		}
	}
}

/* --- Input Polling (For other buttons) --- */
void input_thread_entry(void *p1, void *p2, void *p3)
{
	while (1) {
		if (gpio_pin_get_dt(&btn_led)) {
			blink_led = !blink_led;
			snprintf(last_button_msg, sizeof(last_button_msg), "LED BLINK");
			if (!blink_led) gpio_pin_set_dt(&led, 0);
			k_msleep(500); 
		}
		
		if (gpio_pin_get_dt(&btn_save)) {
			snprintf(last_button_msg, sizeof(last_button_msg), "SAVE (FLASH)");
			k_msleep(500);
		}

		if (gpio_pin_get_dt(&btn_hist)) {
			snprintf(last_button_msg, sizeof(last_button_msg), "HISTORY");
			k_msleep(500);
		}
		k_msleep(50);
	}
}

/* --- Thread Definitions --- */
K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(ui_tid,     STACK_SIZE, ui_thread_entry,     NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(input_tid,  STACK_SIZE, input_thread_entry,  NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	LOG_INF("Zephyr Full Logic Port (CFB) Started");

	/* Initialize Hardware */
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&btn_exit, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_save, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_led,  GPIO_INPUT);
	gpio_pin_configure_dt(&btn_hist, GPIO_INPUT);

	/* Setup Interrupt for Exit Button (PA0) */
	k_work_init_delayable(&exit_btn_work, exit_btn_work_handler);
	gpio_pin_interrupt_configure_dt(&btn_exit, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&exit_btn_cb_data, exit_btn_isr, BIT(btn_exit.pin));
	gpio_add_callback(btn_exit.port, &exit_btn_cb_data);

	return 0;
}

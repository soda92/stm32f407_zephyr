#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main);

/* --- Configuration --- */
#define SENSOR_SLEEP_MS 2000
#define UI_SLEEP_MS 500
#define STACK_SIZE 2048
#define PRIORITY 7

/* --- Data Structure --- */
struct sensor_data {
	float temp;
	float press;
	float hum;
};

/* --- Message Queue --- */
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

/* --- Hardware Devices --- */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *bme280  = DEVICE_DT_GET(DT_ALIAS(temp0));
static const struct device *display = DEVICE_DT_GET(DT_ALIAS(oled0));

/* Buttons */
static const struct gpio_dt_spec btn_exit = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn_save = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec btn_led  = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec btn_hist = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

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
		k_msleep(SENSOR_SLEEP_MS);
	}
}

/* --- Input Thread: Button Polling --- */
void input_thread_entry(void *p1, void *p2, void *p3)
{
	/* Configure buttons */
	gpio_pin_configure_dt(&btn_exit, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_save, GPIO_INPUT);
	gpio_pin_configure_dt(&btn_led,  GPIO_INPUT);
	gpio_pin_configure_dt(&btn_hist, GPIO_INPUT);

	while (1) {
		if (gpio_pin_get_dt(&btn_exit)) LOG_INF("Button: EXIT Pressed");
		if (gpio_pin_get_dt(&btn_save)) LOG_INF("Button: SAVE Pressed");
		if (gpio_pin_get_dt(&btn_led))  LOG_INF("Button: LED Pressed");
		if (gpio_pin_get_dt(&btn_hist)) LOG_INF("Button: HIST Pressed");
		
		k_msleep(100);
	}
}

/* --- OLED Thread: Display Update --- */
void oled_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_data data = {0};
	char msg[32];

	if (!device_is_ready(display)) return;

	/* Simple OLED text output would usually use LVGL, but for a 
	   quick port we'll just log and assume the display loop is 
	   waiting for a real UI framework. */
	
	while (1) {
		/* Get latest data from queue without waiting */
		k_msgq_get(&sensor_msgq, &data, K_FOREVER);

		LOG_INF("Display Update: T=%.2f, P=%.1f, H=%.1f", 
			(double)data.temp, (double)data.press, (double)data.hum);

		/* In a real app, you'd call lv_label_set_text here */
		
		gpio_pin_toggle_dt(&led);
		k_msleep(UI_SLEEP_MS);
	}
}

/* --- Thread Definitions --- */
K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);

K_THREAD_DEFINE(input_tid, STACK_SIZE, input_thread_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);

K_THREAD_DEFINE(oled_tid, STACK_SIZE, oled_thread_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	LOG_INF("Zephyr Full Port Started");
	return 0;
}

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

/* --- Configuration --- */
#define SENSOR_SLEEP_MS 2000
#define STACK_SIZE 1024
#define PRIORITY 7

/* --- Data Structure --- */
struct sensor_data {
	float temp;
	float press;
	float hum;
};

/* --- Message Queue --- */
/* Define a queue that can hold 10 'sensor_data' items */
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

/* --- Hardware Specs --- */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *bme280  = DEVICE_DT_GET(DT_ALIAS(temp0));

/* --- Producer Thread: Sensor Reading --- */
void sensor_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_value temp, press, hum;
	struct sensor_data data;

	if (!device_is_ready(bme280)) {
		LOG_ERR("BME280 not ready");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(bme280) == 0) {
			sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &press);
			sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &hum);

			data.temp  = (float)sensor_value_to_double(&temp);
			data.press = (float)sensor_value_to_double(&press);
			data.hum   = (float)sensor_value_to_double(&hum);

			/* Send to queue (Wait forever if full) */
			k_msgq_put(&sensor_msgq, &data, K_FOREVER);
			LOG_DBG("Producer: Sent data to queue");
		}
		
		k_msleep(SENSOR_SLEEP_MS);
	}
}

/* --- Consumer Thread: Data Processing & LED --- */
void processing_thread_entry(void *p1, void *p2, void *p3)
{
	struct sensor_data received_data;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED not ready");
		return;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	while (1) {
		/* Wait for data from queue */
		if (k_msgq_get(&sensor_msgq, &received_data, K_FOREVER) == 0) {
			LOG_INF("Consumer: T=%.2f C, P=%.2f kPa, H=%.2f %%",
				(double)received_data.temp, 
				(double)received_data.press, 
				(double)received_data.hum);
			
			gpio_pin_toggle_dt(&led);
		}
	}
}

/* --- Static Thread Definitions --- */
K_THREAD_DEFINE(sensor_tid, STACK_SIZE, sensor_thread_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);

K_THREAD_DEFINE(processing_tid, STACK_SIZE, processing_thread_entry, NULL, NULL, NULL,
		PRIORITY, 0, 0);

/* Main thread can stay idle or do other things */
int main(void)
{
	LOG_INF("Zephyr Multi-threaded App Started");
	return 0;
}

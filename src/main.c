#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main);

/* Aliases from devicetree */
#define LED0_NODE  DT_ALIAS(led0)
#define OLED_NODE  DT_ALIAS(oled0)
#define BME280_NODE DT_ALIAS(temp0)
#define FLASH_NODE DT_ALIAS(flash0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *display = DEVICE_DT_GET(OLED_NODE);
static const struct device *bme280  = DEVICE_DT_GET(BME280_NODE);
static const struct device *flash   = DEVICE_DT_GET(FLASH_NODE);

void test_flash(void)
{
	const uint32_t offset = 0x000000;
	const uint8_t write_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	uint8_t read_data[4];
	int ret;

	LOG_INF("--- Flash Test ---");

	ret = flash_erase(flash, offset, 4096); // Sector erase
	if (ret != 0) {
		LOG_ERR("Flash erase failed: %d", ret);
		return;
	}

	ret = flash_write(flash, offset, write_data, sizeof(write_data));
	if (ret != 0) {
		LOG_ERR("Flash write failed: %d", ret);
		return;
	}

	ret = flash_read(flash, offset, read_data, sizeof(read_data));
	if (ret != 0) {
		LOG_ERR("Flash read failed: %d", ret);
		return;
	}

	if (memcmp(write_data, read_data, sizeof(write_data)) == 0) {
		LOG_INF("Flash Test PASSED: %02X %02X %02X %02X", 
			read_data[0], read_data[1], read_data[2], read_data[3]);
	} else {
		LOG_ERR("Flash Test FAILED: %02X %02X %02X %02X", 
			read_data[0], read_data[1], read_data[2], read_data[3]);
	}
}

void test_sensors(void)
{
	struct sensor_value temp, press, hum;
	int ret;

	LOG_INF("--- Sensor Test ---");

	ret = sensor_sample_fetch(bme280);
	if (ret != 0) {
		LOG_ERR("BME280 fetch failed: %d", ret);
		return;
	}

	sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &press);
	sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &hum);

	LOG_INF("BME280: Temp: %d.%06d C, Press: %d.%06d kPa, Hum: %d.%06d %%",
		temp.val1, temp.val2, press.val1, press.val2, hum.val1, hum.val2);
}

int main(void)
{
	LOG_INF("--- App Start (Full Port) ---");

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device not ready");
	} else {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		LOG_INF("LED ready on PB2");
	}

	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready!");
	} else {
		LOG_INF("Display ready");
	}

	if (!device_is_ready(bme280)) {
		LOG_ERR("BME280 device not ready!");
	} else {
		LOG_INF("BME280 ready");
	}

	if (!device_is_ready(flash)) {
		LOG_ERR("Flash device not ready!");
	} else {
		LOG_INF("Flash ready (W25Q16)");
	}

	test_flash();

	while (1) {
		test_sensors();
		gpio_pin_toggle_dt(&led);
		k_msleep(2000);
	}

	return 0;
}

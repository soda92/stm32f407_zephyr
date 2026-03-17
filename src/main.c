#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main);

/* Aliases from devicetree */
#define LED0_NODE DT_ALIAS(led0)
#define OLED_NODE DT_ALIAS(oled0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *display = DEVICE_DT_GET(OLED_NODE);

void oled_flash_test(const struct device *dev)
{
	struct display_buffer_descriptor desc;
	uint8_t buffer[1024]; // 128x64 pixels / 8 bits per pixel

	desc.width = 128;
	desc.height = 64;
	desc.pitch = 128;

	while (1) {
		// --- WHITE ---
		memset(buffer, 0xFF, sizeof(buffer));
		LOG_INF("OLED White...");
		display_write(dev, 0, 0, &desc, buffer);
		k_msleep(1000);

		// --- BLACK ---
		memset(buffer, 0x00, sizeof(buffer));
		LOG_INF("OLED Black...");
		display_write(dev, 0, 0, &desc, buffer);
		k_msleep(1000);

		// Toggle LED too
		gpio_pin_toggle_dt(&led);
		LOG_INF("Heartbeat LED toggled");
	}
}

int main(void)
{
	int ret;

	LOG_INF("--- App Start ---");

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device not ready");
	} else {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED pin: %d", ret);
		} else {
			LOG_INF("LED ready on PB2");
		}
	}

	LOG_INF("Checking Display device...");
	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready!");
	} else {
		LOG_INF("Display ready, starting test loop...");
		oled_flash_test(display);
	}

	while (1) {
		LOG_INF("Idle loop...");
		gpio_pin_toggle_dt(&led);
		k_msleep(2000);
	}

	return 0;
}

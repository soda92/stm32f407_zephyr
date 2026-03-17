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
		LOG_INF("OLED White");
		display_write(dev, 0, 0, &desc, buffer);
		k_msleep(1000);

		// --- BLACK ---
		memset(buffer, 0x00, sizeof(buffer));
		LOG_INF("OLED Black");
		display_write(dev, 0, 0, &desc, buffer);
		k_msleep(1000);

		// Toggle LED too
		gpio_pin_toggle_dt(&led);
	}
}

int main(void)
{
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device not ready");
		return 0;
	}

	if (!device_is_ready(display)) {
		LOG_ERR("Display device not ready");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	
	LOG_INF("Starting OLED Flash Test...");
	oled_flash_test(display);

	return 0;
}

#ifndef NATIVE_SIM_MOCKS_H
#define NATIVE_SIM_MOCKS_H

#ifdef CONFIG_BOARD_NATIVE_SIM
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

/* Mock State */
extern float mock_temp_val;
extern uint8_t mock_flash_storage[4096];

/* Mock device pointers */
static const struct device *bme280  = (void*)0x1;
static const struct device *display = (void*)0x2;
#ifdef CONFIG_ZTEST
extern const struct device *flash;
#else
static const struct device *flash   = (void*)0x3;
#endif
static const struct gpio_dt_spec led = {0};
static const struct gpio_dt_spec btn_exit = {0};
static const struct gpio_dt_spec btn_save = {0};
static const struct gpio_dt_spec btn_led  = {0};
static const struct gpio_dt_spec btn_hist = {0};

/* Mock implementations */
static inline int mock_flash_read(const struct device *dev, off_t offset, void *data, size_t len) { 
    if (offset + len > 4096) return -1;
    memcpy(data, &mock_flash_storage[offset], len);
    return 0; 
}
#define flash_read mock_flash_read

static inline int mock_flash_write(const struct device *dev, off_t offset, const void *data, size_t len) { 
    if (offset + len > 4096) return -1;
    for (size_t i = 0; i < len; i++) {
        mock_flash_storage[offset + i] &= ((uint8_t*)data)[i];
    }
    return 0; 
}
#define flash_write mock_flash_write

static inline int mock_flash_erase(const struct device *dev, off_t offset, size_t size) { 
    if (offset + size > 4096) return -1;
    memset(&mock_flash_storage[offset], 0xFF, size);
    return 0; 
}
#define flash_erase mock_flash_erase

static inline int mock_sensor_sample_fetch(const struct device *dev) { return 0; }
#define sensor_sample_fetch mock_sensor_sample_fetch

static inline int mock_sensor_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    val->val1 = (int32_t)mock_temp_val;
    val->val2 = (int32_t)((mock_temp_val - val->val1) * 1000000);
    return 0;
}
#define sensor_channel_get mock_sensor_channel_get

#define device_is_ready(dev) (true)
#define cfb_framebuffer_init(dev) (0)
#define cfb_framebuffer_clear(dev, clear) (0)

static inline int mock_cfb_print(const struct device *dev, const char *str, uint16_t x, uint16_t y) { return 0; }
#define cfb_print mock_cfb_print

#define cfb_framebuffer_finalize(dev) (0)
#define display_blanking_off(dev) (0)
#define cfb_get_numof_fonts(dev) (1)

static inline int mock_cfb_get_font_size(const struct device *dev, uint8_t idx, uint8_t *w, uint8_t *h) {
    *w = 8; *h = 16; return 0;
}
#define cfb_get_font_size mock_cfb_get_font_size

#define cfb_framebuffer_set_font(dev, idx) (0)
#define gpio_pin_configure_dt(spec, flags) (0)
#define gpio_pin_set_dt(spec, val) (0)
#define gpio_pin_get_dt(spec) (0)
#define gpio_pin_toggle_dt(spec) (0)
#define gpio_pin_interrupt_configure_dt(spec, flags) (0)
#define gpio_init_callback(cb, hand, pins) 
#define gpio_add_callback(port, cb) (0)

/* Shell command logic */
extern bool use_fahrenheit;
extern bool blink_led;
extern bool view_history;
extern uint32_t saved_count;
extern uint32_t history_index;
extern float last_live_temp;
void flash_save_record(float temp);
uint32_t get_next_history_index(uint32_t current, uint32_t count);

static int cmd_mock_save(const struct shell *sh, size_t argc, char **argv) {
    if (!view_history) flash_save_record(last_live_temp);
    else history_index = get_next_history_index(history_index, saved_count);
    return 0;
}
static int cmd_mock_exit(const struct shell *sh, size_t argc, char **argv) {
    use_fahrenheit = !use_fahrenheit; return 0;
}
static int cmd_mock_temp(const struct shell *sh, size_t argc, char **argv) {
    if (argc < 2) return -EINVAL;
    mock_temp_val = strtof(argv[1], NULL); return 0;
}

SHELL_CMD_REGISTER(mock_save, NULL, "Save current temp", cmd_mock_save);
SHELL_CMD_REGISTER(mock_exit, NULL, "Toggle C/F", cmd_mock_exit);
SHELL_CMD_REGISTER(mock_temp, NULL, "Set temp", cmd_mock_temp);

#endif /* CONFIG_BOARD_NATIVE_SIM */
#endif /* NATIVE_SIM_MOCKS_H */

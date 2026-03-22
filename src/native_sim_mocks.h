#ifndef NATIVE_SIM_MOCKS_H
#define NATIVE_SIM_MOCKS_H

#ifdef CONFIG_BOARD_NATIVE_SIM
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <string.h>

/* Mock device pointers */
static const struct device *bme280  = (void*)0x1;
static const struct device *display = (void*)0x2;
static const struct device *flash   = (void*)0x3;
static const struct gpio_dt_spec led = {0};
static const struct gpio_dt_spec btn_exit = {0};
static const struct gpio_dt_spec btn_save = {0};
static const struct gpio_dt_spec btn_led  = {0};
static const struct gpio_dt_spec btn_hist = {0};

/* Mock implementations with different names to avoid collision with static inlines */
static inline int mock_flash_read(const struct device *dev, off_t offset, void *data, size_t len) { 
    memset(data, 0xFF, len); return 0; 
}
#define flash_read mock_flash_read

static inline int mock_flash_write(const struct device *dev, off_t offset, const void *data, size_t len) { return 0; }
#define flash_write mock_flash_write

static inline int mock_flash_erase(const struct device *dev, off_t offset, size_t size) { return 0; }
#define flash_erase mock_flash_erase

static inline int mock_sensor_sample_fetch(const struct device *dev) { return 0; }
#define sensor_sample_fetch mock_sensor_sample_fetch

static inline int mock_sensor_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    val->val1 = 25; val->val2 = 500000; return 0;
}
#define sensor_channel_get mock_sensor_channel_get

#define device_is_ready(dev) (true)
#define cfb_framebuffer_init(dev) (0)
#define cfb_framebuffer_clear(dev, clear) (0)

static inline int mock_cfb_print(const struct device *dev, const char *str, uint16_t x, uint16_t y) { 
    printf("[OLED] %s\n", str); return 0; 
}
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

/* Shell command logic needs access to these globals */
extern bool use_fahrenheit;
extern bool blink_led;
extern bool view_history;
extern uint32_t saved_count;
extern uint32_t history_index;
extern float last_live_temp;
void flash_save_record(float temp);
uint32_t get_next_history_index(uint32_t current, uint32_t count);

static int cmd_press(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) return -1;
    if (strcmp(argv[1], "save") == 0) {
        shell_print(sh, "Mock: Press Save");
        if (!view_history) flash_save_record(last_live_temp);
        else history_index = get_next_history_index(history_index, saved_count);
    } else if (strcmp(argv[1], "exit") == 0) {
        shell_print(sh, "Mock: Press Exit");
        use_fahrenheit = !use_fahrenheit;
    } else if (strcmp(argv[1], "led") == 0) {
        shell_print(sh, "Mock: Toggle LED");
        blink_led = !blink_led;
    } else if (strcmp(argv[1], "hist") == 0) {
        shell_print(sh, "Mock: Toggle History");
        view_history = !view_history;
        if (view_history && saved_count > 0) history_index = 0;
    }
    return 0;
}
SHELL_CMD_REGISTER(press, NULL, "Simulate button press", cmd_press);

#endif /* CONFIG_BOARD_NATIVE_SIM */
#endif /* NATIVE_SIM_MOCKS_H */

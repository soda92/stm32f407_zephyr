#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

#include <stddef.h>

#define MAX_RECORDS 1024

/* Pure Logic: Temperature Conversion */
float celsius_to_fahrenheit(float celsius);

/* Logic: Calculate next history index */
uint32_t get_next_history_index(uint32_t current_index, uint32_t saved_count);

/* Logic: Format display strings */
void format_temp_display(char *buf, size_t len, float temp, bool fahrenheit);
void format_history_header(char *buf, size_t len, uint32_t index, uint32_t total);

/* Logic: Flash record validation */
bool is_valid_record(float val);

#endif

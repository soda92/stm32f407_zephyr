#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_RECORDS 1024

/* Pure Logic: Temperature Conversion */
float celsius_to_fahrenheit(float celsius);

/* Logic: Calculate next history index */
uint32_t get_next_history_index(uint32_t current_index, uint32_t saved_count);

#endif

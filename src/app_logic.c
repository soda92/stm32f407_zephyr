#include "app_logic.h"

float celsius_to_fahrenheit(float celsius)
{
	return (celsius * 9.0f / 5.0f) + 32.0f;
}

uint32_t get_next_history_index(uint32_t current_index, uint32_t saved_count)
{
	if (saved_count == 0) return 0;
	return (current_index + 1) % saved_count;
}

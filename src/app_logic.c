#include "app_logic.h"
#include <stdio.h>
#include <string.h>

float celsius_to_fahrenheit(float celsius)
{
	return (celsius * 9.0f / 5.0f) + 32.0f;
}

uint32_t get_next_history_index(uint32_t current_index, uint32_t saved_count)
{
	if (saved_count == 0) return 0;
	return (current_index + 1) % saved_count;
}

void format_temp_display(char *buf, size_t len, float temp, bool fahrenheit)
{
	float t = fahrenheit ? celsius_to_fahrenheit(temp) : temp;
	snprintf(buf, len, "Temp: %.2f %c", (double)t, fahrenheit ? 'F' : 'C');
}

void format_history_header(char *buf, size_t len, uint32_t index, uint32_t total)
{
	snprintf(buf, len, "- HIST [%d/%d] -", index + 1, total);
}

bool is_valid_record(float val)
{
	uint32_t raw;
	memcpy(&raw, &val, 4);
	return (raw != 0xFFFFFFFF);
}

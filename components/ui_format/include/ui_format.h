/**
 * @file ui_format.h
 * @brief Small time/duration string formatters for labels.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

void ui_format_mm_ss(char *buf, size_t len, uint32_t sec);
void ui_format_hh_mm(char *buf, size_t len, int hour, int min);

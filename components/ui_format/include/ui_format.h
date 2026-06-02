/**
 * @file ui_format.h
 * @brief Small time/duration string formatters for labels.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

void ui_format_mm_ss(char *buf, size_t len, uint32_t sec);
/** e.g. "1 minute", "5 minutes" (whole minutes from @p sec). */
void ui_format_duration_minutes(char *buf, size_t len, uint32_t sec);
/** e.g. "5 seconds", "1 minute 30 seconds" (shows seconds when &lt; 1 min). */
void ui_format_duration_human(char *buf, size_t len, uint32_t sec);
/** 12-hour local time at now + @p offset_sec, e.g. "3:45 PM". */
void ui_format_hh_mm_ampm_after_sec(char *buf, size_t len, uint32_t offset_sec);
void ui_format_hh_mm(char *buf, size_t len, int hour, int min);
/** Format local wall-clock time from the system RTC (after SNTP). */
void ui_format_hh_mm_now(char *buf, size_t len);
/** 12-hour local time with AM/PM, e.g. "3:45 PM". */
void ui_format_hh_mm_ampm_now(char *buf, size_t len);
/** 12-hour parts: @p time_buf e.g. "3:45", @p ampm_buf "AM" or "PM". */
void ui_format_hh_mm_ampm_parts_now(char *time_buf, size_t time_len, char *ampm_buf, size_t ampm_len);

/**
 * @file ui_format.h
 * @brief Small time/duration string formatters for labels.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

void ui_format_mm_ss(char *buf, size_t len, uint32_t sec);
/**
 * Countdown as XX:YY — hours:minutes when @p sec >= 3600, otherwise minutes:seconds.
 */
void ui_format_countdown_xx_yy(char *buf, size_t len, uint32_t sec);
/** e.g. "1 minute", "5 minutes" (whole minutes from @p sec). */
void ui_format_duration_minutes(char *buf, size_t len, uint32_t sec);
/** e.g. "22 hours and 40 minutes", "1 hour and 1 minute" (whole minutes from @p sec). */
void ui_format_hours_and_minutes(char *buf, size_t len, uint32_t sec);
/** e.g. "5 seconds", "1 minute 30 seconds" (shows seconds when &lt; 1 min). */
void ui_format_duration_human(char *buf, size_t len, uint32_t sec);
/** 12-hour local time at now + @p offset_sec, e.g. "3:45 PM". */
void ui_format_hh_mm_ampm_after_sec(char *buf, size_t len, uint32_t offset_sec);
/** 12-hour local time at now + @p offset_sec without AM/PM, e.g. "3:45". */
void ui_format_hh_mm_after_sec(char *buf, size_t len, uint32_t offset_sec);
void ui_format_hh_mm(char *buf, size_t len, int hour, int min);
/** Format local wall-clock time from the system RTC (after SNTP). */
void ui_format_hh_mm_now(char *buf, size_t len);
/** 12-hour local time with AM/PM, e.g. "3:45 PM". */
void ui_format_hh_mm_ampm_now(char *buf, size_t len);
/** 12-hour parts: @p time_buf e.g. "3:45", @p ampm_buf "AM" or "PM". */
void ui_format_hh_mm_ampm_parts_now(char *time_buf, size_t time_len, char *ampm_buf, size_t ampm_len);

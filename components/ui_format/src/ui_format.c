/**
 * @file ui_format.c
 */

#include "ui_format.h"
#include <stdio.h>
#include <time.h>

static void format_12h_parts(const struct tm *tm_info, int *h12_out, int *min_out, const char **ampm_out)
{
    int h24 = tm_info->tm_hour;
    int h12 = h24 % 12;
    if (h12 == 0) {
        h12 = 12;
    }
    *h12_out = h12;
    *min_out = tm_info->tm_min;
    *ampm_out = (h24 >= 12) ? "PM" : "AM";
}

void ui_format_mm_ss(char *buf, size_t len, uint32_t sec)
{
    snprintf(buf, len, "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

void ui_format_duration_minutes(char *buf, size_t len, uint32_t sec)
{
    unsigned mins = (unsigned)(sec / 60);
    if (mins == 1) {
        snprintf(buf, len, "1 minute");
    } else {
        snprintf(buf, len, "%u minutes", mins);
    }
}

void ui_format_hh_mm_ampm_after_sec(char *buf, size_t len, uint32_t offset_sec)
{
    time_t end = time(NULL) + (time_t)offset_sec;
    struct tm tm_info;
    localtime_r(&end, &tm_info);
    int h12;
    int min;
    const char *ampm;
    format_12h_parts(&tm_info, &h12, &min, &ampm);
    snprintf(buf, len, "%d:%02d %s", h12, min, ampm);
}

void ui_format_hh_mm(char *buf, size_t len, int hour, int min)
{
    snprintf(buf, len, "%02d:%02d", hour, min);
}

void ui_format_hh_mm_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    ui_format_hh_mm(buf, len, tm_info.tm_hour, tm_info.tm_min);
}

void ui_format_hh_mm_ampm_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    int h12;
    int min;
    const char *ampm;
    format_12h_parts(&tm_info, &h12, &min, &ampm);
    snprintf(buf, len, "%d:%02d %s", h12, min, ampm);
}

void ui_format_hh_mm_ampm_parts_now(char *time_buf, size_t time_len, char *ampm_buf, size_t ampm_len)
{
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    int h12;
    int min;
    const char *ampm;
    format_12h_parts(&tm_info, &h12, &min, &ampm);
    snprintf(time_buf, time_len, "%d:%02d", h12, min);
    snprintf(ampm_buf, ampm_len, "%s", ampm);
}

/**
 * @file ui_format.c
 */

#include "ui_format.h"
#include <stdio.h>

void ui_format_mm_ss(char *buf, size_t len, uint32_t sec)
{
    snprintf(buf, len, "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

void ui_format_hh_mm(char *buf, size_t len, int hour, int min)
{
    snprintf(buf, len, "%02d:%02d", hour, min);
}

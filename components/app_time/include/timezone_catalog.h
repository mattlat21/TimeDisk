/**
 * @file timezone_catalog.h
 * @brief Embedded country/location → IANA id + POSIX TZ (docs/timezone_catalog.md).
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

size_t timezone_catalog_country_count(void);

const char *timezone_catalog_country_label(size_t country_idx);

size_t timezone_catalog_location_count(size_t country_idx);

const char *timezone_catalog_location_label(size_t country_idx, size_t loc_idx);

const char *timezone_catalog_timezone_id(size_t country_idx, size_t loc_idx);

const char *timezone_catalog_posix_tz(size_t country_idx, size_t loc_idx);

const char *timezone_catalog_posix_tz_by_id(const char *timezone_id);

size_t timezone_catalog_format_country_options(char *buf, size_t buf_len);

size_t timezone_catalog_format_location_options(size_t country_idx, char *buf, size_t buf_len);

bool timezone_catalog_find_by_id(const char *timezone_id, size_t *country_idx, size_t *loc_idx);

/** Default startup selection: Australia / Melbourne. */
#define TIMEZONE_CATALOG_DEFAULT_ID "Australia/Melbourne"

bool timezone_catalog_default_selection(size_t *country_idx, size_t *loc_idx);

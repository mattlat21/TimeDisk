/**
 * @file timezone_catalog.c
 * @brief Curated country/location list (docs/timezone_catalog.md).
 */

#include "timezone_catalog.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *label;
    const char *timezone_id;
    const char *posix_tz;
} tz_loc_t;

typedef struct {
    const char *country;
    const tz_loc_t *locs;
    size_t loc_count;
} tz_country_t;

static const tz_loc_t s_uk[] = {
    { "London", "Europe/London", "GMT0BST,M3.5.0/1,M10.5.0" },
};

static const tz_loc_t s_ie[] = {
    { "Dublin", "Europe/Dublin", "GMT0IST,M3.5.0/1,M10.5.0" },
};

static const tz_loc_t s_us[] = {
    { "Eastern", "America/New_York", "EST5EDT,M3.2.0,M11.1.0" },
    { "Central", "America/Chicago", "CST6CDT,M3.2.0,M11.1.0" },
    { "Mountain", "America/Denver", "MST7MDT,M3.2.0,M11.1.0" },
    { "Pacific", "America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0" },
    { "Alaska", "America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0" },
    { "Hawaii", "Pacific/Honolulu", "HST10" },
};

static const tz_loc_t s_ca[] = {
    { "Eastern", "America/Toronto", "EST5EDT,M3.2.0,M11.1.0" },
    { "Central", "America/Winnipeg", "CST6CDT,M3.2.0,M11.1.0" },
    { "Mountain", "America/Edmonton", "MST7MDT,M3.2.0,M11.1.0" },
    { "Pacific", "America/Vancouver", "PST8PDT,M3.2.0,M11.1.0" },
};

static const tz_loc_t s_au[] = {
    { "Sydney", "Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3" },
    { "Melbourne", "Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3" },
    { "Brisbane", "Australia/Brisbane", "AEST-10" },
    { "Perth", "Australia/Perth", "AWST-8" },
};

static const tz_loc_t s_nz[] = {
    { "Auckland", "Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3" },
};

static const tz_loc_t s_de[] = {
    { "Berlin", "Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const tz_loc_t s_fr[] = {
    { "Paris", "Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const tz_loc_t s_nl[] = {
    { "Amsterdam", "Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const tz_loc_t s_es[] = {
    { "Madrid", "Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const tz_loc_t s_it[] = {
    { "Rome", "Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const tz_loc_t s_in[] = {
    { "Mumbai", "Asia/Kolkata", "IST-5:30" },
};

static const tz_loc_t s_jp[] = {
    { "Tokyo", "Asia/Tokyo", "JST-9" },
};

static const tz_loc_t s_sg[] = {
    { "Singapore", "Asia/Singapore", "SGT-8" },
};

static const tz_loc_t s_ae[] = {
    { "Dubai", "Asia/Dubai", "GST-4" },
};

static const tz_loc_t s_za[] = {
    { "Johannesburg", "Africa/Johannesburg", "SAST-2" },
};

static const tz_loc_t s_br[] = {
    { "Sao Paulo", "America/Sao_Paulo", "<-03>3" },
};

static const tz_loc_t s_mx[] = {
    { "Mexico City", "America/Mexico_City", "CST6" },
};

static const tz_country_t s_countries[] = {
    { "United Kingdom", s_uk, sizeof(s_uk) / sizeof(s_uk[0]) },
    { "Ireland", s_ie, sizeof(s_ie) / sizeof(s_ie[0]) },
    { "United States", s_us, sizeof(s_us) / sizeof(s_us[0]) },
    { "Canada", s_ca, sizeof(s_ca) / sizeof(s_ca[0]) },
    { "Australia", s_au, sizeof(s_au) / sizeof(s_au[0]) },
    { "New Zealand", s_nz, sizeof(s_nz) / sizeof(s_nz[0]) },
    { "Germany", s_de, sizeof(s_de) / sizeof(s_de[0]) },
    { "France", s_fr, sizeof(s_fr) / sizeof(s_fr[0]) },
    { "Netherlands", s_nl, sizeof(s_nl) / sizeof(s_nl[0]) },
    { "Spain", s_es, sizeof(s_es) / sizeof(s_es[0]) },
    { "Italy", s_it, sizeof(s_it) / sizeof(s_it[0]) },
    { "India", s_in, sizeof(s_in) / sizeof(s_in[0]) },
    { "Japan", s_jp, sizeof(s_jp) / sizeof(s_jp[0]) },
    { "Singapore", s_sg, sizeof(s_sg) / sizeof(s_sg[0]) },
    { "United Arab Emirates", s_ae, sizeof(s_ae) / sizeof(s_ae[0]) },
    { "South Africa", s_za, sizeof(s_za) / sizeof(s_za[0]) },
    { "Brazil", s_br, sizeof(s_br) / sizeof(s_br[0]) },
    { "Mexico", s_mx, sizeof(s_mx) / sizeof(s_mx[0]) },
};

size_t timezone_catalog_country_count(void)
{
    return sizeof(s_countries) / sizeof(s_countries[0]);
}

const char *timezone_catalog_country_label(size_t country_idx)
{
    if (country_idx >= timezone_catalog_country_count()) {
        return "";
    }
    return s_countries[country_idx].country;
}

size_t timezone_catalog_location_count(size_t country_idx)
{
    if (country_idx >= timezone_catalog_country_count()) {
        return 0;
    }
    return s_countries[country_idx].loc_count;
}

const char *timezone_catalog_location_label(size_t country_idx, size_t loc_idx)
{
    if (country_idx >= timezone_catalog_country_count()) {
        return "";
    }
    const tz_country_t *c = &s_countries[country_idx];
    if (loc_idx >= c->loc_count) {
        return "";
    }
    return c->locs[loc_idx].label;
}

const char *timezone_catalog_timezone_id(size_t country_idx, size_t loc_idx)
{
    if (country_idx >= timezone_catalog_country_count()) {
        return "";
    }
    const tz_country_t *c = &s_countries[country_idx];
    if (loc_idx >= c->loc_count) {
        return "";
    }
    return c->locs[loc_idx].timezone_id;
}

const char *timezone_catalog_posix_tz(size_t country_idx, size_t loc_idx)
{
    if (country_idx >= timezone_catalog_country_count()) {
        return NULL;
    }
    const tz_country_t *c = &s_countries[country_idx];
    if (loc_idx >= c->loc_count) {
        return NULL;
    }
    return c->locs[loc_idx].posix_tz;
}

const char *timezone_catalog_posix_tz_by_id(const char *timezone_id)
{
    if (timezone_id == NULL) {
        return NULL;
    }
    for (size_t ci = 0; ci < timezone_catalog_country_count(); ci++) {
        for (size_t li = 0; li < timezone_catalog_location_count(ci); li++) {
            if (strcmp(timezone_catalog_timezone_id(ci, li), timezone_id) == 0) {
                return timezone_catalog_posix_tz(ci, li);
            }
        }
    }
    return NULL;
}

static size_t append_option(char *buf, size_t buf_len, size_t used, const char *label)
{
    size_t need = strlen(label) + (used > 0 ? 1 : 0);
    if (used + need + 1 > buf_len) {
        return 0;
    }
    if (used > 0) {
        buf[used++] = '\n';
    }
    memcpy(buf + used, label, strlen(label));
    used += strlen(label);
    buf[used] = '\0';
    return used;
}

size_t timezone_catalog_format_country_options(char *buf, size_t buf_len)
{
    if (buf_len == 0) {
        return 0;
    }
    buf[0] = '\0';
    size_t used = 0;
    for (size_t i = 0; i < timezone_catalog_country_count(); i++) {
        used = append_option(buf, buf_len, used, timezone_catalog_country_label(i));
        if (used == 0) {
            return 0;
        }
    }
    return used;
}

size_t timezone_catalog_format_location_options(size_t country_idx, char *buf, size_t buf_len)
{
    if (buf_len == 0) {
        return 0;
    }
    buf[0] = '\0';
    size_t used = 0;
    size_t n = timezone_catalog_location_count(country_idx);
    for (size_t i = 0; i < n; i++) {
        used = append_option(buf, buf_len, used, timezone_catalog_location_label(country_idx, i));
        if (used == 0) {
            return 0;
        }
    }
    return used;
}

bool timezone_catalog_find_by_id(const char *timezone_id, size_t *country_idx, size_t *loc_idx)
{
    if (timezone_id == NULL) {
        return false;
    }
    for (size_t ci = 0; ci < timezone_catalog_country_count(); ci++) {
        for (size_t li = 0; li < timezone_catalog_location_count(ci); li++) {
            if (strcmp(timezone_catalog_timezone_id(ci, li), timezone_id) == 0) {
                if (country_idx != NULL) {
                    *country_idx = ci;
                }
                if (loc_idx != NULL) {
                    *loc_idx = li;
                }
                return true;
            }
        }
    }
    return false;
}

bool timezone_catalog_default_selection(size_t *country_idx, size_t *loc_idx)
{
    return timezone_catalog_find_by_id(TIMEZONE_CATALOG_DEFAULT_ID, country_idx, loc_idx);
}

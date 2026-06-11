/**
 * @file app_version.h
 * @brief Firmware release version (maj.min.rev).
 *
 * Version numbers are defined in cmake/firmware_version.cmake and passed into
 * this component as compile definitions. app_version_string() returns "maj.min.rev".
 */

#pragma once

#include <stdint.h>

#ifndef APP_VERSION_MAJ
#define APP_VERSION_MAJ 0
#endif
#ifndef APP_VERSION_MIN
#define APP_VERSION_MIN 0
#endif
#ifndef APP_VERSION_REV
#define APP_VERSION_REV 0
#endif

uint8_t app_version_maj(void);
uint8_t app_version_min(void);
uint8_t app_version_rev(void);

/** Combined version string, e.g. "0.1.0". */
const char *app_version_string(void);

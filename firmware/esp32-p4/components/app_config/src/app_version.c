/**
 * @file app_version.c
 */

#include "app_version.h"

#define APP_VERSION_STRINGIFY(x) #x
#define APP_VERSION_TOSTRING(x)  APP_VERSION_STRINGIFY(x)
#define APP_VERSION_COMBINED     APP_VERSION_TOSTRING(APP_VERSION_MAJ) "." \
                                 APP_VERSION_TOSTRING(APP_VERSION_MIN) "." \
                                 APP_VERSION_TOSTRING(APP_VERSION_REV)

uint8_t app_version_maj(void)
{
    return (uint8_t)APP_VERSION_MAJ;
}

uint8_t app_version_min(void)
{
    return (uint8_t)APP_VERSION_MIN;
}

uint8_t app_version_rev(void)
{
    return (uint8_t)APP_VERSION_REV;
}

const char *app_version_string(void)
{
    return APP_VERSION_COMBINED;
}

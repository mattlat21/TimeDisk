/**
 * @file ui_spiffs_pixelart.c
 * @brief Integer block upscale for small SPIFFS pixel-art (avoids LVGL transform filtering).
 */

#include "ui_spiffs_pixelart.h"

#include "draw/lv_image_decoder_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char path[48];
    uint16_t w;
    uint16_t h;
    uint16_t *pixels;
} pixelart_cache_t;

static pixelart_cache_t s_cache;

static bool header_is_block_scalable(const lv_image_header_t *header, int32_t dest_w, int32_t dest_h)
{
    if (header->w == 0 || header->h == 0) {
        return false;
    }
    if (header->w > UI_SPIFFS_PIXELART_CACHE_MAX || header->h > UI_SPIFFS_PIXELART_CACHE_MAX) {
        return false;
    }
    if (header->cf != LV_COLOR_FORMAT_RGB565) {
        return false;
    }
    return (dest_w % (int32_t)header->w) == 0 && (dest_h % (int32_t)header->h) == 0;
}

static void cache_free(void)
{
    free(s_cache.pixels);
    memset(&s_cache, 0, sizeof(s_cache));
}

bool ui_spiffs_pixelart_is_block_scalable(const char *path, int32_t dest_w, int32_t dest_h)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    lv_image_header_t header;
    if (lv_image_decoder_get_info(path, &header) != LV_RESULT_OK) {
        return false;
    }

    return header_is_block_scalable(&header, dest_w, dest_h);
}

void ui_spiffs_pixelart_cache_drop(void)
{
    cache_free();
}

void ui_spiffs_pixelart_cache_preload(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        cache_free();
        return;
    }

    if (strncmp(s_cache.path, path, sizeof(s_cache.path)) == 0 && s_cache.pixels != NULL) {
        return;
    }

    cache_free();

    lv_image_header_t header;
    if (lv_image_decoder_get_info(path, &header) != LV_RESULT_OK) {
        return;
    }
    if (header.w > UI_SPIFFS_PIXELART_CACHE_MAX || header.h > UI_SPIFFS_PIXELART_CACHE_MAX) {
        return;
    }
    if (header.cf != LV_COLOR_FORMAT_RGB565) {
        return;
    }

    lv_image_decoder_dsc_t dsc;
    lv_image_decoder_args_t args;
    lv_memzero(&args, sizeof(args));

    if (lv_image_decoder_open(&dsc, path, &args) != LV_RESULT_OK) {
        return;
    }
    if (dsc.decoded == NULL || dsc.decoded->data == NULL) {
        lv_image_decoder_close(&dsc);
        return;
    }

    const size_t px_count = (size_t)header.w * (size_t)header.h;
    s_cache.pixels = malloc(px_count * sizeof(uint16_t));
    if (s_cache.pixels == NULL) {
        lv_image_decoder_close(&dsc);
        return;
    }

    memcpy(s_cache.pixels, dsc.decoded->data, px_count * sizeof(uint16_t));
    s_cache.w = header.w;
    s_cache.h = header.h;
    snprintf(s_cache.path, sizeof(s_cache.path), "%s", path);

    lv_image_decoder_close(&dsc);
}

static lv_color_t rgb565_to_lv_color(uint16_t px)
{
    const uint8_t r = (uint8_t)(((px >> 11) & 0x1FU) * 255U / 31U);
    const uint8_t g = (uint8_t)(((px >> 5) & 0x3FU) * 255U / 63U);
    const uint8_t b = (uint8_t)((px & 0x1FU) * 255U / 31U);
    return lv_color_make(r, g, b);
}

static void draw_block_scaled(lv_layer_t *layer, const lv_area_t *dest, const pixelart_cache_t *cache)
{
    const int32_t dest_w = lv_area_get_width(dest);
    const int32_t dest_h = lv_area_get_height(dest);
    const int32_t block_w = dest_w / (int32_t)cache->w;
    const int32_t block_h = dest_h / (int32_t)cache->h;

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    dsc.radius = 0;

    for (uint16_t sy = 0; sy < cache->h; sy++) {
        for (uint16_t sx = 0; sx < cache->w; sx++) {
            const uint16_t px = cache->pixels[(size_t)sy * cache->w + sx];
            dsc.bg_color = rgb565_to_lv_color(px);

            lv_area_t block = {
                .x1 = dest->x1 + (int32_t)sx * block_w,
                .y1 = dest->y1 + (int32_t)sy * block_h,
                .x2 = dest->x1 + (int32_t)(sx + 1) * block_w - 1,
                .y2 = dest->y1 + (int32_t)(sy + 1) * block_h - 1,
            };
            lv_draw_rect(layer, &dsc, &block);
        }
    }
}

static void draw_file_1x1(lv_layer_t *layer, const lv_area_t *dest, const char *path)
{
    lv_draw_image_dsc_t draw_dsc;
    lv_draw_image_dsc_init(&draw_dsc);
    draw_dsc.src = path;
    draw_dsc.scale_x = LV_SCALE_NONE;
    draw_dsc.scale_y = LV_SCALE_NONE;
    draw_dsc.antialias = false;
    draw_dsc.image_area = *dest;

    lv_draw_image(layer, &draw_dsc, dest);
}

void ui_spiffs_pixelart_draw(lv_layer_t *layer, const lv_area_t *dest, const char *path)
{
    if (layer == NULL || dest == NULL || path == NULL || path[0] == '\0') {
        return;
    }

    const int32_t dest_w = lv_area_get_width(dest);
    const int32_t dest_h = lv_area_get_height(dest);
    if (dest_w <= 0 || dest_h <= 0) {
        return;
    }

    if (ui_spiffs_pixelart_is_block_scalable(path, dest_w, dest_h)) {
        if (strncmp(s_cache.path, path, sizeof(s_cache.path)) != 0 || s_cache.pixels == NULL) {
            ui_spiffs_pixelart_cache_preload(path);
        }
        if (s_cache.pixels != NULL) {
            draw_block_scaled(layer, dest, &s_cache);
            return;
        }
    }

    draw_file_1x1(layer, dest, path);
}

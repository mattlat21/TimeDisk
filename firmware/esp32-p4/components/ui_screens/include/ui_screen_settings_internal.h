#pragma once

#include "lvgl.h"
#include "app_config.h"

typedef enum {
    PANEL_HUB = 0,
    PANEL_COLOURS,
    PANEL_NETWORK,
    PANEL_TIMEZONE,
    PANEL_SCHEDULE,
    PANEL_TIMEOUTS,
    PANEL_AA,
    PANEL_MQTT,
    PANEL_UPDATE,
    PANEL_COUNT,
} settings_panel_t;

/* Shared Settings screen context (owned by ui_screen_settings.c). */
lv_obj_t *ui_settings_screen(void);
app_config_t *ui_settings_draft(void);
app_config_t *ui_settings_saved(void);

/* Common helpers implemented by ui_screen_settings.c. */
int ui_settings_wf_y(lv_obj_t *parent, int y_wf);
int ui_settings_wf_x(lv_obj_t *parent, int x_wf);
void ui_settings_panel_layout(lv_obj_t *panel);
lv_obj_t *ui_settings_create_sub_panel(const char *title);
void ui_settings_attach_panel_wedges(lv_obj_t *panel, settings_panel_t panel_id, lv_event_cb_t save_cb);
void ui_settings_set_panel_wedges_visible(settings_panel_t panel, bool visible);
void ui_settings_show_panel(settings_panel_t panel);
void ui_settings_idle_cb(void *user_data);
void ui_settings_refresh_web_ui_label(void);
void ui_settings_register_overlay_obj(lv_obj_t *obj);

/* Panel-specific helpers needed by shared cancel plumbing. */
bool ui_settings_timeouts_on_cancel(void);


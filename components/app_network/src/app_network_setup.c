/**
 * @file app_network_setup.c
 * @brief Fallback soft-AP and unauthenticated web UI for Wi-Fi configuration.
 */

#include "app_network.h"
#include "app_config.h"

#include "injected/esp_wifi.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "app_net_setup";

#define SETUP_AP_SSID       APP_NETWORK_SETUP_AP_SSID
#define SETUP_AP_CHANNEL    1
#define SETUP_AP_MAX_CONN   4
#define POST_BODY_MAX       512

static httpd_handle_t s_httpd;
static bool s_setup_ap_active;
static char s_setup_status[96];

static void set_setup_status(const char *status)
{
    if (status != NULL) {
        snprintf(s_setup_status, sizeof(s_setup_status), "%s", status);
    }
}

bool app_network_setup_ap_active(void)
{
    return s_setup_ap_active;
}

static void html_escape(const char *src, char *dst, size_t dst_len)
{
    size_t j = 0;
    if (src == NULL || dst_len == 0) {
        return;
    }
    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_len; i++) {
        const char *rep = NULL;
        switch (src[i]) {
        case '&':
            rep = "&amp;";
            break;
        case '<':
            rep = "&lt;";
            break;
        case '>':
            rep = "&gt;";
            break;
        case '"':
            rep = "&quot;";
            break;
        default:
            dst[j++] = src[i];
            continue;
        }
        if (rep != NULL) {
            for (size_t k = 0; rep[k] != '\0' && j + 1 < dst_len; k++) {
                dst[j++] = rep[k];
            }
        }
    }
    dst[j] = '\0';
}

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

static void url_decode_inplace(char *s)
{
    if (s == NULL) {
        return;
    }
    char *src = s;
    char *dst = s;
    while (*src != '\0') {
        if (src[0] == '+') {
            *dst++ = ' ';
            src++;
        } else if (src[0] == '%' && src[1] != '\0' && src[2] != '\0') {
            int hi = hex_nibble(src[1]);
            int lo = hex_nibble(src[2]);
            if (hi >= 0 && lo >= 0) {
                *dst++ = (char)((hi << 4) | lo);
                src += 3;
            } else {
                *dst++ = *src++;
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static bool form_value(const char *body, const char *key, char *out, size_t out_len)
{
    if (body == NULL || key == NULL || out == NULL || out_len == 0) {
        return false;
    }

    out[0] = '\0';
    size_t key_len = strlen(key);

    for (const char *p = body; *p != '\0';) {
        const char *amp = strchr(p, '&');
        size_t chunk_len = amp != NULL ? (size_t)(amp - p) : strlen(p);
        const char *eq = memchr(p, '=', chunk_len);
        if (eq != NULL) {
            size_t name_len = (size_t)(eq - p);
            if (name_len == key_len && strncmp(p, key, key_len) == 0) {
                size_t val_len = chunk_len - name_len - 1;
                if (val_len >= out_len) {
                    val_len = out_len - 1;
                }
                memcpy(out, eq + 1, val_len);
                out[val_len] = '\0';
                url_decode_inplace(out);
                return true;
            }
        }
        if (amp == NULL) {
            break;
        }
        p = amp + 1;
    }
    return false;
}

static esp_err_t read_post_body(httpd_req_t *req, char *body, size_t body_len)
{
    if (body_len == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    body[0] = '\0';
    if (req->content_len <= 0) {
        return ESP_OK;
    }
    if ((size_t)req->content_len >= body_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    int received = httpd_req_recv(req, body, (size_t)req->content_len);
    if (received <= 0) {
        return ESP_FAIL;
    }
    body[received] = '\0';
    return ESP_OK;
}

static esp_err_t redirect_root(httpd_req_t *req)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");

    httpd_resp_sendstr_chunk(req,
                             "<!DOCTYPE html><html><head>"
                             "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                             "<title>TimeDisk Wi-Fi Setup</title>"
                             "<style>"
                             "body{font-family:system-ui,sans-serif;max-width:640px;margin:24px auto;padding:0 16px;"
                             "background:#1a1024;color:#fff}"
                             "h1{font-size:1.5rem} .card{background:#2a1840;border-radius:16px;padding:16px;margin:16px 0}"
                             "label{display:block;margin:8px 0 4px;color:#cbb8e0}"
                             "input{width:100%;box-sizing:border-box;padding:10px;border-radius:10px;border:0;"
                             "margin-bottom:8px;font-size:1rem}"
                             "button,.btn{display:inline-block;padding:10px 16px;border:0;border-radius:10px;"
                             "font-size:1rem;cursor:pointer;margin-top:8px;margin-right:8px}"
                             ".primary{background:#6BCA24;color:#102010}.danger{background:#c44;color:#fff}"
                             ".muted{background:#5a3d7a;color:#fff}.net{margin:12px 0;padding:12px;"
                             "background:#1f1230;border-radius:12px}"
                             "form{display:inline}"
                             "</style></head><body>"
                             "<h1>TimeDisk Wi-Fi Setup</h1>"
                             "<p>Configure saved networks, then tap <strong>Connect</strong>.</p>");

    const int count = app_config_wifi_network_count();
    if (count == 0) {
        httpd_resp_sendstr_chunk(req, "<div class=\"card\"><p>No networks saved yet.</p></div>");
    } else {
        httpd_resp_sendstr_chunk(req, "<div class=\"card\"><h2>Saved networks</h2>");
        for (int i = 0; i < count; i++) {
            const app_wifi_network_t *net = app_config_wifi_network_get(i);
            if (net == NULL) {
                continue;
            }
            char ssid_esc[APP_WIFI_SSID_MAX * 6];
            html_escape(net->ssid, ssid_esc, sizeof(ssid_esc));

            char block[768];
            snprintf(block, sizeof(block),
                     "<div class=\"net\"><strong>%s</strong>"
                     "<form method=\"POST\" action=\"/delete\">"
                     "<input type=\"hidden\" name=\"index\" value=\"%d\">"
                     "<button type=\"submit\" class=\"danger\">Delete</button></form>"
                     "<form method=\"POST\" action=\"/edit\">"
                     "<input type=\"hidden\" name=\"index\" value=\"%d\">"
                     "<label>Name</label><input name=\"ssid\" value=\"%s\" required>"
                     "<label>Password</label><input name=\"password\" type=\"password\" "
                     "placeholder=\"Leave blank to keep saved\">"
                     "<button type=\"submit\" class=\"muted\">Update</button></form></div>",
                     ssid_esc, i, i, ssid_esc);
            httpd_resp_sendstr_chunk(req, block);
        }
        httpd_resp_sendstr_chunk(req, "</div>");
    }

    char add_block[512];
    snprintf(add_block, sizeof(add_block),
             "<div class=\"card\"><h2>Add network</h2>"
             "<form method=\"POST\" action=\"/add\">"
             "<label>Network name (SSID)</label>"
             "<input name=\"ssid\" maxlength=\"32\" required>"
             "<label>Password</label>"
             "<input name=\"password\" type=\"password\" maxlength=\"64\">"
             "<button type=\"submit\" class=\"primary\">Add network</button>"
             "</form></div>"
             "<div class=\"card\">"
             "<form method=\"POST\" action=\"/connect\">"
             "<button type=\"submit\" class=\"primary\">Connect</button>"
             "</form>"
             "<p>Device will leave setup mode and try saved networks in order.</p>"
             "</div></body></html>");

    httpd_resp_sendstr_chunk(req, add_block);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t add_post_handler(httpd_req_t *req)
{
    char body[POST_BODY_MAX];
    if (read_post_body(req, body, sizeof(body)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }

    char ssid[APP_WIFI_SSID_MAX];
    char password[APP_WIFI_PASSWORD_MAX];
    form_value(body, "ssid", ssid, sizeof(ssid));
    form_value(body, "password", password, sizeof(password));

    if (ssid[0] != '\0') {
        int index = app_config_wifi_network_count();
        if (app_config_wifi_network_set(index, ssid, password, true) == ESP_OK) {
            app_config_save_network();
        }
    }
    return redirect_root(req);
}

static esp_err_t delete_post_handler(httpd_req_t *req)
{
    char body[POST_BODY_MAX];
    if (read_post_body(req, body, sizeof(body)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }

    char index_str[8];
    if (form_value(body, "index", index_str, sizeof(index_str))) {
        int index = atoi(index_str);
        if (app_config_wifi_network_delete(index) == ESP_OK) {
            app_config_save_network();
        }
    }
    return redirect_root(req);
}

static esp_err_t edit_post_handler(httpd_req_t *req)
{
    char body[POST_BODY_MAX];
    if (read_post_body(req, body, sizeof(body)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }

    char index_str[8];
    char ssid[APP_WIFI_SSID_MAX];
    char password[APP_WIFI_PASSWORD_MAX];
    form_value(body, "index", index_str, sizeof(index_str));
    form_value(body, "ssid", ssid, sizeof(ssid));
    form_value(body, "password", password, sizeof(password));

    int index = atoi(index_str);
    const app_wifi_network_t *existing = app_config_wifi_network_get(index);
    if (existing != NULL && ssid[0] != '\0') {
        const char *pw = existing->password;
        bool pw_set = existing->password_set;
        if (password[0] != '\0') {
            pw = password;
            pw_set = true;
        }
        if (app_config_wifi_network_set(index, ssid, pw, pw_set) == ESP_OK) {
            app_config_save_network();
        }
    }
    return redirect_root(req);
}

static esp_err_t connect_post_handler(httpd_req_t *req)
{
    (void)req;
    ESP_LOGI(TAG, "connect requested from web UI");
    app_network_reconnect_sta();
    return redirect_root(req);
}

static esp_err_t setup_http_start(void)
{
    if (s_httpd != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 6144;

    esp_err_t err = httpd_start(&s_httpd, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = root_get_handler},
        {.uri = "/add", .method = HTTP_POST, .handler = add_post_handler},
        {.uri = "/delete", .method = HTTP_POST, .handler = delete_post_handler},
        {.uri = "/edit", .method = HTTP_POST, .handler = edit_post_handler},
        {.uri = "/connect", .method = HTTP_POST, .handler = connect_post_handler},
    };

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        err = httpd_register_uri_handler(s_httpd, &routes[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "register %s failed", routes[i].uri);
            httpd_stop(s_httpd);
            s_httpd = NULL;
            return err;
        }
    }

    ESP_LOGI(TAG, "web UI started");
    return ESP_OK;
}

esp_err_t app_network_web_ui_start(void)
{
    return setup_http_start();
}

bool app_network_web_ui_active(void)
{
    return s_httpd != NULL;
}

static void setup_http_stop(void)
{
    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
}

static esp_err_t setup_ap_begin(void)
{
    esp_netif_create_default_wifi_ap();

    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    wifi_config_t ap_cfg = {0};
    snprintf((char *)ap_cfg.ap.ssid, sizeof(ap_cfg.ap.ssid), "%s", SETUP_AP_SSID);
    ap_cfg.ap.ssid_len = (uint8_t)strlen(SETUP_AP_SSID);
    ap_cfg.ap.channel = SETUP_AP_CHANNEL;
    ap_cfg.ap.max_connection = SETUP_AP_MAX_CONN;
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_ip_info_t ip = {0};
    if (ap != NULL && esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
        char msg[96];
        snprintf(msg, sizeof(msg), "Join %s · http://" IPSTR, SETUP_AP_SSID, IP2STR(&ip.ip));
        set_setup_status(msg);
    } else {
        set_setup_status("Join " SETUP_AP_SSID " · http://192.168.4.1");
    }

    return ESP_OK;
}

esp_err_t app_network_setup_ap_start(void)
{
    if (s_setup_ap_active) {
        return ESP_OK;
    }

    esp_err_t err = setup_ap_begin();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AP start failed: %s", esp_err_to_name(err));
        return err;
    }

    err = setup_http_start();
    if (err != ESP_OK) {
        esp_wifi_stop();
        return err;
    }

    s_setup_ap_active = true;
    ESP_LOGI(TAG, "setup AP active (%s)", s_setup_status);
    return ESP_OK;
}

void app_network_setup_ap_stop(void)
{
    if (!s_setup_ap_active) {
        return;
    }

    esp_wifi_stop();
    s_setup_ap_active = false;
    ESP_LOGI(TAG, "setup AP stopped (web UI kept running)");
}

const char *app_network_setup_status_text(void)
{
    return s_setup_status;
}

bool app_network_get_web_ui_url(char *out, size_t out_len)
{
    char ip[40];

    if (out == NULL || out_len == 0) {
        return false;
    }
    out[0] = '\0';
    if (s_httpd == NULL) {
        return false;
    }
    if (!app_network_get_device_ip(ip, sizeof(ip))) {
        return false;
    }

    snprintf(out, out_len, "http://%s", ip);
    return true;
}

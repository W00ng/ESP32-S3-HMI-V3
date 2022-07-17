/**
 * @file ui_prov.c
 * @brief Provision UI src.
 * @version 0.1
 * @date 2021-03-07
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 * 
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ui_prov.h"

static const char *TAG = "ui_prov";

#define COLOR_BAR   lv_color_make(86, 94, 102)
#define COLOR_THEME lv_color_make(252, 199, 0)
#define COLOR_DEEP  lv_color_make(246, 174, 61)
#define COLOR_TEXT  lv_color_make(56, 56, 56)
#define COLOR_BG    lv_color_make(238, 241, 245)

static lv_obj_t *page_wifi = NULL;
static lv_obj_t *wifi_progress = NULL;
static lv_obj_t *wifi_list_left = NULL;
static lv_obj_t *wifi_list_right = NULL;
static lv_obj_t *keyboard = NULL;
static lv_obj_t *textarea_pswd = NULL;
static lv_obj_t *btn_connect = NULL;
static lv_obj_t *label_ssid = NULL;
static lv_obj_t *msg = NULL;

static char *ap_ssid = NULL;

/*!< Callback function defination */
static void wifi_list_cb(lv_event_t *event);
static void btn_connect_cb(lv_event_t *event);
static void keyboard_event_cb(lv_event_t *event);
static void msg_event_cb(lv_event_t *event);

/*!< Private function forward declaration */
void ui_set_list_count(size_t item);
static void btn_pwd_mode_cb(lv_event_t *event);


void ui_prov_init(void)
{
    page_wifi = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page_wifi, 750, 400);
    lv_obj_set_style_radius(page_wifi, 15, LV_STATE_DEFAULT);
    lv_obj_align(page_wifi, LV_ALIGN_CENTER, 0, 20);

    wifi_list_left = lv_table_create(page_wifi);
    lv_table_set_col_cnt(wifi_list_left, 3);
    lv_table_set_row_cnt(wifi_list_left, 5);
    // lv_obj_set_style_border_width(wifi_list_left, LV_TABLE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_text_font(wifi_list_left, &font_en_20, LV_STATE_DEFAULT);

    wifi_list_right = lv_table_create(page_wifi);
    lv_table_set_col_cnt(wifi_list_right, 3);
    lv_table_set_row_cnt(wifi_list_right, 5);
    // lv_obj_set_style_local_border_width(wifi_list_right, LV_TABLE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_text_font(wifi_list_right, &font_en_20, LV_STATE_DEFAULT);

    /* Set Wi-Fi list's column width */
    lv_table_set_col_width(wifi_list_left, 0, 200);
    lv_table_set_col_width(wifi_list_left, 1, 50);
    lv_table_set_col_width(wifi_list_left, 2, 50);

    lv_table_set_col_width(wifi_list_right, 0, 200);
    lv_table_set_col_width(wifi_list_right, 1, 50);
    lv_table_set_col_width(wifi_list_right, 2, 50);

    lv_obj_align(wifi_list_left, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(wifi_list_right, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* Set Wi-Fi list's click event */
    lv_obj_add_event_cb(wifi_list_left, wifi_list_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(wifi_list_right, wifi_list_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /*!< Create a progress bar to show scanning progress */
    wifi_progress = lv_bar_create(lv_scr_act());
    lv_obj_add_flag(wifi_progress, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(wifi_progress, 750, 6);
    lv_bar_set_range(wifi_progress, 0, 21);
    lv_bar_set_value(wifi_progress, 0, LV_ANIM_ON);
    lv_obj_align(wifi_progress, LV_ALIGN_TOP_MID, 0, 25);

    /* Initiaize done, show Wi-Fi list */
    // lv_obj_clear_flag(page_wifi, LV_OBJ_FLAG_HIDDEN);
}

static void ui_show_pswd_textarea(void)
{
    /*!< Hide Wi-Fi lists */
    lv_obj_add_flag(wifi_list_left, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifi_list_right, LV_OBJ_FLAG_HIDDEN);

    /*!< Create textarea object if not created yet */
    if (NULL == textarea_pswd) {
        textarea_pswd = lv_textarea_create(lv_scr_act());
        lv_textarea_set_max_length(textarea_pswd, 32);
        lv_textarea_set_one_line(textarea_pswd, true);
        lv_textarea_set_text(textarea_pswd, "");
        lv_textarea_set_password_mode(textarea_pswd, true);
        lv_textarea_set_placeholder_text(textarea_pswd, " Password");
        lv_obj_set_size(textarea_pswd, 520, 64);
        lv_obj_clear_flag(textarea_pswd, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(textarea_pswd, LV_ALIGN_TOP_LEFT, 70, 140);
        lv_obj_set_style_text_font(textarea_pswd, &font_en_24, LV_STATE_DEFAULT);
        // lv_obj_set_style_border_width(textarea_pswd, LV_TEXTAREA_PART_BG, LV_STATE_DEFAULT, 0);
        lv_obj_set_style_radius(textarea_pswd, 6, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(textarea_pswd, COLOR_BG, LV_STATE_DEFAULT);

        /*!< Create a 'password mode' button */
        lv_obj_t *btn_pwd_mode = lv_btn_create(textarea_pswd);
        lv_obj_set_height(btn_pwd_mode, 50);
        lv_obj_t *label_pwd_mode = lv_label_create(btn_pwd_mode);
        lv_obj_set_style_text_font(label_pwd_mode, &font_kb_24, LV_STATE_DEFAULT);
        lv_label_set_text_static(label_pwd_mode, LV_SYMBOL_EYE_CLOSE);
        lv_obj_center(label_pwd_mode);

        lv_obj_align(btn_pwd_mode, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_user_data(btn_pwd_mode, (void *) label_pwd_mode);
        lv_obj_add_event_cb(btn_pwd_mode, btn_pwd_mode_cb, LV_EVENT_CLICKED, NULL);


        btn_connect = lv_btn_create(lv_scr_act());
        lv_obj_set_height(btn_connect, 50);
        lv_obj_t *label_connect = lv_label_create(btn_connect);
        lv_obj_set_style_text_font(label_connect, &font_en_bold_24, LV_STATE_DEFAULT);        
        lv_label_set_text_static(label_connect, "Connect");
        lv_obj_center(label_connect);

        lv_obj_align_to(btn_connect, textarea_pswd, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        lv_obj_add_event_cb(btn_connect, btn_connect_cb, LV_EVENT_CLICKED, (void *) false);
    } 
    else {
        lv_obj_clear_flag(textarea_pswd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_connect, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_set_list_count(size_t item)
{
    if (0 != item % 2) {
        lv_table_set_row_cnt(wifi_list_left, item / 2 + 1);
        lv_table_set_row_cnt(wifi_list_right, item / 2 + 1);    
    } else {
        lv_table_set_row_cnt(wifi_list_left, item / 2);
        lv_table_set_row_cnt(wifi_list_right, item / 2);
    }
}

void ui_set_list_text(size_t index, const char *ap_name, int rssi, bool secure)
{
    static const uint16_t ssid_col_num = 0;
    static const uint16_t rssi_col_num = 1;
    static const uint16_t lock_col_num = 2;
    
    static const int rssi_min = -80;
    static const int rssi_max = -50;

    static lv_obj_t *obj_list = NULL;

    if (0 == index % 2) {
        obj_list = wifi_list_left;
    } else {
        obj_list = wifi_list_right;
    }

    /*!< Set AP name */
    // lv_table_set_cell_crop(obj_list, index / 2, ssid_col_num, true);
    lv_table_set_cell_value(obj_list, index / 2, ssid_col_num, ap_name);

    /*!< Set Wi-Fi logo according to RSSI */
    if (rssi < rssi_min) {
        lv_table_set_cell_value(obj_list, index / 2, rssi_col_num, LV_SYMBOL_EXTRA_WIFI_MIN);
    } else if (rssi > rssi_max) {
        lv_table_set_cell_value(obj_list, index / 2, rssi_col_num, LV_SYMBOL_EXTRA_WIFI_MAX);
    } else {
        lv_table_set_cell_value(obj_list, index / 2, rssi_col_num, LV_SYMBOL_EXTRA_WIFI_MID);
    }

    /*!< Show Wi-Fi secure info */
    if (secure) {
        lv_table_set_cell_value(obj_list, index / 2, lock_col_num, LV_SYMBOL_EXTRA_LOCK);
    }
}

static void wifi_list_cb(lv_event_t *event)
{
    static uint16_t _row = 0, _col = 0;
    lv_obj_t *obj = (lv_obj_t *) event->target;

    lv_table_get_selected_cell(obj, &_row, &_col);
    
    /* DO NOT show password enter page if SSID is NULL */
    if ('\0' == strcmp(lv_table_get_cell_value(obj, _row, 0), "")) {
        ESP_LOGE(TAG, "Illigal SSID");
        return;
    }

    ap_ssid = (char *)lv_table_get_cell_value(obj, _row, 0);

    if (NULL != keyboard) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    } else {
        /*!< Create keyboard and set style and location */
        keyboard = lv_keyboard_create(lv_scr_act());
        lv_obj_set_style_text_font(keyboard, &font_kb_24, LV_STATE_DEFAULT);
        lv_keyboard_set_popovers(keyboard, true);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

        /*!< Set keyboard event callback for hiding itself */
        lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_CANCEL, (void *) false);

        /*!< Show password textarea */
        ui_show_pswd_textarea();

        /*!< Show SSID name */
        if (NULL == label_ssid) {
            label_ssid = lv_label_create(lv_scr_act());
            lv_label_set_text(label_ssid, "");
            lv_obj_set_style_text_font(label_ssid, &font_en_bold_28, LV_STATE_DEFAULT);
            lv_obj_align(label_ssid, LV_ALIGN_TOP_LEFT, 70, 90);
        }
        lv_obj_clear_flag(label_ssid, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(label_ssid, "Enter password for \"%s\"", lv_table_get_cell_value(obj, _row, 0));

        /*!< Register textarea for keyboard */
        lv_keyboard_set_textarea(keyboard, textarea_pswd);
    }
}

static void keyboard_event_cb(lv_event_t *event)
{
    if(keyboard) 
    {
        lv_obj_del(keyboard);
        keyboard = NULL;

        lv_obj_add_flag(label_ssid, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_connect, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(textarea_pswd, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(wifi_list_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(wifi_list_right, LV_OBJ_FLAG_HIDDEN);
    }    
}

static void btn_pwd_mode_cb(lv_event_t *event)
{
    lv_obj_t *btn = (lv_obj_t *) event->target;
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;

    lv_textarea_set_password_mode(textarea_pswd, !lv_textarea_get_password_mode(textarea_pswd));
    lv_label_set_text_static(lab, lv_textarea_get_password_mode(textarea_pswd) ? LV_SYMBOL_EYE_CLOSE: LV_SYMBOL_EYE_OPEN);
}

void ui_connected(bool connected)
{
    if (connected) {
        ESP_LOGI(TAG, "connect AP SUCCESS!!!");
        lv_obj_del(label_ssid);    
        lv_obj_del(textarea_pswd);
        lv_obj_del(btn_connect);
        lv_obj_del(keyboard);

        lv_obj_del(wifi_list_left);
        lv_obj_del(wifi_list_right);
    } else {
        msg = lv_msgbox_create(lv_scr_act(), "ERROR:", "Failed connect to AP", NULL, true);
        lv_obj_set_style_text_font(msg, &font_en_bold_24, LV_STATE_DEFAULT);
        lv_obj_center(msg);
        lv_obj_add_event_cb(msg, msg_event_cb, LV_EVENT_DELETE, (void *) false);
    }
}

void ui_show_ap_info(wifi_ap_record_t *ap_info)
{
    static const char *column_str[] = {
        "SSID", "BSSID", "RSSI", "Channel", "Auth Mode",
    };

    static const char *wifi_second_chan_str[] = {
        "HT20", "HT40+", "HT40-",
    };

    static const char *wifi_auth_mode_str[] = {
        "OPEN", "WEP", "WPA_PSK", "WPA2_PSK",
        "WPA_WPA2_PSK", "WPA2_ENTERPRISE",
        "WPA3_PSK", "WPA2_WPA3_PSK", "WAPI_PSK",
    };

    lv_obj_t *table = lv_table_create(page_wifi);
    // lv_obj_set_style_local_border_width(table, LV_TABLE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_text_font(table, &font_en_20, LV_STATE_DEFAULT);
    lv_table_set_row_cnt(table, sizeof column_str / sizeof column_str[0]);
    lv_table_set_col_cnt(table, 2);
    lv_table_set_col_width(table, 0, 200);
    lv_table_set_col_width(table, 1, 400);

    /*!< Add items text to col 1 */
    for (size_t i = 0; i < sizeof column_str / sizeof column_str[0]; i++) {
        lv_table_set_cell_value(table, i, 0, column_str[i]);
    }

    lv_table_set_cell_value_fmt(table, 0, 1,    /*!< SSID */
        "%s",
        (const char *)ap_info->ssid);
    lv_table_set_cell_value_fmt(table, 1, 1,    /*!< BSSID */
        "%02X:%02X:%02X:%02X:%02X:%02X",
        ap_info->bssid[0], ap_info->bssid[1], ap_info->bssid[2],
        ap_info->bssid[3], ap_info->bssid[4], ap_info->bssid[5]);
    
    lv_table_set_cell_value_fmt(table, 2, 1,    /*!< RSSI */
        "%d", ap_info->rssi);

    lv_table_set_cell_value_fmt(table, 3, 1,    /*!< AP Channel */
        "%u - %s", ap_info->primary, wifi_second_chan_str[(int)ap_info->second]);

    lv_table_set_cell_value_fmt(table, 4, 1,    /*!< Auth mode */
        "%s", wifi_auth_mode_str[(int)ap_info->authmode]);

    lv_obj_align_to(table, page_wifi, LV_ALIGN_CENTER, 0, 0);
}

static void wifi_scan_progress_task(void *arg)
{
    while (1)
    {
        if (lv_bar_get_value(wifi_progress) < lv_bar_get_max_value(wifi_progress)) {
            lv_bar_set_value(wifi_progress, lv_bar_get_value(wifi_progress) + 1, LV_ANIM_ON);
        } else {
            lv_obj_add_flag(wifi_progress, LV_OBJ_FLAG_HIDDEN);
            vTaskDelete(NULL);               
        }
        vTaskDelay(100/portTICK_PERIOD_MS);     
    }
}

void ui_scan_start(void)
{
    lv_bar_set_value(wifi_progress, 0, LV_ANIM_ON);
    lv_obj_clear_flag(wifi_progress, LV_OBJ_FLAG_HIDDEN);

    xTaskCreate(wifi_scan_progress_task, "wifi_scan_progress_task", 4096, NULL, 2, NULL);
}

static void btn_connect_cb(lv_event_t *event)
{
    lv_obj_t *btn = (lv_obj_t *) event->target;

    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    ESP_LOGI(TAG, "start connect AP: %s     %s", ap_ssid, lv_textarea_get_text(textarea_pswd));

    wifi_sta_set_ssid(ap_ssid);
    wifi_sta_set_pswd(lv_textarea_get_text(textarea_pswd));
    wifi_sta_connect();
}

static void msg_event_cb(lv_event_t *event)
{
    // lv_obj_t *obj = (lv_obj_t *) event->target;
    // lv_obj_del(obj);

    lv_obj_add_flag(btn_connect, LV_OBJ_FLAG_CLICKABLE);    
}



/**
 * @file ui_power.c
 * @brief UI src.
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

#include "ui_power.h"


static const char *TAG = "ui_power";

ext_io_t io_conf = BSP_EXT_IO_OUTPUT_CONFIG();
ext_io_t io_level = BSP_EXT_IO_OUTPUT_DEFAULT_LEVEL();

/* Color value defination */
#define COLOR_BG    lv_color_make(238, 241, 245)
#define COLOR_DEEP  lv_color_make(242, 173, 71)
#define COLOR_LIGHT lv_color_make(253, 200, 0)
#define COLOR_THEME lv_color_make(252, 199, 0)

static lv_obj_t *btn_sleep = NULL;
static lv_obj_t *arc_bat_voltage = NULL;
static lv_obj_t *label_arc_voltage = NULL;
static lv_obj_t *label_bat_voltage = NULL;
static lv_obj_t *label_led = NULL;

/* Objects for MEMS detail page */
static lv_obj_t *label_acc = NULL;
static lv_obj_t *label_acc_x = NULL;
static lv_obj_t *label_acc_y = NULL;
static lv_obj_t *label_acc_z = NULL;
static lv_obj_t *label_gyro = NULL;
static lv_obj_t *label_gyro_x = NULL;
static lv_obj_t *label_gyro_y = NULL;
static lv_obj_t *label_gyro_z = NULL;
static lv_obj_t *bar_acc = NULL;
static lv_obj_t *bar_acc_x = NULL;
static lv_obj_t *bar_acc_y = NULL;
static lv_obj_t *bar_acc_z = NULL;
static lv_obj_t *bar_gyro = NULL;
static lv_obj_t *bar_gyro_x = NULL;
static lv_obj_t *bar_gyro_y = NULL;
static lv_obj_t *bar_gyro_z = NULL;


static lv_obj_t *sw_ext_io[3];
static lv_obj_t *label_ext_io[3];
static lv_obj_t *sw_esp_io0;
static lv_obj_t *sw_esp_io33;
static lv_obj_t *sw_esp_io46;
static lv_obj_t *label_esp_io0;
static lv_obj_t *label_esp_io33;
static lv_obj_t *label_esp_io46;

#if 1
void gpio_config_input(gpio_num_t gpio_num, uint8_t mode)
{
    gpio_config_t io_conf = {};
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << gpio_num;

    switch (mode)
    {
    case 0:   // float
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;        
        break;

    case 1:  // pull down
        io_conf.pull_down_en = 1;
        io_conf.pull_up_en = 0;        
        break;

    case 2:  // pull up
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;        
        break;

    default:
        break;
    }

    gpio_config(&io_conf);
}

void gpio_config_output(gpio_num_t gpio_num, uint8_t level)
{
    gpio_config_t io_conf = {};
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << gpio_num;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    if (level) gpio_set_level(gpio_num, 1);
    else gpio_set_level(gpio_num, 0);
}

void all_gpio_sleep_config(void)
{
    for (size_t i = 1; i <= 21; i++)
    {
        gpio_config_input(i, 0);
    }

    gpio_config_input(GPIO_NUM_33, 0);

    for (size_t i = 34; i <= 37; i++)
    {
        gpio_config_input(i, 0);  //其他模式
    }

    gpio_config_input(GPIO_NUM_38, 0);

    gpio_config_input(GPIO_NUM_39, 0);  //其他模式
    gpio_config_input(GPIO_NUM_40, 0);

    for (size_t i = 41; i <= 48; i++)
    {
        gpio_config_input(i, 0);
    }
}
#endif

static void btn_sleep_cb(lv_event_t *event)
{
    // ESP_ERROR_CHECK(fx5x06_deep_sleep());

    // ESP_ERROR_CHECK(mpu6050_init());
    // mpu6050_sleep();

    /* Shut down all programmable power */
    static ext_io_t io_conf_sleep = BSP_EXT_IO_DEFAULT_CONFIG();
    static ext_io_t io_level_sleep = BSP_EXT_IO_SLEEP_LEVEL();    
    tca9554_set_configuration(io_conf_sleep.val);
    tca9554_write_output_pins(io_level_sleep.val);
    /* Wait IO status written to IO expander */
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Enabling EXT0 wakeup on pin GPIO%d", GPIO_KEY);
    esp_sleep_enable_ext0_wakeup(GPIO_KEY, 0);   //input level which will trigger wakeup (0=low, 1=high)
    rtc_gpio_pullup_en(GPIO_KEY);
    rtc_gpio_pulldown_dis(GPIO_KEY);

    ESP_LOGI(TAG, "esp32 enter deep sleep");

    /* Enter deep sleep */
    esp_deep_sleep_start();
}

static void sw_event_cb(lv_event_t * event)
{
    lv_obj_t * sw = lv_event_get_target(event);
    int id = (size_t)lv_event_get_user_data(event);
    if(lv_obj_has_state(sw, LV_STATE_CHECKED)) 
    {
        switch (id)
        {
            case 0:
                io_level.ext_io0 = 1;
                break;
            case 1:
                io_level.ext_io1 = 1;
                break;
            case 2:
                io_level.ext_io2 = 1;
                break;
            case 3:
                gpio_set_level(GPIO_NUM_0, 1);
            case 4:
                gpio_set_level(GPIO_NUM_33, 1);
                break;
            case 5:
                gpio_set_level(GPIO_NUM_46, 1);
                break;

            default:
                break;
        }
    }else
    {
        switch (id)
        {
            case 0:
                io_level.ext_io0 = 0;
                break;
            case 1:
                io_level.ext_io1 = 0;
                break;
            case 2:
                io_level.ext_io2 = 0;
                break;
            case 3:
                gpio_set_level(GPIO_NUM_0, 0);
            case 4:
                gpio_set_level(GPIO_NUM_33, 0);
                break;
            case 5:
                gpio_set_level(GPIO_NUM_46, 0);
                break;

            default:
                break;
        }
    }
    tca9554_write_output_pins(io_level.val);
}

static void bat_voltage_update_task(void *args)
{
    static int count = 0;
    static float voltage = 0;
    static uint16_t adc_val = 0;
    static char fmt_text[8] = "0.00V";

    while (1) 
    {
        count++;
        if (count <= 10) {
            adc_val = adc_get_voltage();
            voltage += adc_val * 2 / 1000.0f;
        } else {
            voltage /= 10.0f;
            lv_arc_set_value(arc_bat_voltage, 100 * voltage);
            sprintf(fmt_text, "%.2fV", voltage);
            lv_label_set_text(label_arc_voltage, fmt_text);
            count = 0;
            voltage = 0;
        }        
        vTaskDelay((200) / portTICK_PERIOD_MS);   
    }
    vTaskDelete(NULL);
}

static void mpu6050_update_task(void *data)
{
    static mpu6050_acce_value_t acc_val;
    static mpu6050_gyro_value_t gyro_val;
    while (1) 
    {
        /*!< Get MEMS data */
        mpu6050_get_acce(&acc_val);
        mpu6050_get_gyro(&gyro_val);

        lv_bar_set_value(bar_acc_x, acc_val.acce_x * 100, LV_ANIM_ON);
        lv_bar_set_value(bar_acc_y, acc_val.acce_y * 100, LV_ANIM_ON);
        lv_bar_set_value(bar_acc_z, acc_val.acce_z * 100, LV_ANIM_ON);
        lv_bar_set_value(bar_gyro_x, gyro_val.gyro_x, LV_ANIM_ON);
        lv_bar_set_value(bar_gyro_y, gyro_val.gyro_y, LV_ANIM_ON);
        lv_bar_set_value(bar_gyro_z, gyro_val.gyro_z, LV_ANIM_ON);

        vTaskDelay((100) / portTICK_PERIOD_MS);   
    }
    vTaskDelete(NULL);
}


static void ui_mems_init(void)
{
    label_acc = lv_label_create(lv_scr_act());
    label_gyro = lv_label_create(lv_scr_act());
    label_acc_x = lv_label_create(lv_scr_act());
    label_acc_y = lv_label_create(lv_scr_act());
    label_acc_z = lv_label_create(lv_scr_act());
    label_gyro_x = lv_label_create(lv_scr_act());
    label_gyro_y = lv_label_create(lv_scr_act());
    label_gyro_z = lv_label_create(lv_scr_act());

    lv_label_set_text(label_acc, "Acceleration");
    lv_obj_set_style_text_font(label_acc, &font_en_bold_20, LV_STATE_DEFAULT);    
    lv_label_set_text(label_gyro, "Angular Velocity");
    lv_obj_set_style_text_font(label_gyro, &font_en_bold_20, LV_STATE_DEFAULT);        
    lv_label_set_text(label_acc_x, "X");
    lv_label_set_text(label_acc_y, "Y");
    lv_label_set_text(label_acc_z, "Z");
    lv_label_set_text(label_gyro_x, "X");
    lv_label_set_text(label_gyro_y, "Y");
    lv_label_set_text(label_gyro_z, "Z");

    bar_acc = lv_bar_create(lv_scr_act());
    bar_gyro = lv_bar_create(lv_scr_act());

    bar_acc_x = lv_bar_create(lv_scr_act());
    bar_acc_y = lv_bar_create(lv_scr_act());
    bar_acc_z = lv_bar_create(lv_scr_act());
    bar_gyro_x = lv_bar_create(lv_scr_act());
    bar_gyro_y = lv_bar_create(lv_scr_act());
    bar_gyro_z = lv_bar_create(lv_scr_act());
 
    lv_obj_set_size(bar_acc_x, 140, 10);
    lv_obj_set_size(bar_acc_y, 140, 10);
    lv_obj_set_size(bar_acc_z, 140, 10);
    lv_obj_set_size(bar_gyro_x, 140, 10);
    lv_obj_set_size(bar_gyro_y, 140, 10);
    lv_obj_set_size(bar_gyro_z, 140, 10);

    lv_obj_set_style_bg_color(bar_acc, COLOR_DEEP, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar_gyro, COLOR_LIGHT, LV_STATE_DEFAULT);

    /* Set bars' size */
    lv_obj_set_size(bar_acc, 50, 10);
    lv_obj_set_size(bar_gyro, 50, 10);

    lv_bar_set_range(bar_acc_x, -160, 160);
    lv_bar_set_range(bar_acc_y, -160, 160);
    lv_bar_set_range(bar_acc_z, -160, 160);
    lv_bar_set_range(bar_gyro_x, -360, 360);
    lv_bar_set_range(bar_gyro_y, -360, 360);
    lv_bar_set_range(bar_gyro_z, -360, 360);

    lv_obj_align_to(bar_acc_x, NULL, LV_ALIGN_TOP_LEFT, 260, 180);
    lv_obj_align_to(bar_acc_y, bar_acc_x, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);
    lv_obj_align_to(bar_acc_z, bar_acc_y, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);
    lv_obj_align_to(bar_gyro_x, bar_acc_x, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    lv_obj_align_to(bar_gyro_y, bar_gyro_x, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);
    lv_obj_align_to(bar_gyro_z, bar_gyro_y, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);

    lv_obj_align_to(bar_acc, bar_acc_x, LV_ALIGN_OUT_TOP_LEFT, 0, -60);
    lv_obj_align_to(bar_gyro, bar_gyro_x, LV_ALIGN_OUT_TOP_LEFT, 0, -60);

    lv_obj_align_to(label_acc, bar_acc, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_gyro, bar_gyro, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    
    lv_obj_align_to(label_acc_x, bar_acc_x, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_acc_y, bar_acc_y, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_acc_z, bar_acc_z, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_gyro_x, bar_gyro_x, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_gyro_y, bar_gyro_y, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_obj_align_to(label_gyro_z, bar_gyro_z, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
}

void ui_sensor_init(void)
{
    /* Create a button to enter sleep mode */
    btn_sleep = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_sleep, LV_ALIGN_CENTER, 0, 180);
    lv_obj_t *label_sleep = lv_label_create(btn_sleep);
    lv_obj_set_style_text_font(label_sleep, &font_en_24, LV_STATE_DEFAULT);        
    lv_label_set_text_static(label_sleep, "Sleep");
    lv_obj_center(label_sleep);
    lv_obj_add_event_cb(btn_sleep, btn_sleep_cb, LV_EVENT_CLICKED, (void *) false);

    /* Create battery voltage arc */
    arc_bat_voltage = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc_bat_voltage, 180, 180);
    // lv_arc_set_rotation(arc_bat_voltage, 135);
    lv_arc_set_bg_angles(arc_bat_voltage, 180, 360);
    lv_arc_set_range(arc_bat_voltage, 300 , 450);
    lv_arc_set_value(arc_bat_voltage, 0);
    lv_obj_align(arc_bat_voltage, LV_ALIGN_CENTER, -280, 50);
    label_arc_voltage = lv_label_create(arc_bat_voltage);
    lv_label_set_text(label_arc_voltage, "0.00V");
    lv_obj_set_style_text_font(label_arc_voltage, &font_en_24, LV_STATE_DEFAULT);
    lv_obj_align(label_arc_voltage, LV_ALIGN_CENTER, 0, -10);

    label_bat_voltage = lv_label_create(lv_scr_act());
    lv_label_set_text_static(label_bat_voltage, "Battery Voltage");
    lv_obj_set_style_text_font(label_bat_voltage, &font_en_bold_24, LV_STATE_DEFAULT);
    lv_obj_align_to(label_bat_voltage, arc_bat_voltage, LV_ALIGN_OUT_TOP_MID, 0, -30);

    ui_mems_init();

    char str[10] = "";
    for (size_t i = 0; i < 3; i++)
    {
        sw_ext_io[i] = lv_switch_create(lv_scr_act());
        lv_obj_align(sw_ext_io[i], LV_ALIGN_CENTER, 250, -150 + 60 * i);
        lv_obj_set_size(sw_ext_io[i], 70, 35);
        lv_obj_set_ext_click_area(sw_ext_io[i], 8);
        lv_obj_add_event_cb(sw_ext_io[i], sw_event_cb, LV_EVENT_VALUE_CHANGED, i);

        label_ext_io[i] = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_ext_io[i], &font_en_18, LV_STATE_DEFAULT);
        sprintf(str, "EXT_IO%d", i);
        lv_label_set_text(label_ext_io[i], str);
        lv_obj_align_to(label_ext_io[i], sw_ext_io[i], LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    }

    sw_esp_io0 = lv_switch_create(lv_scr_act());
    lv_obj_align(sw_esp_io0, LV_ALIGN_CENTER, 250, 30);
    lv_obj_set_size(sw_esp_io0, 70, 35);
    lv_obj_set_ext_click_area(sw_esp_io0, 8);
    lv_obj_add_event_cb(sw_esp_io0, sw_event_cb, LV_EVENT_VALUE_CHANGED, 3);
    sw_esp_io33 = lv_switch_create(lv_scr_act());
    lv_obj_align(sw_esp_io33, LV_ALIGN_CENTER, 250, 90);
    lv_obj_set_size(sw_esp_io33, 70, 35);
    lv_obj_set_ext_click_area(sw_esp_io33, 8);
    lv_obj_add_event_cb(sw_esp_io33, sw_event_cb, LV_EVENT_VALUE_CHANGED, 4);
    sw_esp_io46 = lv_switch_create(lv_scr_act());
    lv_obj_align(sw_esp_io46, LV_ALIGN_CENTER, 250, 150);
    lv_obj_set_size(sw_esp_io46, 70, 35);
    lv_obj_set_ext_click_area(sw_esp_io46, 8);
    lv_obj_add_event_cb(sw_esp_io46, sw_event_cb, LV_EVENT_VALUE_CHANGED, 5);

    label_esp_io0 = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_esp_io0, &font_en_18, LV_STATE_DEFAULT);
    lv_label_set_text(label_esp_io0, "ESP_IO0");
    lv_obj_align_to(label_esp_io0, sw_esp_io0, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    label_esp_io33 = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_esp_io33, &font_en_18, LV_STATE_DEFAULT);
    lv_label_set_text(label_esp_io33, "ESP_IO33");
    lv_obj_align_to(label_esp_io33, sw_esp_io33, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    label_esp_io46 = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_esp_io46, &font_en_18, LV_STATE_DEFAULT);
    lv_label_set_text(label_esp_io46, "ESP_IO46");
    lv_obj_align_to(label_esp_io46, sw_esp_io46, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    
    xTaskCreate(bat_voltage_update_task, "bat_voltage_update_task", 4096, NULL, 2, NULL);
    xTaskCreate(mpu6050_update_task, "mpu6050_update_task", 4096, NULL, 3, NULL);
}




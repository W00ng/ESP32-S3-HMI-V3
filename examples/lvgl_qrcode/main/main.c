/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "tca9554.h"
#include "bsp_lcd.h"
#include "ft5x06.h"
#include "bat_adc.h"
#include "htu21.h"
#include "mpu6050.h"
#include "key.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"
#include "lv_demos.h"
#include "bsp_sdcard.h"
#include "fs_hal.h"


static const char *TAG = "main";


void ui_task(void *arg)
{
    static lv_obj_t *default_src;
    default_src = lv_scr_act();

    lv_style_t style2;
    lv_style_set_bg_color(&style2, lv_palette_main(LV_PALETTE_LIGHT_GREEN));
    lv_obj_add_style(default_src, &style2, _LV_STYLE_STATE_CMP_SAME);

    lv_obj_t * label = lv_label_create(default_src);          /*Add a label to the button*/
    lv_label_set_text(label, "hello_world\n11223344");        /*Set the labels text*/
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 100, 50);
    lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_font(&style, &lv_font_montserrat_28);
    lv_style_set_text_opa(&style, LV_OPA_90);
    lv_style_set_text_letter_space(&style, 5);
    lv_style_set_text_line_space(&style, 20);
    lv_obj_add_style(label, &style, 0); 

    lv_obj_t *slid = lv_slider_create(default_src);
    lv_slider_set_range(slid, 0, 100);
    lv_slider_set_value(slid, 50, LV_ANIM_ON);
    lv_obj_align(slid, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_obj_t *arc = lv_arc_create(default_src);
    lv_arc_set_range(arc, 0 , 300);
    lv_arc_set_value(arc, 150);
    lv_obj_set_pos(arc, 100, 200);

    static char *str = "this is test qrcode!";
    lv_obj_t *qrcode = lv_qrcode_create(default_src, 300, lv_color_black(), lv_color_white());
    lv_qrcode_update(qrcode, str, strlen(str));
    // lv_obj_align(qrcode, LV_ALIGN_TOP_MID, 100, 50); 
    lv_obj_set_pos(qrcode, 450, 50);
    // vTaskDelay((3000) / portTICK_PERIOD_MS);
    // lv_qrcode_delete(qrcode);

    while(1) 
    {
        vTaskDelay((1000) / portTICK_PERIOD_MS);
    }
}

LV_IMG_DECLARE(img_test);
void img_disp_task(void *arg)
{
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_test);
    lv_obj_align(img, LV_ALIGN_CENTER, 0 , 0);

    uint16_t pos = 0;    
    while(1) 
    {
        vTaskDelay((1000) / portTICK_PERIOD_MS); 
        // lv_img_set_angle(img, 0);
        // vTaskDelay((1000) / portTICK_PERIOD_MS);   
        // lv_img_set_angle(img, 900);
        // vTaskDelay((1000) / portTICK_PERIOD_MS);
        // lv_img_set_angle(img, 1800);
        // vTaskDelay((1000) / portTICK_PERIOD_MS);
        // lv_img_set_angle(img, 2700);
        // vTaskDelay((1000) / portTICK_PERIOD_MS);  

        // lv_obj_set_pos(img, pos, 40);
        // vTaskDelay((1000) / portTICK_PERIOD_MS); 
        // pos += 50;
        // if(pos > 450) pos = 0;
    }
}

void lv_tick_task(void *arg)
{
    while(1) 
    {
        vTaskDelay((20) / portTICK_PERIOD_MS);
        lv_task_handler();        
    }
}

void app_main(void)
{
    /* Initialize I2C 400KHz */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));
 
    /* Init TCA9554 */
    ESP_ERROR_CHECK(tca9554_init());
    ext_io_t io_conf = BSP_EXT_IO_DEFAULT_CONFIG();
    ext_io_t io_level = BSP_EXT_IO_DEFAULT_LEVEL();
    ESP_ERROR_CHECK(tca9554_set_configuration(io_conf.val));
    ESP_ERROR_CHECK(tca9554_write_output_pins(io_level.val));

    /* LVGL init */
    lv_init();                  //内核初始化
    lv_port_disp_init();	    //接口初始化
    lv_port_indev_init();       //输入设备初始化
    // lv_port_fs_init();       //文件系统初始化
    lv_port_tick_init();

    xTaskCreate(lv_tick_task, "lv_tick_task", 4096, NULL, 1, NULL);
    xTaskCreate(ui_task, "ui_task", 4096*4, NULL, 3, NULL);
    // xTaskCreate(img_disp_task, "img_disp_task", 4096*2, NULL, 3, NULL);
}



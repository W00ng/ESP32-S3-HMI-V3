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
#include "bsp_sdcard.h"
#include "fs_hal.h"
#include "jpegd2.h"
#include "main.h"


static const char *TAG = "main";

/*Initialize your Storage device and File system.*/
static esp_err_t spiffs_init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS as demo assets storage.");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true   //挂载失败，可以先设置true格式化
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ret;    
}

static bool lcd_write_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data)
{
    esp_lcd_panel_draw_bitmap(lcd_panel, x, y, x + w, y + h,  (uint16_t *)data);

    return true;
}

void app_main(void)
{
    /* Initialize I2C 400KHz */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));
 
    /* Initialize LCD */
    ESP_ERROR_CHECK(bsp_lcd_init());
    lcd_clear_fast(lcd_panel, COLOR_BLACK);

    /* Initialize SPIFFS */
    spiffs_init();

    /* Initialize sdcard */
    esp_err_t ret_val = bsp_sdcard_init();

    ESP_LOGI(TAG, "init done");

    /* malloc a buffer for RGB565 data */
    uint8_t *lcd_buffer = (uint8_t *)heap_caps_malloc((CONFIG_LCD_BUF_WIDTH * CONFIG_LCD_BUF_HIGHT * 2), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(lcd_buffer != NULL);

    uint8_t *jpeg_buf = (uint8_t *)heap_caps_malloc(JPG_IMAGE_MAX_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(jpeg_buf != NULL);

    size_t i = 1;
    char file_name[64] = {0};    
    while (1)
    {
        if(ESP_OK == ret_val) {
            if (i > 20)  i = 1;
            sprintf(file_name, "/sdcard/Photo/img%03d.jpg", i);
        }else{
            if (i > 4)  i = 1;
            sprintf(file_name, "/spiffs/r%03d.jpg", i);          
        }

        FILE *fd = fopen(file_name, "r");

        int read_bytes = fread(jpeg_buf, 1, JPG_IMAGE_MAX_SIZE, fd);
        fclose(fd);

        mjpegdraw(jpeg_buf, read_bytes, lcd_buffer, lcd_write_bitmap);
        ESP_LOGD(TAG, "file_name: %s, fd: %p, read_bytes: %d, free_heap: %d", file_name, fd, read_bytes, esp_get_free_heap_size());

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        i += 1;
    }

    free(jpeg_buf);
}



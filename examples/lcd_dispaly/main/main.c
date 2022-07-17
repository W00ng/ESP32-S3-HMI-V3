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


static const char *TAG = "main";

void app_main(void)
{
    /* Initialize I2C 400KHz */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));

    /* Initialize LCD */
    ESP_ERROR_CHECK(bsp_lcd_init());

    /* LCD touch IC init */
    ESP_ERROR_CHECK(ft5x06_init());

    ESP_LOGI(TAG, "init ok");

    while (1)
    {
        lcd_clear_fast(lcd_panel, COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_clear_fast(lcd_panel, COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_clear_fast(lcd_panel, COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



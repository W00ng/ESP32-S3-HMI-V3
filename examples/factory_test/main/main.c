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
#include "tca9554.h"
#include "bsp_lcd.h"
#include "ft5x06.h"
#include "bat_adc.h"
#include "htu21.h"
#include "mpu6050.h"
#include "key.h"
#include "bsp_sdcard.h"
#include "es8311.h"


static const char *TAG = "main";

void my_task1(void *arg)
{
    uint8_t touch_points;
    uint16_t pos_x, pos_y;
    while (1)
    {
        if(ESP_OK == ft5x06_read_pos(&touch_points, &pos_x, &pos_y))
        {
            ESP_LOGI(TAG, "x=%d y=%d", pos_x, pos_y);            
        }
        vTaskDelay(200/portTICK_PERIOD_MS);        
    }
    vTaskDelete(NULL);
}

void my_task2(void *arg)
{
    while (1)
    {
        adc_get_voltage();
        vTaskDelay(2000/portTICK_PERIOD_MS);  
    }
    vTaskDelete(NULL);
}

void my_task3(void *arg)
{
    uint8_t pin_val = 0;
    // tca9554_set_polarity_inversion(0x0F);
    while (1)
    {
        tca9554_read_input_pins(&pin_val);
        ESP_LOGI(TAG, "P0~P7=0x%02X", pin_val);
        vTaskDelay(2000/portTICK_PERIOD_MS);  
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "silicon revision %d, %dMB %s flash", chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    /* Initialize I2C 400KHz */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));

    /* LCD init */
    ESP_ERROR_CHECK(bsp_lcd_init());
    lcd_clear_fast(lcd_panel, COLOR_BLUE);

    /* Touch IC init */
    ESP_ERROR_CHECK(ft5x06_init());

    /* Init temp and humid sensor */
    ESP_ERROR_CHECK(htu21_init());

    /* Init MEMS sensor */
    ESP_ERROR_CHECK(mpu6050_init());

    /* Init ADC */
    ESP_ERROR_CHECK(adc_init());

    /* Init sdcard */
    if(ESP_OK != bsp_sdcard_init())  ESP_LOGE(TAG, "please insert sdcard");

    /* Init codec */
    ESP_ERROR_CHECK(es8311_codec_config(AUDIO_HAL_44K_SAMPLES));
    es8311_read_chipid();

    /* Init KEY */
    key_init_isr();

    ESP_LOGI(TAG, "初始化完成");

    uint16_t adc_val = 0, voltage;
    float temp, humid;

    temp = htu21_get_temp();
    humid = htu21_get_humid();
    ESP_LOGI(TAG, "temp=%.1f°C", temp);
    ESP_LOGI(TAG, "humid=%.1f%%", humid);

    voltage = adc_get_voltage();
    ESP_LOGI(TAG, "VBAT=%dmV", voltage*2);

    xTaskCreate(my_task1, "my_task1", 4096, NULL, 2, NULL);
    // xTaskCreate(my_task2, "my_task2", 4096, NULL, 3, NULL);
    // xTaskCreate(my_task3, "my_task3", 4096, NULL, 4, NULL);
}


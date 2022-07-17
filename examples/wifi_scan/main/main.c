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
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"

#include "app_prov.h"
#include "ui_prov.h"

static const char *TAG = "main";


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
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
    }

    /* Initialize I2C 400KHz */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));

    /* Init TCA9554 */
    ESP_ERROR_CHECK(tca9554_init());
    ext_io_t io_conf = BSP_EXT_IO_DEFAULT_CONFIG();
    ext_io_t io_level = BSP_EXT_IO_DEFAULT_LEVEL();
    ESP_ERROR_CHECK(tca9554_set_configuration(io_conf.val));
    ESP_ERROR_CHECK(tca9554_write_output_pins(io_level.val));

    /* LVGL init */
    lv_init();                  
    lv_port_disp_init();	    
    lv_port_indev_init();       
    // lv_port_fs_init();       
    lv_port_tick_init();

    /* Initialize provision UI */
    ui_prov_init();

    /* Start provisioning app */
    app_prov_start();

    xTaskCreate(lv_tick_task, "lv_tick_task", 4096, NULL, 1, NULL);
}



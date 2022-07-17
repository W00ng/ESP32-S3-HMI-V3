#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"

#include "tca9554.h"

static const char *TAG = "main";

LV_IMG_DECLARE(pic1);
LV_IMG_DECLARE(pic2);
LV_IMG_DECLARE(pic3);
LV_IMG_DECLARE(shijuan);

void img_disp_task(void *arg)
{
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_obj_align(img, LV_ALIGN_CENTER, 0 , 0);

    while(1)
    {
        lv_img_set_src(img, &pic1);
        vTaskDelay((3000) / portTICK_PERIOD_MS);
        lv_img_set_src(img, &pic2);
        vTaskDelay((3000) / portTICK_PERIOD_MS);
        lv_img_set_src(img, &pic3);
        vTaskDelay((3000) / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void img_disp_task2(void *arg)
{
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0 , 0);

    lv_img_set_src(img, &shijuan);
    while(1)
    {
        vTaskDelay((1000) / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void lv_tick_task(void *arg)
{
    while(1) 
    {
        vTaskDelay((20) / portTICK_PERIOD_MS);
        lv_task_handler();        
    }
}

void app_main()
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
    lv_init();                  
    lv_port_disp_init();	    
    lv_port_indev_init();       
    // lv_port_fs_init();       
    lv_port_tick_init();

    xTaskCreate(lv_tick_task, "lv_tick_task", 4096, NULL, 1, NULL);
    xTaskCreate(img_disp_task, "img_disp_task", 4096 * 2, NULL, 3, NULL);
}

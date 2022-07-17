#pragma once

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "esp_sleep.h"
#include "esp32/ulp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"

#include "lvgl.h"

#include "bat_adc.h"
#include "tca9554.h"
#include "bsp_ext_io.h"
#include "mpu6050.h"
#include "ft5x06.h"


#ifdef __cplusplus
extern "C" {
#endif


extern ext_io_t io_conf;
extern ext_io_t io_level;


/**
 * @brief Initialize power evaluation UI
 * 
 */
void ui_sensor_init(void);

void gpio_config_input(gpio_num_t gpio_num, uint8_t mode);

void gpio_config_output(gpio_num_t gpio_num, uint8_t level);

#ifdef __cplusplus
}
#endif

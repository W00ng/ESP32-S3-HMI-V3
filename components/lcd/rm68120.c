/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rm68120.h"

static const char *TAG = "rm68120";

static esp_err_t panel_rm68120_del(esp_lcd_panel_t *panel);
static esp_err_t panel_rm68120_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_rm68120_init(esp_lcd_panel_t *panel);
static esp_err_t panel_rm68120_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_rm68120_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_rm68120_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_rm68120_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_rm68120_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_rm68120_disp_off(esp_lcd_panel_t *panel, bool off);
static void rm68120_reg_config(esp_lcd_panel_t *panel);


esp_err_t esp_lcd_new_panel_rm68120(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK;
    lcd_panel_t *rm68120 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    rm68120 = calloc(1, sizeof(lcd_panel_t));
    ESP_GOTO_ON_FALSE(rm68120, ESP_ERR_NO_MEM, err, TAG, "no mem for rm68120 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    // scanning direction
#if LCD_DIRECTION_LANDSCAPE   // landscape mode
    rm68120->dir = SCR_DIR_LRTB; rm68120->madctl_val = 0x60;
#else   // portrait mode
    rm68120->dir = SCR_DIR_TBLR; rm68120->madctl_val = 0x00;
#endif

    switch (rm68120->dir){
        case SCR_DIR_LRTB:
            rm68120->width = LCD_WIDTH;
            rm68120->height = LCD_HEIGHT;
            break;
        case SCR_DIR_TBLR:
            rm68120->width = LCD_WIDTH;
            rm68120->height = LCD_HEIGHT;
            break;            
        default: ESP_LOGE(TAG, "undefine, do not use!!!!");
            break;
    }

    switch (panel_dev_config->color_space) {
    case ESP_LCD_COLOR_SPACE_RGB:
        rm68120->madctl_val &= ((~LCD_CMD_BGR_BIT)&0xFF);
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        rm68120->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    //Color Format, REG:3A00H
    switch (panel_dev_config->bits_per_pixel) {
    case 16:
        rm68120->colmod_cal = 0x55;  
        break;
    case 18:
        rm68120->colmod_cal = 0x66;
        break;
    case 24:
        rm68120->colmod_cal = 0x77;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    rm68120->x_gap = 0;
    rm68120->y_gap = 0;
    rm68120->io = io;
    rm68120->bits_per_pixel = panel_dev_config->bits_per_pixel;
    rm68120->reset_gpio_num = panel_dev_config->reset_gpio_num;
    rm68120->reset_level = panel_dev_config->flags.reset_active_high;
    rm68120->base.del = panel_rm68120_del;
    rm68120->base.reset = panel_rm68120_reset;
    rm68120->base.init = panel_rm68120_init;
    rm68120->base.draw_bitmap = panel_rm68120_draw_bitmap;
    rm68120->base.invert_color = panel_rm68120_invert_color;
    rm68120->base.set_gap = panel_rm68120_set_gap;
    rm68120->base.mirror = panel_rm68120_mirror;
    rm68120->base.swap_xy = panel_rm68120_swap_xy;
    rm68120->base.disp_off = panel_rm68120_disp_off;
    *ret_panel = &(rm68120->base);
    ESP_LOGD(TAG, "new rm68120 panel @%p", rm68120);

    return ESP_OK;

err:
    if (rm68120) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(rm68120);
    }
    return ret;
}

static esp_err_t panel_rm68120_del(esp_lcd_panel_t *panel)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);

    if (rm68120->reset_gpio_num >= 0) {
        gpio_reset_pin(rm68120->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del rm68120 panel @%p", rm68120);
    free(rm68120);
    return ESP_OK;
}

static esp_err_t panel_rm68120_reset(esp_lcd_panel_t *panel)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;

    // perform hardware reset
    if (rm68120->reset_gpio_num >= 0) {
        gpio_set_level(rm68120->reset_gpio_num, rm68120->reset_level);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(rm68120->reset_gpio_num, !rm68120->reset_level);
        vTaskDelay(pdMS_TO_TICKS(20));
    } else {
        // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET << 8, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5m before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_rm68120_init(esp_lcd_panel_t *panel)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;

    // register config
    rm68120_reg_config(panel);    
    // SLEEP OUT 
    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT << 8, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    // scanning direction
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL << 8, (uint16_t[]) {
        rm68120->madctl_val,
    }, 2);
    // Color Format
    esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD << 8, (uint16_t[]) {
        rm68120->colmod_cal,
    }, 2);
    // turn on display
    esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON << 8, NULL, 0);

#if 1
ESP_LOGI(TAG, "LCD=%dx%d dir=%d xgap=%d ygap=%d", rm68120->width, rm68120->height, rm68120->dir, rm68120->x_gap, rm68120->y_gap);
ESP_LOGI(TAG, "madctl=0x%02X colmod=0x%02X", rm68120->madctl_val, rm68120->colmod_cal);
#endif
    return ESP_OK;
}

/* * (x_start-x_end)*(y_start-y_end)= MAX(32640) * */
static esp_err_t panel_rm68120_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    assert((x_start <= x_end) && (y_start <= y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = rm68120->io;

    x_start += rm68120->x_gap;
    x_end += rm68120->x_gap;
    y_start += rm68120->y_gap;
    y_end += rm68120->y_gap;

    // define an area of frame memory where MCU can access
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_CASET << 8) + 0, (uint16_t[]) {
        (x_start >> 8) & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_CASET << 8) + 1, (uint16_t[]) {
        x_start & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_CASET << 8) + 2, (uint16_t[]) {
        ((x_end - 1) >> 8) & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_CASET << 8) + 3, (uint16_t[]) {
        (x_end - 1) & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_RASET << 8) + 0, (uint16_t[]) {
        (y_start >> 8) & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_RASET << 8) + 1, (uint16_t[]) {
        y_start & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_RASET << 8) + 2, (uint16_t[]) {
        ((y_end - 1) >> 8) & 0xFF,
    }, 2);
    esp_lcd_panel_io_tx_param(io, (LCD_CMD_RASET << 8) + 3, (uint16_t[]) {
        (y_end - 1) & 0xFF,
    }, 2);
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * rm68120->bits_per_pixel / 8;
    // ESP_LOGI(TAG, "transfer size = %d", len);
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR << 8, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_rm68120_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command << 8, NULL, 0);
    return ESP_OK;
}

static esp_err_t panel_rm68120_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;
    if (mirror_x) {
        rm68120->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        rm68120->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        rm68120->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        rm68120->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL << 8, (uint16_t[]) {
        rm68120->madctl_val
    }, 2);
    return ESP_OK;
}

static esp_err_t panel_rm68120_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;
    if (swap_axes) {
        rm68120->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        rm68120->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL << 8, (uint16_t[]) {
        rm68120->madctl_val
    }, 2);
    return ESP_OK;
}

static esp_err_t panel_rm68120_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    rm68120->x_gap = x_gap;
    rm68120->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_rm68120_disp_off(esp_lcd_panel_t *panel, bool off)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;
    int command = 0;
    if (off) {
        command = LCD_CMD_DISPOFF;
    } else {
        command = LCD_CMD_DISPON;
    }
    esp_lcd_panel_io_tx_param(io, command << 8, NULL, 0);
    return ESP_OK;
}

static void rm68120_reg_config(esp_lcd_panel_t *panel)
{
    lcd_panel_t *rm68120 = __containerof(panel, lcd_panel_t, base);
    esp_lcd_panel_io_handle_t io = rm68120->io;

    //ENABLE PAGE 1
    esp_lcd_panel_io_tx_param(io, 0xF000, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF001, (uint16_t[]) {0xAA,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF002, (uint16_t[]) {0x52,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF003, (uint16_t[]) {0x08,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF004, (uint16_t[]) {0x01,}, 2);

    //GAMMA SETING  RED
    esp_lcd_panel_io_tx_param(io, 0xD100, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD101, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD102, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD103, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD104, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD105, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD106, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD107, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD108, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD109, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD10F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD110, (uint16_t[]) {0x68,}, 2);
    
    esp_lcd_panel_io_tx_param(io, 0xD111, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD112, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD113, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD114, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD115, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD116, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD117, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD118, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD119, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD11F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD120, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD121, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD122, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD123, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD124, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD125, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD126, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD127, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD128, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD129, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12B, (uint16_t[]) {0x4A,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD12F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD130, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD131, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD132, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD133, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD134, (uint16_t[]) {0xff,}, 2);

    //GAMMA SETING GREEN
    esp_lcd_panel_io_tx_param(io, 0xD200, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD201, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD202, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD203, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD204, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD205, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD206, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD207, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD208, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD209, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD20F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD210, (uint16_t[]) {0x68,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD211, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD212, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD213, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD214, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD215, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD216, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD217, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD218, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD219, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD21F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD220, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD221, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD222, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD223, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD224, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD225, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD226, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD227, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD228, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD229, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22B, (uint16_t[]) {0x4a,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD22F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD230, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD231, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD232, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD233, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD234, (uint16_t[]) {0xff,}, 2);
    
   //GAMMA SETING BLUE
    esp_lcd_panel_io_tx_param(io, 0xD300, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD301, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD302, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD303, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD304, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD305, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD306, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD307, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD308, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD309, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD30F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD310, (uint16_t[]) {0x68,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD311, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD312, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD313, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD314, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD315, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD316, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD317, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD318, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD319, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD31F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD320, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD321, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD322, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD323, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD324, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD325, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD326, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD327, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD328, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD329, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32B, (uint16_t[]) {0x4A,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD32F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD330, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD331, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD332, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD333, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD334, (uint16_t[]) {0xff,}, 2);

    //GAMMA SETING  RED
    esp_lcd_panel_io_tx_param(io, 0xD400, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD401, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD402, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD403, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD404, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD405, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD406, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD407, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD408, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD409, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD40F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD410, (uint16_t[]) {0x68,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD411, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD412, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD413, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD414, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD415, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD416, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD417, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD418, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD419, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD41F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD420, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD421, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD422, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD423, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD424, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD425, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD426, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD427, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD428, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD429, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42B, (uint16_t[]) {0x4A,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD42F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD430, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD431, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD432, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD433, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD434, (uint16_t[]) {0xff,}, 2);
    
    //GAMMA SETING GREEN
    esp_lcd_panel_io_tx_param(io, 0xD500, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD501, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD502, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD503, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD504, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD505, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD506, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD507, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD508, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD509, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD50F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD510, (uint16_t[]) {0x68,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD511, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD512, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD513, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD514, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD515, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD516, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD517, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD518, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD519, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD51F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD520, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD521, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD522, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD523, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD524, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD525, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD526, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD527, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD528, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD529, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52B, (uint16_t[]) {0x4a,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD52F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD530, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD531, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD532, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD533, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD534, (uint16_t[]) {0xff,}, 2);
    
    //GAMMA SETING BLUE
    esp_lcd_panel_io_tx_param(io, 0xD600, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD601, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD602, (uint16_t[]) {0x1b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD603, (uint16_t[]) {0x44,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD604, (uint16_t[]) {0x62,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD605, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD606, (uint16_t[]) {0x7b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD607, (uint16_t[]) {0xa1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD608, (uint16_t[]) {0xc0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD609, (uint16_t[]) {0xee,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60A, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60B, (uint16_t[]) {0x10,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60C, (uint16_t[]) {0x2c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60D, (uint16_t[]) {0x43,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60E, (uint16_t[]) {0x57,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD60F, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD610, (uint16_t[]) {0x68,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD611, (uint16_t[]) {0x78,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD612, (uint16_t[]) {0x87,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD613, (uint16_t[]) {0x94,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD614, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD615, (uint16_t[]) {0xa0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD616, (uint16_t[]) {0xac,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD617, (uint16_t[]) {0xb6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD618, (uint16_t[]) {0xc1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD619, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61A, (uint16_t[]) {0xcb,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61B, (uint16_t[]) {0xcd,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61C, (uint16_t[]) {0xd6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61D, (uint16_t[]) {0xdf,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61E, (uint16_t[]) {0x95,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD61F, (uint16_t[]) {0xe8,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD620, (uint16_t[]) {0xf1,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD621, (uint16_t[]) {0xfa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD622, (uint16_t[]) {0x02,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD623, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD624, (uint16_t[]) {0x0b,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD625, (uint16_t[]) {0x13,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD626, (uint16_t[]) {0x1d,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD627, (uint16_t[]) {0x26,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD628, (uint16_t[]) {0xaa,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD629, (uint16_t[]) {0x30,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62A, (uint16_t[]) {0x3c,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62B, (uint16_t[]) {0x4A,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62C, (uint16_t[]) {0x63,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62D, (uint16_t[]) {0xea,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62E, (uint16_t[]) {0x79,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD62F, (uint16_t[]) {0xa6,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD630, (uint16_t[]) {0xd0,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD631, (uint16_t[]) {0x20,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD632, (uint16_t[]) {0x0f,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD633, (uint16_t[]) {0x8e,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xD634, (uint16_t[]) {0xff,}, 2);
    
    //AVDD VOLTAGE SETTING
    esp_lcd_panel_io_tx_param(io, 0xB000, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB001, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB002, (uint16_t[]) {0x05,}, 2);
    //AVEE VOLTAGE SETTING
    esp_lcd_panel_io_tx_param(io, 0xB100, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB101, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB102, (uint16_t[]) {0x05,}, 2);
    //AVDD Boosting
    esp_lcd_panel_io_tx_param(io, 0xB600, (uint16_t[]) {0x34,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB601, (uint16_t[]) {0x34,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB603, (uint16_t[]) {0x34,}, 2);
    //AVEE Boosting
    esp_lcd_panel_io_tx_param(io, 0xB700, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB701, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB702, (uint16_t[]) {0x24,}, 2);
    //VCL Boosting
    esp_lcd_panel_io_tx_param(io, 0xB800, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB801, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB802, (uint16_t[]) {0x24,}, 2);
    //VGLX VOLTAGE SETTING
    esp_lcd_panel_io_tx_param(io, 0xBA00, (uint16_t[]) {0x14,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBA01, (uint16_t[]) {0x14,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBA02, (uint16_t[]) {0x14,}, 2);
    //VCL Boosting
    esp_lcd_panel_io_tx_param(io, 0xB900, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB901, (uint16_t[]) {0x24,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xB902, (uint16_t[]) {0x24,}, 2);
    //Gamma Voltage
    esp_lcd_panel_io_tx_param(io, 0xBc00, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBc01, (uint16_t[]) {0xa0,}, 2);//vgmp=5.0
    esp_lcd_panel_io_tx_param(io, 0xBc02, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBd00, (uint16_t[]) {0x00,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBd01, (uint16_t[]) {0xa0,}, 2);//vgmn=5.0
    esp_lcd_panel_io_tx_param(io, 0xBd02, (uint16_t[]) {0x00,}, 2);
    //VCOM Setting
    esp_lcd_panel_io_tx_param(io, 0xBe01, (uint16_t[]) {0x3d,}, 2);//3
    //ENABLE PAGE 0
    esp_lcd_panel_io_tx_param(io, 0xF000, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF001, (uint16_t[]) {0xAA,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF002, (uint16_t[]) {0x52,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF003, (uint16_t[]) {0x08,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF004, (uint16_t[]) {0x00,}, 2);
    //Vivid Color Function Control
    esp_lcd_panel_io_tx_param(io, 0xB400, (uint16_t[]) {0x10,}, 2);
    //Z-INVERSION
    esp_lcd_panel_io_tx_param(io, 0xBC00, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBC01, (uint16_t[]) {0x05,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xBC02, (uint16_t[]) {0x05,}, 2);

    //*************** add on 20111021**********************//
    esp_lcd_panel_io_tx_param(io, 0xB700, (uint16_t[]) {0x22,}, 2);//GATE EQ CONTROL
    esp_lcd_panel_io_tx_param(io, 0xB701, (uint16_t[]) {0x22,}, 2);//GATE EQ CONTROL
    esp_lcd_panel_io_tx_param(io, 0xC80B, (uint16_t[]) {0x2A,}, 2);//DISPLAY TIMING CONTROL
    esp_lcd_panel_io_tx_param(io, 0xC80C, (uint16_t[]) {0x2A,}, 2);//DISPLAY TIMING CONTROL
    esp_lcd_panel_io_tx_param(io, 0xC80F, (uint16_t[]) {0x2A,}, 2);//DISPLAY TIMING CONTROL
    esp_lcd_panel_io_tx_param(io, 0xC810, (uint16_t[]) {0x2A,}, 2);//DISPLAY TIMING CONTROL
    //*************** add on 20111021**********************//
    //PWM_ENH_OE =1
    esp_lcd_panel_io_tx_param(io, 0xd000, (uint16_t[]) {0x01,}, 2);
    //DM_SEL =1
    esp_lcd_panel_io_tx_param(io, 0xb300, (uint16_t[]) {0x10,}, 2);
    //VBPDA=07h
    esp_lcd_panel_io_tx_param(io, 0xBd02, (uint16_t[]) {0x07,}, 2);
    //VBPDb=07h
    esp_lcd_panel_io_tx_param(io, 0xBe02, (uint16_t[]) {0x07,}, 2);
    //VBPDc=07h
    esp_lcd_panel_io_tx_param(io, 0xBf02, (uint16_t[]) {0x07,}, 2);
    //ENABLE PAGE 2
    esp_lcd_panel_io_tx_param(io, 0xF000, (uint16_t[]) {0x55,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF001, (uint16_t[]) {0xAA,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF002, (uint16_t[]) {0x52,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF003, (uint16_t[]) {0x08,}, 2);
    esp_lcd_panel_io_tx_param(io, 0xF004, (uint16_t[]) {0x02,}, 2);
    //SDREG0 =0
    esp_lcd_panel_io_tx_param(io, 0xc301, (uint16_t[]) {0xa9,}, 2);
    //DS=14
    esp_lcd_panel_io_tx_param(io, 0xfe01, (uint16_t[]) {0x94,}, 2);
    //OSC =60h
    esp_lcd_panel_io_tx_param(io, 0xf600, (uint16_t[]) {0x60,}, 2);
    //TE ON
    esp_lcd_panel_io_tx_param(io, 0x3500, (uint16_t[]) {0x00,}, 2);

    // //SLEEP OUT 
    // esp_lcd_panel_io_tx_param(io, 0x1100, NULL, 0);
    // vTaskDelay(100 / portTICK_RATE_MS);

    // //DISPLY ON
    // esp_lcd_panel_io_tx_param(io, 0x2900, NULL, 0);
    // vTaskDelay(100 / portTICK_RATE_MS);

    // esp_lcd_panel_io_tx_param(io, 0x3A00, (uint16_t[]) {0x55,}, 2);
    // esp_lcd_panel_io_tx_param(io, 0x3600, (uint16_t[]) {0xA3,}, 2);
}


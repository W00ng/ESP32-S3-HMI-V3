
#pragma once

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/************************* IMPORTANT1 ***************************
 * CONFIG_LCD_BUF_WIDTH must be equal to the actual image width  */
#define CONFIG_LCD_BUF_WIDTH 800   //image with
#define CONFIG_LCD_BUF_HIGHT 40    //max = 32640 / with


/************************* IMPORTANT2 ***************************
 * the actual image size must be smaller than JPG_IMAGE_MAX_SIZE */
#define JPG_IMAGE_MAX_SIZE   (450 * 1024)   


#ifdef __cplusplus
}
#endif




/*
 * ws2812.h
 *
 *  Created on: 27-Oct-2026
 *      Author: Varalakshmi
 */


#ifndef INC_WS2812_H
#define INC_WS2812_H

#include "stm32f4xx_hal.h"

#define WS2812_LED_COUNT    15U

HAL_StatusTypeDef WS2812_Init(TIM_HandleTypeDef *htim, DMA_HandleTypeDef *hdma, GPIO_TypeDef *gpio_port, uint16_t gpio_pin);

void WS2812_SetPixel(uint32_t led, uint8_t red, uint8_t green, uint8_t blue);

HAL_StatusTypeDef WS2812_Show(void);

uint8_t WS2812_IsBusy(void);

#endif /* INC_WS2812_H */

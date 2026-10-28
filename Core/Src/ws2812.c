/*
 * ws2812.c
 *
 *  Created on: 27-Oct-2026
 *      Author: Varalakshmi
 */

#include "ws2812.h"

#define WS2812_BITS_PER_LED       24U			//  3 * 8
#define WS2812_SLOTS_PER_BIT       3U
#define WS2812_DMA_LENGTH (WS2812_LED_COUNT * WS2812_BITS_PER_LED * WS2812_SLOTS_PER_BIT)

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} WS2812_Pixel;

static TIM_HandleTypeDef *ws2812_timer;
static DMA_HandleTypeDef *ws2812_dma;
static GPIO_TypeDef *ws2812_gpio_port;
static uint16_t ws2812_gpio_pin;

static WS2812_Pixel ws2812_pixels[WS2812_LED_COUNT];
static uint32_t ws2812_dma_buffer[WS2812_DMA_LENGTH];

static volatile uint8_t ws2812_busy;
static uint8_t ws2812_initialized;
static uint32_t ws2812_reset_started_ms;

static uint32_t WS2812_SetMask(void)
{
	return (uint32_t)ws2812_gpio_pin;
}

static uint32_t WS2812_ResetMask(void)
{
	return (uint32_t)ws2812_gpio_pin << 16U;
}

static void WS2812_EncodeByte(uint8_t value, uint32_t *index)
{
	for (int32_t bit = 7; bit >= 0; bit--) {
		// Every WS2812 bit starts high.
		ws2812_dma_buffer[(*index)++] = WS2812_SetMask();

		// if bit is 0, set low, else same (high)
		if ((value & (uint8_t)(1U << bit)) != 0U) {
			ws2812_dma_buffer[(*index)++] = 0U;
		} else {
			ws2812_dma_buffer[(*index)++] = WS2812_ResetMask();
		}

		// Both bit values finish low.
		ws2812_dma_buffer[(*index)++] = WS2812_ResetMask();
	}
}

static void WS2812_BuildBuffer(void)
{
	uint32_t index = 0U;

	for (uint32_t led = 0U; led < WS2812_LED_COUNT; led++) {
		// WS2812 data order is green, red, blue.
		WS2812_EncodeByte(ws2812_pixels[led].green, &index);
		WS2812_EncodeByte(ws2812_pixels[led].red, &index);
		WS2812_EncodeByte(ws2812_pixels[led].blue, &index);
	}
}

static void WS2812_StopTransfer(void)
{
	__HAL_TIM_DISABLE_DMA(ws2812_timer, TIM_DMA_UPDATE);
	__HAL_TIM_DISABLE(ws2812_timer);
	ws2812_gpio_port->BSRR = WS2812_ResetMask();
	ws2812_reset_started_ms = HAL_GetTick();
	ws2812_busy = 0U;
}

static void WS2812_DMA_Complete(DMA_HandleTypeDef *hdma)
{
	if (hdma == ws2812_dma) {
		WS2812_StopTransfer();
	}
}

static void WS2812_DMA_Error(DMA_HandleTypeDef *hdma)
{
	if (hdma == ws2812_dma) {
		WS2812_StopTransfer();
	}
}

HAL_StatusTypeDef WS2812_Init(TIM_HandleTypeDef *htim, DMA_HandleTypeDef *hdma, GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	if ((htim == NULL) || (hdma == NULL) || (gpio_port == NULL) || (gpio_pin == 0U)) {
		return HAL_ERROR;
	}

	ws2812_timer = htim;
	ws2812_dma = hdma;
	ws2812_gpio_port = gpio_port;
	ws2812_gpio_pin = gpio_pin;
	ws2812_busy = 0U;
	ws2812_initialized = 1U;
	ws2812_reset_started_ms = HAL_GetTick() - 1U;

	for (uint32_t led = 0U; led < WS2812_LED_COUNT; led++) {
		WS2812_SetPixel(led, 0U, 0U, 0U);
	}

	ws2812_gpio_port->BSRR = WS2812_ResetMask();
	return HAL_OK;
}

void WS2812_SetPixel(uint32_t led, uint8_t red, uint8_t green, uint8_t blue)
{
	if (led >= WS2812_LED_COUNT) {
		return;
	}

	ws2812_pixels[led].red = red;
	ws2812_pixels[led].green = green;
	ws2812_pixels[led].blue = blue;
}

HAL_StatusTypeDef WS2812_Show(void)
{
	HAL_StatusTypeDef status;

	if (ws2812_initialized == 0U) {
		return HAL_ERROR;
	}

	if (WS2812_IsBusy() != 0U) {
		return HAL_BUSY;
	}

	ws2812_busy = 1U;
	WS2812_BuildBuffer();

	ws2812_gpio_port->BSRR = WS2812_ResetMask();
	__HAL_TIM_DISABLE(ws2812_timer);
	__HAL_TIM_SET_COUNTER(ws2812_timer, 0U);
	__HAL_TIM_CLEAR_FLAG(ws2812_timer, TIM_FLAG_UPDATE);

	ws2812_dma->XferCpltCallback = WS2812_DMA_Complete;
	ws2812_dma->XferErrorCallback = WS2812_DMA_Error;

	status = HAL_DMA_Start_IT(ws2812_dma,
			(uint32_t)(uintptr_t)ws2812_dma_buffer,
			(uint32_t)(uintptr_t)&ws2812_gpio_port->BSRR,
			WS2812_DMA_LENGTH);

	if (status != HAL_OK) {
		ws2812_busy = 0U;
		return status;
	}

	__HAL_TIM_ENABLE_DMA(ws2812_timer, TIM_DMA_UPDATE);
	__HAL_TIM_ENABLE(ws2812_timer);

	return HAL_OK;
}

uint8_t WS2812_IsBusy(void)
{
	if (ws2812_busy != 0U) {
		return 1U;
	}

	// Keep the line low for at least 1 ms between frames.
	if ((ws2812_initialized != 0U) && ((uint32_t)(HAL_GetTick() - ws2812_reset_started_ms) < 1U)) {
		return 1U;
	}

	return 0U;
}

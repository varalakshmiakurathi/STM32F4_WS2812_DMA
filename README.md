# STM32F4 WS2812 DMA Driver

A simple WS2812 LED driver for the STM32F446 using TIM1-triggered DMA writes
to the GPIO BSRR register. The example drives 15 LEDs and continuously changes
the complete strip between red, green, and blue.

## Features

- 800 kHz WS2812 data signal
- Non-blocking DMA transmission
- GRB color transmission order
- DMA completion and error handling
- 1 ms reset interval between frames
- No CPU bit-banging while a frame is being transmitted

## Hardware used

- STM32F446ZE microcontroller
- WS2812 or WS2812B LED strip with 15 LEDs
- External 5 V LED power supply
- PA3 as the LED data output

### Basic connections

| STM32 / supply | WS2812 strip |
| --- | --- |
| PA3 | DIN |
| External 5 V | 5 V |
| STM32 and supply ground | GND |

The STM32 and LED supply must share a common ground. A 5 V-compatible logic
level shifter is recommended between the 3.3 V STM32 output and the LED data
input.

## Peripheral configuration

The included STM32CubeIDE project uses:

- System clock: 180 MHz
- TIM1 prescaler: `0`
- TIM1 period: `74`
- Timer update frequency: 2.4 MHz
- DMA2 Stream 5, Channel 6
- DMA direction: memory to peripheral
- DMA mode: normal
- DMA memory increment: enabled
- DMA data width: word
- GPIO output: PA3, push-pull, very-high speed

Each WS2812 bit uses three timer slots. At 2.4 MHz, one slot is approximately
416.7 ns and one complete WS2812 bit is 1.25 us.

## Usage

```c
WS2812_Init(&htim1, &hdma_tim1_up,
            WS_2812DATA_GPIO_Port, WS_2812DATA_Pin);

WS2812_SetPixel(0, 255, 0, 0);
WS2812_Show();
```

`WS2812_SetPixel()` changes the stored color, and `WS2812_Show()` transmits the
updated colors to the complete LED strip.

## Current scope

This version supports one 24-bit GRB strip with a compile-time LED count. It
does not currently include brightness control, gamma correction, or animation
effects beyond the RGB example in `main.c`.

# STM32F407 Bare-Metal Peripheral Drivers

Register-level (no HAL/LL) peripheral drivers for the STM32F407VGT6,
written in C, developed while learning embedded systems on the STM32F4.

## Peripherals implemented

| Peripheral | Clock control | Init | Read/Write | Interrupts |
|---|---|---|---|---|
| GPIO  | ✅ | ✅ | ✅ | 🔜 (EXTI RTSR/FTSR/IMR wired up in `GPIO_Init`, SYSCFG EXTICR line-select not yet done) |
| SPI   | ✅ | ✅ | ✅ (blocking) | 🔜 |
| I2C   | ✅ | ✅ | ✅ (blocking master TX/RX) | 🔜 |
| USART | ✅ | ✅ | ✅ (blocking) | 🔜 |

## Folder structure

```
drivers/
  inc/   stm32f407xx.h                    <- base register/struct defs
         stm32f407xx_gpio_driver.h
         stm32f407xx_spi_driver.h
         stm32f407xx_i2c_driver.h
         stm32f407xx_usart_driver.h
  src/   stm32f407xx_gpio_driver.c
         stm32f407xx_spi_driver.c
         stm32f407xx_i2c_driver.c
         stm32f407xx_usart_driver.c
```

## Usage

```c
#include "stm32f407xx.h"

GPIO_Handle_t led = {0};
led.pGPIOx = GPIOD;
led.GPIO_Config.GPIO_PinNumber   = GPIO_PIN_NO_12;
led.GPIO_Config.GPIO_PinMode     = GPIO_MODE_OUT;
led.GPIO_Config.GPIO_OutputType  = GPIO_OP_TYPE_PP;
led.GPIO_Config.GPIO_OutputSpeed = GPIO_SPEED_FAST;
led.GPIO_Config.GPIO_PinPUPD     = GPIO_NO_PUPD;

GPIO_PeripheralClockControl(GPIOD, ENABLE);
GPIO_Init(&led);
GPIO_ToggleOutputPin(GPIOD, GPIO_PIN_NO_12);
```

## Notes on this repo's history

These drivers were written and debugged incrementally (GPIO → SPI → I2C →
USART), catching bugs like:
- missing "clear field before OR-ing in" on multi-bit register fields
- `while(len >= 0)` on an unsigned length (infinite loop / underflow)
- read/write direction mixed up in receive functions
- wrong register (SR vs CR1) used for flag-clear vs config bits
- `stm32f407xx.h` (base register map + struct layout) was written after
  the four peripheral drivers, to make everything here actually compile —
  the original files assumed it existed already but it wasn't part of
  the working set they came from.

Tested by compilation/logic review only — not yet flashed to hardware.
Treat as a work-in-progress driver library, not a production HAL.

## TODO

- GPIO EXTI line selection via SYSCFG->EXTICR
- I2C TRISE register configuration
- Interrupt-driven (non-blocking) SPI/I2C/USART APIs
- PLL clock source support in the PCLK helpers (currently HSI/HSE only)

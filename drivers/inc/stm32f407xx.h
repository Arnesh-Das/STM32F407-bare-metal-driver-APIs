/*
 * stm32f407xx.h
 *
 * Base device header: peripheral register layouts, base addresses,
 * and generic macros used by every peripheral driver in this repo.
 *
 * NOTE: This file was NOT part of the original upload. The four driver
 * files referenced macros/structs (GPIOA, RCC, ENABLE, SET, etc.) as
 * "predefined" but the base header itself was never shared, so this is
 * a from-scratch, standard CMSIS-style base header for the STM32F407,
 * written to make the corrected drivers in this repo actually compile.
 * If you already have your own stm32f407xx.h from a course/project,
 * use that instead and just make sure the struct field names below
 * match what the driver .c files expect.
 */

#ifndef INC_STM32F407XX_H_
#define INC_STM32F407XX_H_

#include <stdint.h>

#define __vo volatile

/* ---------------------- Generic Macros ---------------------- */
#define ENABLE          1
#define DISABLE         0
#define SET             ENABLE
#define RESET           DISABLE
#define GPIO_PIN_SET    SET
#define GPIO_PIN_RESET  RESET
#define FLAG_SET        SET
#define FLAG_RESET      RESET

/* ---------------------- Base addresses ----------------------- */
#define FLASH_BASEADDR         0x08000000U
#define SRAM1_BASEADDR         0x20000000U

#define PERIPH_BASEADDR        0x40000000U
#define APB1PERIPH_BASEADDR    PERIPH_BASEADDR
#define APB2PERIPH_BASEADDR    0x40010000U
#define AHB1PERIPH_BASEADDR    0x40020000U
#define AHB2PERIPH_BASEADDR    0x50000000U

#define GPIOA_BASEADDR         (AHB1PERIPH_BASEADDR + 0x0000)
#define GPIOB_BASEADDR         (AHB1PERIPH_BASEADDR + 0x0400)
#define GPIOC_BASEADDR         (AHB1PERIPH_BASEADDR + 0x0800)
#define GPIOD_BASEADDR         (AHB1PERIPH_BASEADDR + 0x0C00)
#define GPIOE_BASEADDR         (AHB1PERIPH_BASEADDR + 0x1000)
#define GPIOF_BASEADDR         (AHB1PERIPH_BASEADDR + 0x1400)
#define GPIOG_BASEADDR         (AHB1PERIPH_BASEADDR + 0x1800)
#define GPIOH_BASEADDR         (AHB1PERIPH_BASEADDR + 0x1C00)
#define GPIOI_BASEADDR         (AHB1PERIPH_BASEADDR + 0x2000)

#define RCC_BASEADDR           (AHB1PERIPH_BASEADDR + 0x3800)

#define I2C1_BASEADDR          (APB1PERIPH_BASEADDR + 0x5400)
#define I2C2_BASEADDR          (APB1PERIPH_BASEADDR + 0x5800)
#define I2C3_BASEADDR          (APB1PERIPH_BASEADDR + 0x5C00)

#define SPI2_BASEADDR          (APB1PERIPH_BASEADDR + 0x3800)
#define SPI3_BASEADDR          (APB1PERIPH_BASEADDR + 0x3C00)
#define USART2_BASEADDR        (APB1PERIPH_BASEADDR + 0x4400)
#define USART3_BASEADDR        (APB1PERIPH_BASEADDR + 0x4800)
#define UART4_BASEADDR         (APB1PERIPH_BASEADDR + 0x4C00)
#define UART5_BASEADDR         (APB1PERIPH_BASEADDR + 0x5000)

#define SPI1_BASEADDR          (APB2PERIPH_BASEADDR + 0x3000)
#define USART1_BASEADDR        (APB2PERIPH_BASEADDR + 0x1000)
#define USART6_BASEADDR        (APB2PERIPH_BASEADDR + 0x1400)
#define EXTI_BASEADDR          (APB2PERIPH_BASEADDR + 0x3C00)
#define SYSCFG_BASEADDR        (APB2PERIPH_BASEADDR + 0x3800)

/* ---------------------- Peripheral register structs ---------------------- */
typedef struct
{
    __vo uint32_t MODER;
    __vo uint32_t OTYPER;
    __vo uint32_t OSPEEDR;
    __vo uint32_t PUPDR;
    __vo uint32_t IDR;
    __vo uint32_t ODR;
    __vo uint32_t BSRR;
    __vo uint32_t LCKR;
    __vo uint32_t AFRL;   /* AFR[0] */
    __vo uint32_t AFRH;   /* AFR[1] */
} GPIO_RegDef_t;

typedef struct
{
    __vo uint32_t CR;
    __vo uint32_t PLLCFGR;
    __vo uint32_t CFGR;
    __vo uint32_t CIR;
    __vo uint32_t AHB1RSTR;
    __vo uint32_t AHB2RSTR;
    __vo uint32_t AHB3RSTR;
    uint32_t      RESERVED0;
    __vo uint32_t APB1RSTR;
    __vo uint32_t APB2RSTR;
    uint32_t      RESERVED1[2];
    __vo uint32_t AHB1ENR;
    __vo uint32_t AHB2ENR;
    __vo uint32_t AHB3ENR;
    uint32_t      RESERVED2;
    __vo uint32_t APB1ENR;
    __vo uint32_t APB2ENR;
    uint32_t      RESERVED3[2];
    __vo uint32_t AHB1LPENR;
    __vo uint32_t AHB2LPENR;
    __vo uint32_t AHB3LPENR;
    uint32_t      RESERVED4;
    __vo uint32_t APB1LPENR;
    __vo uint32_t APB2LPENR;
    uint32_t      RESERVED5[2];
    __vo uint32_t BDCR;
    __vo uint32_t CSR;
    uint32_t      RESERVED6[2];
    __vo uint32_t SSCGR;
    __vo uint32_t PLLI2SCFGR;
    __vo uint32_t PLLSAICFGR;
    __vo uint32_t DCKCFGR;
} RCC_RegDef_t;

/* Convenience alias used by the drivers when they read the system-clock
   source / prescaler bits out of RCC->CFGR. */
#define RCC_CLKCFGR CFGR

typedef struct
{
    __vo uint32_t IMR;
    __vo uint32_t EMR;
    __vo uint32_t RTSR;
    __vo uint32_t FTSR;
    __vo uint32_t SWIER;
    __vo uint32_t PR;
} EXTI_RegDef_t;

typedef struct
{
    __vo uint32_t MEMRMP;
    __vo uint32_t PMC;
    __vo uint32_t EXTICR[4];
    uint32_t      RESERVED[2];
    __vo uint32_t CMPCR;
} SYSCFG_RegDef_t;

typedef struct
{
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t SR;
    __vo uint32_t DR;
    __vo uint32_t CRCPR;
    __vo uint32_t RXCRCR;
    __vo uint32_t TXCRCR;
    __vo uint32_t I2SCFGR;
    __vo uint32_t I2SPR;
} SPI_RegDef_t;

typedef struct
{
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t OAR1;
    __vo uint32_t OAR2;
    __vo uint32_t DR;
    __vo uint32_t SR1;
    __vo uint32_t SR2;
    __vo uint32_t CCR;
    __vo uint32_t TRISE;
    __vo uint32_t FLTR;
} I2C_RegDef_t;

typedef struct
{
    __vo uint32_t SR;
    __vo uint32_t DR;
    __vo uint32_t BRR;
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t CR3;
    __vo uint32_t GTPR;
} USART_RegDef_t;

/* ---------------------- Peripheral pointer defs ---------------------- */
#define GPIOA   ((GPIO_RegDef_t*)GPIOA_BASEADDR)
#define GPIOB   ((GPIO_RegDef_t*)GPIOB_BASEADDR)
#define GPIOC   ((GPIO_RegDef_t*)GPIOC_BASEADDR)
#define GPIOD   ((GPIO_RegDef_t*)GPIOD_BASEADDR)
#define GPIOE   ((GPIO_RegDef_t*)GPIOE_BASEADDR)
#define GPIOF   ((GPIO_RegDef_t*)GPIOF_BASEADDR)
#define GPIOG   ((GPIO_RegDef_t*)GPIOG_BASEADDR)
#define GPIOH   ((GPIO_RegDef_t*)GPIOH_BASEADDR)
#define GPIOI   ((GPIO_RegDef_t*)GPIOI_BASEADDR)

#define RCC     ((RCC_RegDef_t*)RCC_BASEADDR)
#define EXTI    ((EXTI_RegDef_t*)EXTI_BASEADDR)
#define SYSCFG  ((SYSCFG_RegDef_t*)SYSCFG_BASEADDR)

#define SPI1    ((SPI_RegDef_t*)SPI1_BASEADDR)
#define SPI2    ((SPI_RegDef_t*)SPI2_BASEADDR)
#define SPI3    ((SPI_RegDef_t*)SPI3_BASEADDR)

#define I2C1    ((I2C_RegDef_t*)I2C1_BASEADDR)
#define I2C2    ((I2C_RegDef_t*)I2C2_BASEADDR)
#define I2C3    ((I2C_RegDef_t*)I2C3_BASEADDR)

#define USART1  ((USART_RegDef_t*)USART1_BASEADDR)
#define USART2  ((USART_RegDef_t*)USART2_BASEADDR)
#define USART3  ((USART_RegDef_t*)USART3_BASEADDR)
#define UART4   ((USART_RegDef_t*)UART4_BASEADDR)
#define UART5   ((USART_RegDef_t*)UART5_BASEADDR)
#define USART6  ((USART_RegDef_t*)USART6_BASEADDR)

#include "stm32f407xx_gpio_driver.h"
#include "stm32f407xx_spi_driver.h"
#include "stm32f407xx_i2c_driver.h"
#include "stm32f407xx_usart_driver.h"

#endif /* INC_STM32F407XX_H_ */

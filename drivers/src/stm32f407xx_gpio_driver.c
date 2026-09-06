/*
 * stm32f407xx_gpio_driver.c
 *
 * Corrected version of the original gpio_driver.h. Fixes applied vs. the
 * uploaded file (logic bugs only — see README for the full list):
 *  - GPIO_Init: MODER/OTYPER/OSPEEDR/PUPDR/AFR fields are now cleared
 *    before being OR'd in (was OR-only, which could mix stale reset-state
 *    bits with the new value).
 *  - AFRH shift now correctly uses (PinNumber - 8); AFRH covers pins 8-15.
 *  - GPIO_ReadFromInputPin now right-shifts + masks a single bit instead
 *    of left-shifting the whole IDR register.
 *  - GPIO_WriteToOutputPin now takes an explicit PinNumber parameter
 *    (the original referenced an undeclared "pinnumber" variable).
 *  - GPIO_WriteToOutputPort assigns (=) instead of OR-ing (|=), since a
 *    full port write must also clear bits, not just set them.
 *  - GPIO_setorreset/DeInit now pulses the AHB1RSTR bit (set then clear)
 *    instead of leaving it permanently set, which would hold the
 *    peripheral in permanent reset.
 */

#include "stm32f407xx.h"

void GPIO_PeripheralClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pGPIOx == GPIOA)      RCC->AHB1ENR |= (1 << 0);
        else if (pGPIOx == GPIOB) RCC->AHB1ENR |= (1 << 1);
        else if (pGPIOx == GPIOC) RCC->AHB1ENR |= (1 << 2);
        else if (pGPIOx == GPIOD) RCC->AHB1ENR |= (1 << 3);
        else if (pGPIOx == GPIOE) RCC->AHB1ENR |= (1 << 4);
        else if (pGPIOx == GPIOF) RCC->AHB1ENR |= (1 << 5);
        else if (pGPIOx == GPIOG) RCC->AHB1ENR |= (1 << 6);
        else if (pGPIOx == GPIOH) RCC->AHB1ENR |= (1 << 7);
        else if (pGPIOx == GPIOI) RCC->AHB1ENR |= (1 << 8);
    }
    else
    {
        if (pGPIOx == GPIOA)      RCC->AHB1ENR &= ~(1 << 0);
        else if (pGPIOx == GPIOB) RCC->AHB1ENR &= ~(1 << 1);
        else if (pGPIOx == GPIOC) RCC->AHB1ENR &= ~(1 << 2);
        else if (pGPIOx == GPIOD) RCC->AHB1ENR &= ~(1 << 3);
        else if (pGPIOx == GPIOE) RCC->AHB1ENR &= ~(1 << 4);
        else if (pGPIOx == GPIOF) RCC->AHB1ENR &= ~(1 << 5);
        else if (pGPIOx == GPIOG) RCC->AHB1ENR &= ~(1 << 6);
        else if (pGPIOx == GPIOH) RCC->AHB1ENR &= ~(1 << 7);
        else if (pGPIOx == GPIOI) RCC->AHB1ENR &= ~(1 << 8);
    }
}

/* Peripheral reset: a reset line must be pulsed (set then clear),
 * never left permanently set — otherwise the peripheral stays in reset. */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
    uint8_t bitpos = 0xFF;

    if (pGPIOx == GPIOA)      bitpos = 0;
    else if (pGPIOx == GPIOB) bitpos = 1;
    else if (pGPIOx == GPIOC) bitpos = 2;
    else if (pGPIOx == GPIOD) bitpos = 3;
    else if (pGPIOx == GPIOE) bitpos = 4;
    else if (pGPIOx == GPIOF) bitpos = 5;
    else if (pGPIOx == GPIOG) bitpos = 6;
    else if (pGPIOx == GPIOH) bitpos = 7;
    else if (pGPIOx == GPIOI) bitpos = 8;

    if (bitpos != 0xFF)
    {
        RCC->AHB1RSTR |= (1 << bitpos);
        RCC->AHB1RSTR &= ~(1 << bitpos);
    }
}

void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    uint32_t temp = 0;
    uint8_t  pin  = pGPIOHandle->GPIO_Config.GPIO_PinNumber;

    /* 1. Mode */
    if (pGPIOHandle->GPIO_Config.GPIO_PinMode <= GPIO_MODE_ANALOG)
    {
        temp = pGPIOHandle->GPIO_Config.GPIO_PinMode << (2 * pin);
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 << (2 * pin)); /* clear field first */
        pGPIOHandle->pGPIOx->MODER |= temp;
    }
    else
    {
        /* Interrupt modes: configured via EXTI, MODER stays in input mode (00) */
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 << (2 * pin));

        if (pGPIOHandle->GPIO_Config.GPIO_PinMode == GPIO_MODE_RT)
        {
            EXTI->RTSR |= (1 << pin);
            EXTI->FTSR &= ~(1 << pin);
        }
        else if (pGPIOHandle->GPIO_Config.GPIO_PinMode == GPIO_MODE_FT)
        {
            EXTI->FTSR |= (1 << pin);
            EXTI->RTSR &= ~(1 << pin);
        }
        else
        {
            EXTI->RTSR |= (1 << pin);
            EXTI->FTSR |= (1 << pin);
        }

        /* NOTE: SYSCFG_EXTICR port-selection bits also need setting here
         * (which GPIO port owns this EXTI line) — not implemented, same
         * as the original file's "//configure SYSCFGR register:" TODO. */

        EXTI->IMR |= (1 << pin);
    }

    /* 2. Output type */
    temp = pGPIOHandle->GPIO_Config.GPIO_OutputType << pin;
    pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 << pin);
    pGPIOHandle->pGPIOx->OTYPER |= temp;

    /* 3. Output speed */
    temp = pGPIOHandle->GPIO_Config.GPIO_OutputSpeed << (2 * pin);
    pGPIOHandle->pGPIOx->OSPEEDR &= ~(0x3 << (2 * pin));
    pGPIOHandle->pGPIOx->OSPEEDR |= temp;

    /* 4. Pull-up/pull-down */
    temp = pGPIOHandle->GPIO_Config.GPIO_PinPUPD << (2 * pin);
    pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 << (2 * pin));
    pGPIOHandle->pGPIOx->PUPDR |= temp;

    /* 5. Alternate function */
    if (pGPIOHandle->GPIO_Config.GPIO_PinMode == GPIO_MODE_ALTFN)
    {
        if (pin <= GPIO_PIN_NO_7)
        {
            temp = pGPIOHandle->GPIO_Config.GPIO_AlternateFun << (4 * pin);
            pGPIOHandle->pGPIOx->AFRL &= ~(0xF << (4 * pin));
            pGPIOHandle->pGPIOx->AFRL |= temp;
        }
        else
        {
            temp = pGPIOHandle->GPIO_Config.GPIO_AlternateFun << (4 * (pin - 8));
            pGPIOHandle->pGPIOx->AFRH &= ~(0xF << (4 * (pin - 8)));
            pGPIOHandle->pGPIOx->AFRH |= temp;
        }
    }
}

uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    return (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x00000001);
}

uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx)
{
    return (uint16_t)pGPIOx->IDR;
}

void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value)
{
    if (Value == GPIO_PIN_SET)
        pGPIOx->ODR |= (1 << PinNumber);
    else
        pGPIOx->ODR &= ~(1 << PinNumber);
}

void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value)
{
    pGPIOx->ODR = Value; /* full-port write must overwrite, not OR */
}

void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    pGPIOx->ODR ^= (1 << PinNumber);
}

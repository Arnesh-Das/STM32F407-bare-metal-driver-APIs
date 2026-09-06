/*
 * stm32f407xx_spi_driver.c
 *
 * Corrected version of spi_driver.h. Key fixes vs. the upload (see README
 * for the full list from the chat review):
 *  - SPI_GetFlagStatus signature now takes SPI_RegDef_t*, matching how it
 *    was actually being called (pSPIHandle->pSPIx) — the original
 *    declared it as taking SPI_Handle_t*, a type mismatch.
 *  - SPI_Init: SSM/DFF branches were comparing the wrong config fields
 *    (SPI_CPHA and SPI_mode instead of SPI_SSM and SPI_DFF) — fixed.
 *  - Baud rate field is now cleared before OR-ing in (was OR-only).
 *  - SPI_SendData/SPI_ReceiveData: loop condition changed from
 *    `len >= 0` (always true for unsigned len, causes underflow/infinite
 *    loop) to `len > 0`.
 *  - SPI_ReceiveData now reads DR into the buffer (was writing pointer
 *    values into DR — backwards for a "receive" function).
 *  - Flag-status waits now use the peripheral pointer passed into the
 *    function instead of being hardcoded to SPI1 / a wrong argument.
 *  - 8-bit DR writes/reads use plain assignment instead of |=, since DR
 *    is 8-bit valid and stale bits should not be OR'd in.
 */

#include "stm32f407xx.h"

void SPI_PeripheralClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pSPIx == SPI1)      RCC->APB2ENR |= (1 << 12);
        else if (pSPIx == SPI2) RCC->APB1ENR |= (1 << 14);
        else if (pSPIx == SPI3) RCC->APB1ENR |= (1 << 15);
    }
    else
    {
        if (pSPIx == SPI1)      RCC->APB2ENR &= ~(1 << 12);
        else if (pSPIx == SPI2) RCC->APB1ENR &= ~(1 << 14);
        else if (pSPIx == SPI3) RCC->APB1ENR &= ~(1 << 15);
    }
}

void SPI_Init(SPI_Handle_t *pSPIHandle)
{
    uint32_t temp = 0;
    SPI_RegDef_t *pSPIx = pSPIHandle->pSPIx;

    /* 1. Device mode */
    if (pSPIHandle->SPI_Config.SPI_mode == SPI_MASTER)
        pSPIx->CR1 |= (1 << 2);
    else
        pSPIx->CR1 &= ~(1 << 2);

    /* 2. Bus mode */
    if (pSPIHandle->SPI_Config.SPI_BUS_mode == SPI_FD)
    {
        pSPIx->CR1 &= ~(1 << 15);
    }
    else if (pSPIHandle->SPI_Config.SPI_BUS_mode == SPI_HD)
    {
        pSPIx->CR1 |= (1 << 15);
    }
    else if (pSPIHandle->SPI_Config.SPI_BUS_mode == SPI_SIMPLEX_RX_ONLY)
    {
        pSPIx->CR1 |= (1 << 15);
        pSPIx->CR1 |= (1 << 10);
    }

    /* 3. Data frame format — was comparing SPI_mode instead of SPI_DFF */
    if (pSPIHandle->SPI_Config.SPI_DFF == SPI_DFF_8)
        pSPIx->CR1 &= ~(1 << 11);
    else if (pSPIHandle->SPI_Config.SPI_DFF == SPI_DFF_16)
        pSPIx->CR1 |= (1 << 11);

    /* 4. CPOL */
    if (pSPIHandle->SPI_Config.SPI_CPOL == SPI_CPOL_LOW)
        pSPIx->CR1 &= ~(1 << 1);
    else
        pSPIx->CR1 |= (1 << 1);

    /* 5. CPHA */
    if (pSPIHandle->SPI_Config.SPI_CPHA == SPI_CPHA_LOW)
        pSPIx->CR1 &= ~(1 << 0);
    else
        pSPIx->CR1 |= (1 << 0);

    /* 6. Baud rate (BR[2:0], bits 3-5) — clear field first */
    temp = (pSPIHandle->SPI_Config.SPI_Sclk_Speed & 0x7) << 3;
    pSPIx->CR1 &= ~(0x7 << 3);
    pSPIx->CR1 |= temp;

    /* 7. SSM — was comparing SPI_CPHA instead of SPI_SSM */
    if (pSPIHandle->SPI_Config.SPI_SSM == SPI_SSM_DIS)
    {
        pSPIx->CR1 &= ~(1 << 9);
    }
    else if (pSPIHandle->SPI_Config.SPI_SSM == SPI_SSM_EN)
    {
        pSPIx->CR1 |= (1 << 9);
        pSPIx->CR1 |= (1 << 8); /* SSI */
    }
}

void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
    if (pSPIx == SPI1)      { RCC->APB2RSTR |= (1 << 12); RCC->APB2RSTR &= ~(1 << 12); }
    else if (pSPIx == SPI2) { RCC->APB1RSTR |= (1 << 14); RCC->APB1RSTR &= ~(1 << 14); }
    else if (pSPIx == SPI3) { RCC->APB1RSTR |= (1 << 15); RCC->APB1RSTR &= ~(1 << 15); }
}

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
    return (pSPIx->SR & FlagName) ? FLAG_SET : FLAG_RESET;
}

void SPI_Enable(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
        pSPIx->CR1 |= (1 << 6);
    else
        pSPIx->CR1 &= ~(1 << 6);
}

void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
        pSPIx->CR1 |= (1 << 8);
    else
        pSPIx->CR1 &= ~(1 << 8);
}

void SPI_SSOEControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
    /* SSOE is CR2 bit 2, not CR1 */
    if (EnorDi == ENABLE)
        pSPIx->CR2 |= (1 << 2);
    else
        pSPIx->CR2 &= ~(1 << 2);
}

void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t len)
{
    while (len > 0)
    {
        while (SPI_GetFlagStatus(pSPIx, SPI_FLAG_TXE) == FLAG_RESET);

        if (pSPIx->CR1 & (1 << 11))
        {
            /* 16-bit frame */
            pSPIx->DR = *((uint16_t *)pTxBuffer);
            pTxBuffer += 2;
            len -= 2;
        }
        else
        {
            /* 8-bit frame */
            pSPIx->DR = *pTxBuffer;
            pTxBuffer++;
            len--;
        }
    }
}

void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t len)
{
    while (len > 0)
    {
        while (SPI_GetFlagStatus(pSPIx, SPI_FLAG_RXNE) == FLAG_RESET);

        if (pSPIx->CR1 & (1 << 11))
        {
            /* 16-bit frame */
            *((uint16_t *)pRxBuffer) = pSPIx->DR;
            pRxBuffer += 2;
            len -= 2;
        }
        else
        {
            /* 8-bit frame */
            *pRxBuffer = (uint8_t)pSPIx->DR;
            pRxBuffer++;
            len--;
        }
    }
}

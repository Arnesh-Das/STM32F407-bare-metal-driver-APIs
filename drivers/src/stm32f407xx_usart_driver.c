/*
 * stm32f407xx_usart_driver.c
 *
 * Corrected version of USART_driver.h. Key fixes vs. the upload:
 *  - `tempreg` was used without ever being declared.
 *  - Sampling-mode check compared `tempreg` (a bit-shifted, not-yet-
 *    meaningful value) against USART_SAMPLING_8 instead of checking the
 *    config field directly.
 *  - Parity block used `tempreg != (1<<10)` (a no-op comparison) instead
 *    of `tempreg |= (1<<10)`, and used the same bit for both "parity
 *    enabled" and "even/odd selection" — PCE is bit 10, PS is bit 9;
 *    these are now set independently.
 *  - Several spots did `tempreg |= pUSARTx->CRn` (reading the register
 *    into tempreg) where the clear intent was to write tempreg *into*
 *    CRn — the original never actually wrote CR1/CR2/CR3 or BRR at all.
 *  - Stop-bits code referenced `pI2CHandle` (copy-paste from the I2C
 *    file) instead of `pUSARTHandle`.
 *  - The PCLK helper function was defined *inside* USART_Init(), which
 *    is not legal C — pulled out as its own static function, same
 *    uint8_t-truncates-a-Hz-value and missing-shift bugs fixed as in
 *    the I2C driver's copy of this helper.
 *  - Baud rate calculation: original scaled by 10000 and masked raw
 *    bits with no rounding into the 4-bit fractional field, and never
 *    wrote the result to BRR at all. Replaced with the standard
 *    mantissa/fraction calculation that's actually written to BRR.
 *  - Send/Receive: `while(len >= 0)` unsigned-underflow bug (same class
 *    as the SPI/I2C files) fixed to `len > 0`.
 *  - Send: DR writes changed from `|=` to `=`, masked to 9 bits when in
 *    9-bit mode.
 *  - Receive: function took `uint8_t pRxBuffer` (by value) instead of
 *    `uint8_t *pRxBuffer` — couldn't have written to the caller's buffer
 *    at all. Also fixed `pTxBuffer++` (wrong buffer, copy-paste from
 *    Send) to `pRxBuffer++`, and the reversed cast
 *    `(uint16_t)*pRxBuffer = ...` to `*(uint16_t*)pRxBuffer = ...`.
 *  - The "wait for IDLE, then clear it" logic was structured as
 *    `while(!IDLE){ clear-flag code }`, which only runs the clear code
 *    *before* IDLE is set, and skips it entirely once IDLE is actually
 *    set. Changed to an empty wait followed by the clear code.
 */

#include "stm32f407xx.h"

static uint32_t USART_GetPCLKValue(USART_RegDef_t *pUSARTx)
{
    uint32_t systemclock = 0;
    uint8_t  clksrc = (RCC->CFGR >> 2) & 0x3;

    if (clksrc == 0)      systemclock = 16000000U; /* HSI */
    else if (clksrc == 1) systemclock = 8000000U;  /* HSE (board-dependent) */
    else                  systemclock = 16000000U; /* PLL — placeholder */

    uint16_t AHBPrescArr[8]  = {2, 4, 8, 16, 32, 64, 128, 256};
    uint8_t  APBPrescArr[4]  = {2, 4, 8, 16};

    uint32_t ahbTemp = (RCC->CFGR >> 4) & 0xF;
    uint32_t ahbPresc = (ahbTemp < 8) ? 1 : AHBPrescArr[ahbTemp - 8];

    uint32_t apbBitpos = (pUSARTx == USART1 || pUSARTx == USART6) ? 13 : 10; /* PPRE2 vs PPRE1 */
    uint32_t apbTemp = (RCC->CFGR >> apbBitpos) & 0x7;
    uint32_t apbPresc = (apbTemp < 4) ? 1 : APBPrescArr[apbTemp - 4];

    return (systemclock / ahbPresc) / apbPresc;
}

void USART_PeripheralClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pUSARTx == USART1)      RCC->APB2ENR |= (1 << 4);
        else if (pUSARTx == USART2) RCC->APB1ENR |= (1 << 17);
        else if (pUSARTx == USART3) RCC->APB1ENR |= (1 << 18);
        else if (pUSARTx == UART4)  RCC->APB1ENR |= (1 << 19);
        else if (pUSARTx == UART5)  RCC->APB1ENR |= (1 << 20);
        else if (pUSARTx == USART6) RCC->APB2ENR |= (1 << 5);
    }
    else
    {
        if (pUSARTx == USART1)      RCC->APB2ENR &= ~(1 << 4);
        else if (pUSARTx == USART2) RCC->APB1ENR &= ~(1 << 17);
        else if (pUSARTx == USART3) RCC->APB1ENR &= ~(1 << 18);
        else if (pUSARTx == UART4)  RCC->APB1ENR &= ~(1 << 19);
        else if (pUSARTx == UART5)  RCC->APB1ENR &= ~(1 << 20);
        else if (pUSARTx == USART6) RCC->APB2ENR &= ~(1 << 5);
    }
}

void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
        pUSARTx->CR1 |= (1 << 13);
    else
        pUSARTx->CR1 &= ~(1 << 13);
}

uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName)
{
    return (pUSARTx->SR & FlagName) ? FLAG_SET : FLAG_RESET;
}

static void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{
    uint32_t pclk = USART_GetPCLKValue(pUSARTx);
    uint32_t usartdiv;
    uint8_t  over8 = (pUSARTx->CR1 & (1 << 15)) ? 1 : 0;

    if (over8)
        usartdiv = (25 * pclk) / (2 * BaudRate);
    else
        usartdiv = (25 * pclk) / (4 * BaudRate);

    uint32_t mantissa = usartdiv / 100;
    uint32_t fracRaw  = usartdiv - (mantissa * 100);
    uint32_t fraction;

    if (over8)
        fraction = ((fracRaw * 8) + 50) / 100 & 0x7;
    else
        fraction = ((fracRaw * 16) + 50) / 100 & 0xF;

    pUSARTx->BRR = (mantissa << 4) | fraction;
}

void USART_Init(USART_Handle_t *pUSARTHandle)
{
    USART_RegDef_t *pUSARTx = pUSARTHandle->pUSARTx;
    uint32_t cr1 = 0, cr2 = 0, cr3 = 0;

    /* Sampling mode (CR1 bit 15) */
    if (pUSARTHandle->USART_Config.USART_SamplingMode == USART_SAMPLING_8)
        cr1 |= (1 << 15);

    /* Word length (CR1 bit 12) */
    if (pUSARTHandle->USART_Config.USART_WordLength == USART_WORD_LEN_9)
        cr1 |= (1 << 12);

    /* Parity: PCE = bit 10, PS = bit 9 (0 = even, 1 = odd) */
    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
    {
        cr1 |= (1 << 10);
    }
    else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD)
    {
        cr1 |= (1 << 10);
        cr1 |= (1 << 9);
    }

    pUSARTx->CR1 = cr1;

    /* Stop bits (CR2 bits 12-13) */
    cr2 = (pUSARTHandle->USART_Config.USART_NoOfStopBits & 0x3) << 12;
    pUSARTx->CR2 = cr2;

    /* HW flow control (CR3 bits 8-9) */
    if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
        cr3 |= (1 << 9);
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
        cr3 |= (1 << 8);
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
        cr3 |= (1 << 8) | (1 << 9);

    pUSARTx->CR3 = cr3;

    /* Baud rate */
    USART_SetBaudRate(pUSARTx, pUSARTHandle->USART_Config.USART_Baud);
}

void USART_DeInit(USART_RegDef_t *pUSARTx)
{
    if (pUSARTx == USART1)      { RCC->APB2RSTR |= (1 << 4);  RCC->APB2RSTR &= ~(1 << 4); }
    else if (pUSARTx == USART2) { RCC->APB1RSTR |= (1 << 17); RCC->APB1RSTR &= ~(1 << 17); }
    else if (pUSARTx == USART3) { RCC->APB1RSTR |= (1 << 18); RCC->APB1RSTR &= ~(1 << 18); }
    else if (pUSARTx == UART4)  { RCC->APB1RSTR |= (1 << 19); RCC->APB1RSTR &= ~(1 << 19); }
    else if (pUSARTx == UART5)  { RCC->APB1RSTR |= (1 << 20); RCC->APB1RSTR &= ~(1 << 20); }
    else if (pUSARTx == USART6) { RCC->APB2RSTR |= (1 << 5);  RCC->APB2RSTR &= ~(1 << 5); }
}

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t len)
{
    USART_RegDef_t *pUSARTx = pUSARTHandle->pUSARTx;
    uint8_t is9bit = (pUSARTHandle->USART_Config.USART_WordLength == USART_WORD_LEN_9);

    pUSARTx->CR1 |= (1 << 13); /* UE */
    pUSARTx->CR1 |= (1 << 3);  /* TE */

    while (len > 0)
    {
        while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == FLAG_RESET);

        if (is9bit)
        {
            pUSARTx->DR = (*(uint16_t *)pTxBuffer) & 0x01FF;
            pTxBuffer += 2;
            len -= 2;
        }
        else
        {
            pUSARTx->DR = (*pTxBuffer & 0xFF);
            pTxBuffer++;
            len--;
        }
    }

    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == FLAG_RESET);
    pUSARTx->SR &= ~(1 << 6); /* clear TC */
}

void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t len)
{
    USART_RegDef_t *pUSARTx = pUSARTHandle->pUSARTx;
    uint8_t is9bit = (pUSARTHandle->USART_Config.USART_WordLength == USART_WORD_LEN_9);
    uint32_t dummyread;

    pUSARTx->CR1 |= (1 << 13); /* UE */
    pUSARTx->CR1 |= (1 << 2);  /* RE */

    while (len > 0)
    {
        while (USART_GetFlagStatus(pUSARTx, USART_FLAG_RXNE) == FLAG_RESET);

        if (is9bit)
        {
            *(uint16_t *)pRxBuffer = pUSARTx->DR & 0x01FF;
            pRxBuffer += 2;
            len -= 2;
        }
        else
        {
            *pRxBuffer = (uint8_t)(pUSARTx->DR & 0xFF);
            pRxBuffer++;
            len--;
        }
    }

    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_IDLE) == FLAG_RESET);
    dummyread = pUSARTx->SR;
    dummyread = pUSARTx->DR;
    (void)dummyread;
}

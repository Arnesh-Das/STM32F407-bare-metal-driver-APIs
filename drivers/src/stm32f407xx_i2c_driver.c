/*
 * stm32f407xx_i2c_driver.c
 *
 * Corrected version of I2C_driver.h. Key fixes vs. the upload:
 *  - Missing closing brace after the clock-control ENABLE block (the
 *    original function never compiled — the `else` was outside the
 *    function body).
 *  - RCC_GetPCLK1Value(): systemclock/fpclk/AHBPresc/APBPresc were all
 *    declared uint8_t, which truncates a ~16-80 MHz clock value to
 *    garbage. Changed to uint32_t.
 *  - AHB/APB prescaler bit-fields were masked but never shifted down
 *    (e.g. `& 0xF0` without `>> 4`), so the array index was wrong by
 *    orders of magnitude. Fixed with proper shifts.
 *  - AHBPrescArr was declared with 8 slots but initialized with 9
 *    values, and referenced under two different (inconsistent) names.
 *  - CCR calculation was mixing `fpclk` (peripheral clock) with
 *    `systemclock` (core clock) in the same formula — now uses fpclk
 *    consistently, per the SCL_Speed the user actually configured
 *    (not a hardcoded 1 MHz assumption).
 *  - Duplicate/near-duplicate SPI_Get_Flag_Status leftover (wrong type,
 *    wrong struct) removed — I2C_GetFlagStatus is the single helper.
 *  - I2C_MasterSendData / I2C_MasterReceiveData: the two functions had
 *    the same name in the original file (both called I2C_Send_Data).
 *  - ADDR-clear / SB-clear "wait" loops were structured as
 *    `while(!flag){ clear-code }`, which only runs the clear code while
 *    the flag is NOT yet set (i.e. never actually clears it once it
 *    becomes set). Changed to an empty wait followed by the clear code.
 *  - ACK control was being written to SR1 (a status/clear register) in
 *    two places instead of CR1, which is where the ACK bit actually
 *    lives.
 *  - Address byte: `slaveaddress <<!` (invalid) corrected to
 *    `slaveaddress <<= 1`.
 *  - Multi-byte receive now waits on RXNE before reading DR inside the
 *    loop (missing in the original), and the len==1 special case now
 *    returns instead of falling through into the general loop as well.
 */

#include "stm32f407xx.h"

void I2C_PeripheralClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
    {
        if (pI2Cx == I2C1)      RCC->APB1ENR |= (1 << 21);
        else if (pI2Cx == I2C2) RCC->APB1ENR |= (1 << 22);
        else if (pI2Cx == I2C3) RCC->APB1ENR |= (1 << 23);
    }
    else
    {
        if (pI2Cx == I2C1)      RCC->APB1ENR &= ~(1 << 21);
        else if (pI2Cx == I2C2) RCC->APB1ENR &= ~(1 << 22);
        else if (pI2Cx == I2C3) RCC->APB1ENR &= ~(1 << 23);
    }
}

void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
    if (EnorDi == ENABLE)
        pI2Cx->CR1 |= (1 << 0);
    else
        pI2Cx->CR1 &= ~(1 << 0);
}

void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
    if (pI2Cx == I2C1)      { RCC->APB1RSTR |= (1 << 21); RCC->APB1RSTR &= ~(1 << 21); }
    else if (pI2Cx == I2C2) { RCC->APB1RSTR |= (1 << 22); RCC->APB1RSTR &= ~(1 << 22); }
    else if (pI2Cx == I2C3) { RCC->APB1RSTR |= (1 << 23); RCC->APB1RSTR &= ~(1 << 23); }
}

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
    return (pI2Cx->SR1 & FlagName) ? FLAG_SET : FLAG_RESET;
}

/* Returns the APB1 peripheral clock frequency in Hz. All intermediate
 * values are uint32_t (Hz-scale numbers do not fit in uint8_t). */
uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t systemclock = 0;
    uint8_t  clksrc = (RCC->CFGR >> 2) & 0x3;

    if (clksrc == 0)      systemclock = 16000000U; /* HSI */
    else if (clksrc == 1) systemclock = 8000000U;  /* HSE (board-dependent; adjust as needed) */
    else                  systemclock = 16000000U; /* PLL — not derived here, placeholder */

    uint16_t AHBPrescArr[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint8_t  APB1PrescArr[4] = {2, 4, 8, 16};

    uint32_t ahbTemp = (RCC->CFGR >> 4) & 0xF;   /* HPRE[3:0] */
    uint32_t ahbPresc = (ahbTemp < 8) ? 1 : AHBPrescArr[ahbTemp - 8];

    uint32_t apbTemp = (RCC->CFGR >> 10) & 0x7;  /* PPRE1[2:0] */
    uint32_t apb1Presc = (apbTemp < 4) ? 1 : APB1PrescArr[apbTemp - 4];

    return (systemclock / ahbPresc) / apb1Presc;
}

void I2C_Init(I2C_Handle_t *pI2CHandle)
{
    I2C_RegDef_t *pI2Cx = pI2CHandle->pI2Cx;
    uint32_t pclk1 = RCC_GetPCLK1Value();

    /* 1. ACK control (CR1 bit 10) */
    if (pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
        pI2Cx->CR1 |= (1 << 10);
    else
        pI2Cx->CR1 &= ~(1 << 10);

    /* 2. FREQ field of CR2 (MHz value of pclk1) */
    pI2Cx->CR2 &= ~0x3F;
    pI2Cx->CR2 |= (pclk1 / 1000000U) & 0x3F;

    /* 3. Own address, 7-bit mode. Bit 14 must be kept set per reference manual. */
    pI2Cx->OAR1 &= ~(1 << 15);
    pI2Cx->OAR1 |= (1 << 14);
    pI2Cx->OAR1 |= (pI2CHandle->I2C_Config.I2C_DeviceAddress << 1);

    /* 4. CCR: speed + duty cycle, all derived from pclk1 (never systemclock) */
    uint16_t ccr = 0;
    if (pI2CHandle->I2C_Config.SCL_Speed <= I2C_SCL_SPEED_SM)
    {
        pI2Cx->CCR &= ~(1 << 15); /* standard mode */
        ccr = (uint16_t)(pclk1 / (2 * pI2CHandle->I2C_Config.SCL_Speed));
    }
    else
    {
        pI2Cx->CCR |= (1 << 15); /* fast mode */
        if (pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2)
        {
            pI2Cx->CCR &= ~(1 << 14);
            ccr = (uint16_t)(pclk1 / (3 * pI2CHandle->I2C_Config.SCL_Speed));
        }
        else
        {
            pI2Cx->CCR |= (1 << 14);
            ccr = (uint16_t)(pclk1 / (25 * pI2CHandle->I2C_Config.SCL_Speed));
        }
    }
    pI2Cx->CCR &= ~0xFFF;
    pI2Cx->CCR |= (ccr & 0xFFF);

    /* 5. TRISE — not implemented (matches original file's TODO). Typical
     *    calculation: standard mode TRISE = (pclk1/1e6)+1;
     *    fast mode TRISE = (pclk1/1e6 * 300/1000)+1. Add before use. */
}

static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t slaveaddress)
{
    slaveaddress <<= 1;
    slaveaddress &= ~1; /* R/W = 0 (write) */
    pI2Cx->DR = slaveaddress;
}

static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t slaveaddress)
{
    slaveaddress <<= 1;
    slaveaddress |= 1; /* R/W = 1 (read) */
    pI2Cx->DR = slaveaddress;
}

static void I2C_ClearAddrFlag(I2C_RegDef_t *pI2Cx)
{
    uint32_t dummyread;
    dummyread = pI2Cx->SR1;
    dummyread = pI2Cx->SR2;
    (void)dummyread;
}

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t slaveaddress, uint32_t len)
{
    I2C_RegDef_t *pI2Cx = pI2CHandle->pI2Cx;

    pI2Cx->CR1 |= (1 << 8); /* generate START */
    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_SB));

    I2C_ExecuteAddressPhaseWrite(pI2Cx, slaveaddress);
    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_ADDR));
    I2C_ClearAddrFlag(pI2Cx);

    while (len > 0)
    {
        while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_TXE));
        pI2Cx->DR = *pTxBuffer;
        pTxBuffer++;
        len--;
    }

    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_TXE));
    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_BTF));

    pI2Cx->CR1 |= (1 << 9); /* generate STOP */
}

void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t slaveaddress, uint32_t len)
{
    I2C_RegDef_t *pI2Cx = pI2CHandle->pI2Cx;

    pI2Cx->CR1 |= (1 << 8); /* generate START */
    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_SB));

    I2C_ExecuteAddressPhaseRead(pI2Cx, slaveaddress);
    while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_ADDR));

    if (len == 1)
    {
        pI2Cx->CR1 &= ~(1 << 10);   /* disable ACK (CR1, not SR1) */
        I2C_ClearAddrFlag(pI2Cx);
        pI2Cx->CR1 |= (1 << 9);     /* generate STOP */
        while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_RXNE));
        *pRxBuffer = pI2Cx->DR;
        return; /* don't fall through into the multi-byte loop below */
    }

    I2C_ClearAddrFlag(pI2Cx);

    for (uint32_t i = len; i > 0; i--)
    {
        while (!I2C_GetFlagStatus(pI2Cx, I2C_FLAG_RXNE));

        if (i == 2) /* second-to-last byte: stop NACK/ACK before final read */
        {
            pI2Cx->CR1 &= ~(1 << 10); /* disable ACK (CR1, not SR1) */
            pI2Cx->CR1 |= (1 << 9);   /* generate STOP */
        }

        *pRxBuffer = pI2Cx->DR;
        pRxBuffer++;
    }
}

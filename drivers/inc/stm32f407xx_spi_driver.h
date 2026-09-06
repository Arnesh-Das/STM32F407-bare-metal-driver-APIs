#ifndef INC_STM32F407XX_SPI_DRIVER_H_
#define INC_STM32F407XX_SPI_DRIVER_H_

#include <stdint.h>

typedef struct SPI_RegDef_t SPI_RegDef_t;

typedef struct
{
    uint8_t SPI_mode;
    uint8_t SPI_BUS_mode;
    uint8_t SPI_Sclk_Speed;
    uint8_t SPI_DFF;
    uint8_t SPI_CPOL;
    uint8_t SPI_CPHA;
    uint8_t SPI_SSM;
} SPI_Config_t;

typedef struct
{
    SPI_RegDef_t *pSPIx;
    SPI_Config_t SPI_Config;
} SPI_Handle_t;

#define SPI_MASTER  1
#define SPI_SLAVE   0

#define SPI_FD               0
#define SPI_HD               1
#define SPI_SIMPLEX_RX_ONLY  2

#define SPI_DFF_8   0
#define SPI_DFF_16  1

#define SPI_CPOL_LOW   0
#define SPI_CPOL_HIGH  1

#define SPI_CPHA_LOW   0
#define SPI_CPHA_HIGH  1

#define SPI_SSM_DIS  0
#define SPI_SSM_EN   1

/* SR flag bit masks */
#define SPI_FLAG_RXNE  (1 << 0)
#define SPI_FLAG_TXE   (1 << 1)
#define SPI_FLAG_BSY   (1 << 7)
#define TXE   SPI_FLAG_TXE
#define RXNE  SPI_FLAG_RXNE

void SPI_PeripheralClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi);
void SPI_Init(SPI_Handle_t *pSPIHandle);
void SPI_DeInit(SPI_RegDef_t *pSPIx);

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName);
void SPI_Enable(SPI_RegDef_t *pSPIx, uint8_t EnorDi);
void SPI_SSOEControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi);
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi);

void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t len);
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t len);

#endif /* INC_STM32F407XX_SPI_DRIVER_H_ */

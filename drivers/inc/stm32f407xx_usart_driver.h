#ifndef INC_STM32F407XX_USART_DRIVER_H_
#define INC_STM32F407XX_USART_DRIVER_H_

#include <stdint.h>

typedef struct USART_RegDef_t USART_RegDef_t;

typedef struct
{
    uint8_t  USART_Mode;
    uint32_t USART_Baud;
    uint8_t  USART_NoOfStopBits;
    uint8_t  USART_WordLength;
    uint8_t  USART_ParityControl;
    uint8_t  USART_HWFlowControl;
    uint8_t  USART_SamplingMode;
} USART_Config_t;

typedef struct
{
    USART_RegDef_t *pUSARTx;
    USART_Config_t USART_Config;
} USART_Handle_t;

#define USART_SAMPLING_16  0
#define USART_SAMPLING_8   1

#define USART_WORD_LEN_8  0
#define USART_WORD_LEN_9  1

#define USART_PARITY_DISABLE  0
#define USART_PARITY_EN_EVEN  1
#define USART_PARITY_EN_ODD   2

#define USART_STOP_BITS_1    0
#define USART_STOP_BITS_0_5  1
#define USART_STOP_BITS_2    2
#define USART_STOP_BITS_1_5  3

#define USART_HW_FLOW_CTRL_NONE     0
#define USART_HW_FLOW_CTRL_CTS      1
#define USART_HW_FLOW_CTRL_RTS      2
#define USART_HW_FLOW_CTRL_CTS_RTS  3

#define USART_FLAG_TXE   (1 << 7)
#define USART_FLAG_RXNE  (1 << 5)
#define USART_FLAG_TC    (1 << 6)
#define USART_FLAG_IDLE  (1 << 4)
#define TXE   USART_FLAG_TXE
#define RXNE  USART_FLAG_RXNE
#define TC    USART_FLAG_TC
#define IDLE  USART_FLAG_IDLE

void USART_PeripheralClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi);
void USART_Init(USART_Handle_t *pUSARTHandle);
void USART_DeInit(USART_RegDef_t *pUSARTx);
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi);

uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName);

void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t len);
void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t len);

#endif /* INC_STM32F407XX_USART_DRIVER_H_ */

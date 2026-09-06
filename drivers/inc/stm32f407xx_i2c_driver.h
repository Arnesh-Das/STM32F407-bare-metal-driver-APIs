#ifndef INC_STM32F407XX_I2C_DRIVER_H_
#define INC_STM32F407XX_I2C_DRIVER_H_

#include <stdint.h>

typedef struct I2C_RegDef_t I2C_RegDef_t;

typedef struct
{
    uint32_t SCL_Speed;
    uint8_t  I2C_DeviceAddress;
    uint8_t  I2C_ACKControl;
    uint8_t  I2C_FMDutyCycle;
} I2C_Config_t;

typedef struct
{
    I2C_RegDef_t *pI2Cx;
    I2C_Config_t I2C_Config;
} I2C_Handle_t;

#define I2C_SCL_SPEED_SM  100000U
#define I2C_SCL_SPEED_FM  400000U

#define I2C_ACK_ENABLE   1
#define I2C_ACK_DISABLE  0

#define I2C_FM_DUTY_2     0
#define I2C_FM_DUTY_16_9  1

/* SR1 flag bit masks */
#define I2C_FLAG_SB     (1 << 0)
#define I2C_FLAG_ADDR   (1 << 1)
#define I2C_FLAG_BTF    (1 << 2)
#define I2C_FLAG_RXNE   (1 << 6)
#define I2C_FLAG_TXE    (1 << 7)
#define SB    I2C_FLAG_SB
#define ADDR  I2C_FLAG_ADDR
#define BTF   I2C_FLAG_BTF
#define RXNE  I2C_FLAG_RXNE
#define TXE   I2C_FLAG_TXE

void I2C_PeripheralClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);
void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_RegDef_t *pI2Cx);

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName);
uint32_t RCC_GetPCLK1Value(void);

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint8_t slaveaddress, uint32_t len);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t slaveaddress, uint32_t len);

#endif /* INC_STM32F407XX_I2C_DRIVER_H_ */

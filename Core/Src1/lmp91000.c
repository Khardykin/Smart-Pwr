//===============================================================================
//
//
//
//
//	lmp91000.c
//                                                   				  31.08.2021
//
//==============================================================================

#include "main.h"
#include "debug.h"
#include "defines.h"
#include "lmp91000.h"
#include "device.h"

//==============================================================================

#define LMP_PORT I2C1
#define LMP_I2C_ADDRESS (0x48 << 1)

#define LMP_STATUS_REG                         (0x00)
#define LMP_LOCK_REG                           (0x01)
#define LMP_TIACN_REG                          (0x10)
#define LMP_REFCN_REG                          (0x11)
#define LMP_MODECN_REG                         (0x12)

#define LMP_WRITE_LOCK                         (0x01)
#define LMP_WRITE_UNLOCK                       (0x00)
#define LMP_READY                              (0x01)
#define LMP_NOT_READY                          (0x00)

//==============================================================================

static void TimeOut_Set(uint32_t);
static BOOL TimeOutRead(void);

//==============================================================================

#define I2C_TO 1

static void I2C_ByteSend(uint8_t data){

	TimeOut_Set(I2C_TO);
	while(!TimeOutRead() && !LL_I2C_IsActiveFlag_TXIS(LMP_PORT));
	LL_I2C_TransmitData8(LMP_PORT, data);

}

//==============================================================================
// Запись в регистр

void LMP_WRITE_REG(uint8_t reg, uint8_t data){


	LL_I2C_HandleTransfer(LMP_PORT, LMP_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	I2C_ByteSend(reg);
	I2C_ByteSend(data);

	TimeOut_Set(I2C_TO);

	while(!TimeOutRead() && !LL_I2C_IsActiveFlag_STOP(LMP_PORT));
	LL_I2C_ClearFlag_STOP(LMP_PORT);

}

//==============================================================================
// Чтение регистра

uint8_t LMP_READ_REG(uint8_t reg){

	uint8_t byte;

	LL_I2C_HandleTransfer(LMP_PORT, LMP_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_WRITE);

	I2C_ByteSend(reg);

	LL_I2C_HandleTransfer(LMP_PORT, LMP_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_READ);

	TimeOut_Set(I2C_TO);
	while(!TimeOutRead() && !LL_I2C_IsActiveFlag_RXNE(LMP_PORT));
	byte = LL_I2C_ReceiveData8(LMP_PORT);

	LL_I2C_GenerateStopCondition(LMP_PORT);

	TimeOut_Set(I2C_TO);
	while(!TimeOutRead() && !LL_I2C_IsActiveFlag_STOP(LMP_PORT));
	LL_I2C_ClearFlag_STOP(LMP_PORT);

	return byte;

}

//==============================================================================

static uint8_t LMP_Tia;
static uint8_t LMP_Ref;
static uint8_t LMP_Mode;

enum{
	WITHOUT_MODE = 0,
	WITH_MODE
};

void LMP_MemSet(void){

	uint8_t tmp;

	tmp  = 0b00000011 &  HIBYTE(dev.Config.LMP_Gain);
	tmp |= 0b00011100 & (LOBYTE(dev.Config.LMP_Gain) << 2);

	LMP_Tia = tmp;

	tmp  = 0b00001111 &  HIBYTE(dev.Config.LMP_BiasSign);
	tmp |= 0b00010000 & (LOBYTE(dev.Config.LMP_BiasSign) 	<< 4);
	tmp |= 0b01100000 & (HIBYTE(dev.Config.LMP_Source) 		<< 5);
	tmp |= 0b10000000 & (LOBYTE(dev.Config.LMP_Source) 		<< 7);

	LMP_Ref = tmp;

	tmp  = 0b00000111 &  HIBYTE(dev.Config.LMP_FET);

	if(tmp == 5) tmp = 7;

	tmp |= 0b10000000 & (LOBYTE(dev.Config.LMP_FET) 		<< 7);

	LMP_Mode = tmp;

}


void LMP_write_reg(void){

	LMP_WRITE_REG(LMP_LOCK_REG, LMP_WRITE_UNLOCK);

	LMP_WRITE_REG(LMP_TIACN_REG, 	LMP_Tia);
	LMP_WRITE_REG(LMP_REFCN_REG, 	LMP_Ref);
	LMP_WRITE_REG(LMP_MODECN_REG, 	LMP_Mode);

	LMP_WRITE_REG(LMP_LOCK_REG, LMP_WRITE_LOCK);

}



void lmp_proc(void){

	LMP_MemSet();

}

void LMP_Init(void){
	uint8_t dat = 0;
	dat = LMP_READ_REG(LMP_STATUS_REG);

	d_printf("\n\rLMP STAT:0x%02X", dat);


	LMP_MemSet();
	LMP_write_reg();

	dat = LMP_READ_REG(LMP_TIACN_REG);

	d_printf(" TIA:0x%02X", dat);

	dat = LMP_READ_REG(LMP_REFCN_REG);
	d_printf(" REF:0x%02X", dat);

	dat = LMP_READ_REG(LMP_MODECN_REG);
	d_printf(" MODE:0x%02X ", dat);
}

void LMP_Set_Mode(uint8_t reg){

	LMP_Mode = 0x07 & reg;
	LMP_Mode |= 0b10000000 & (LOBYTE(dev.Config.LMP_FET) 		<< 7);

	LMP_write_reg();

}

//==============================================================================

static uint32_t TimeOutDelay;

static uint32_t TimeOutCnt;

static void TimeOut_Set(uint32_t timeOut){

	__IO uint32_t  tmp;
	tmp = SysTick->CTRL;

	((void)tmp);

	TimeOutDelay = timeOut+1;

}

static BOOL TimeOutRead(void){

	if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U)
	{
		TimeOutDelay--;
	}

	if(TimeOutDelay > 0){
		return FALSE;
	}

	TimeOutCnt++;

	return TRUE;
}

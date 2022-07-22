#include "main.h"
#include "debug.h"
#include "defines.h"
#include "ADS1115.h"
#include "device.h"

//==============================================================================
static uint16_t ADS_READ_REG(uint8_t reg);
static void 	ADS_WRITE_REG(uint8_t reg, uint16_t data);
static void 	TimeOut_Set_I2C(uint32_t);
static BOOL 	TimeOut_Read_I2C(void);
//==============================================================================
uint8_t ads_conversion_ready = 0; // Флаг готовности преобразования ADS

static uint16_t config_ads;
static uint32_t TimeOutDelay;
static uint32_t TimeOutCnt;

//==============================================================================
static void I2C_ByteSend(uint8_t data)
{
	TimeOut_Set_I2C(I2C_TO);
	while(!TimeOut_Read_I2C() && !LL_I2C_IsActiveFlag_TXIS(ADS_PORT))
	{
		__NOP();
	}
	LL_I2C_TransmitData8(ADS_PORT, data);
	return;
}

//==============================================================================
// Запись в регистр
static void ADS_WRITE_REG(uint8_t reg, uint16_t data)
{
	LL_I2C_HandleTransfer(ADS_PORT, ADS_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 3, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	I2C_ByteSend(reg);
	I2C_ByteSend((uint8_t)(data>>8));
	I2C_ByteSend((uint8_t)(data));

	TimeOut_Set_I2C(I2C_TO);
	while(!TimeOut_Read_I2C() && !LL_I2C_IsActiveFlag_STOP(ADS_PORT));
	LL_I2C_ClearFlag_STOP(ADS_PORT);
}

//==============================================================================
// Чтение регистра
static uint16_t ADS_READ_REG(uint8_t reg)
{
	uint8_t byte[2] = {0,0};

	LL_I2C_HandleTransfer(ADS_PORT, ADS_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_WRITE);

	I2C_ByteSend(reg);

	LL_I2C_HandleTransfer(ADS_PORT, ADS_I2C_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_READ);

	TimeOut_Set_I2C(I2C_TO);
	while(!TimeOut_Read_I2C() && !LL_I2C_IsActiveFlag_RXNE(ADS_PORT));
	byte[0] = LL_I2C_ReceiveData8(ADS_PORT);
	while(!TimeOut_Read_I2C() && !LL_I2C_IsActiveFlag_RXNE(ADS_PORT));
	byte[1] = LL_I2C_ReceiveData8(ADS_PORT);


	LL_I2C_GenerateStopCondition(ADS_PORT);

	TimeOut_Set_I2C(I2C_TO);
	while(!TimeOut_Read_I2C() && !LL_I2C_IsActiveFlag_STOP(ADS_PORT));
	LL_I2C_ClearFlag_STOP(ADS_PORT);
#ifdef CONFIG_FID
	return ((((byte[0]<<8) + byte[1]) + 0x8000));
#endif
#ifdef CONFIG_PI
	return ~(((byte[0]<<8) + byte[1]) + 0x8000);
#endif

}

//==============================================================================
// Инициализация
void ADS_Init(uint16_t gain)
{
	config_ads = (gain |
			ADS_CONFIG_REG_MUX_DIF_0_1 |
			ADS_CONFIG_REG_MODE_SINGLE |
			ADS_CONFIG_REG_RATE_128SPS |
			ADS_CONFIG_REG_CMODE_TRAD |
			ADS_CONFIG_REG_CPOL_ACTVLOW |
			ADS_CONFIG_REG_CLAT_NONLAT |
			ADS_CONFIG_REG_CQUE_NONE);
	ADS_WRITE_REG(ADS_CONFIG_REG, config_ads);
}

//==============================================================================
// Чтение аналогового дифференциального вывода
uint16_t ADS_Read_Diff(uint16_t data, uint16_t gain)
{
	uint16_t time_conversion = 10;
#if 0
	uint16_t dataread = 0;
	dataread = ADS_READ_REG(ADS_CONFIG_REG);
	dataread &=~ ADS_CONFIG_REG_MUX_MASK;
	dataread |= (data | ADS_CONFIG_REG_OS_SINGLE);

	ADS_WRITE_REG(ADS_CONFIG_REG, dataread);
#else
	config_ads &=~ ADS_CONFIG_REG_PGA_MASK;
	config_ads |= gain;
	config_ads &=~ ADS_CONFIG_REG_MUX_MASK;
	config_ads |= data;

	ADS_WRITE_REG(ADS_CONFIG_REG, (config_ads | ADS_CONFIG_REG_OS_SINGLE));
#endif
	while(!((ADS_READ_REG(ADS_CONFIG_REG) & ADS_CONFIG_REG_OS_MASK) == 0) && time_conversion--);

	if(time_conversion == 0)
	{
		return 0;
	}
	data = ADS_READ_REG(ADS_CONVERSION_REG);
	return data;
}
//==============================================================================
// Чтение значения ацп
static uint32_t ADS_Result;
uint16_t ADS_Read_adc(uint16_t gain)
{
	uint32_t avg = 0;
	ADS_Result = 0;
	for(uint16_t i = 0; i < ADS_POINTS_DEFAULT; i++){
		ADS_Result += ADS_Read_Diff(ADS_CONFIG_REG_MUX_DIF_0_1, gain);
	}
	avg = ((ADS_Result<<1)/ADS_POINTS_DEFAULT + 1)>>1;
	return avg;
}
//==============================================================================
// Преобразование ADC в mV
uint16_t ADS_Read_volt(uint16_t adc_data)
{
	uint16_t fsRange;
	switch(config_ads&ADS_CONFIG_REG_PGA_MASK){
	case GAIN_TWOTHIRDS:
		fsRange = 6144;
		break;
	case GAIN_ONE:
		fsRange = 4096;
		break;
	case GAIN_TWO:
		fsRange = 2048;
		break;
	case GAIN_FOUR:
		fsRange = 1024;
		break;
	case GAIN_EIGHT:
		fsRange = 512;
		break;
	case GAIN_SIXTEEN:
		fsRange = 256;
		break;
	default:
		fsRange = 0;
		break;
	}

	return ((adc_data * (fsRange*10000 / 32768)))/10000;
}

//==============================================================================
static void TimeOut_Set_I2C(uint32_t timeOut)
{
	__IO uint32_t  tmp;
	tmp = SysTick->CTRL;

	((void)tmp);

	TimeOutDelay = timeOut+1;
}

static BOOL TimeOut_Read_I2C(void)
{
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


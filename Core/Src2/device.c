//===============================================================================
//
//
//
//
//	device.c
//                                                   				  03.09.2021
//
//==============================================================================

#include "main.h"
#include "defines.h"
#include "debug.h"
#include "device.h"
#include "lmp91000.h"
#include "ADS1115.h"
#include "modbus.h"
#include "calculations.h"

stMain_td stMain;

dev_td dev;

uint8_t CntTo250ms;
BOOL f_Time250ms = FALSE;

uint8_t CntTo500ms;
BOOL f_Time500ms = FALSE;

///000
uint8_t CntTo2min;
BOOL f_Time2min = TRUE;
BOOL f_TimeCalibFid = TRUE;

uint8_t CntToSec = 0;
int32_t CntSec = 0;
int32_t HourTimer = 0;

uint32_t Cnt_1_128;

BOOL f_readADC = FALSE;

void timer_1_128(void){

	Cnt_1_128++;

	if(CntTo250ms < (32-1)){

		CntTo250ms++;

	}else{

		f_Time250ms = TRUE;
		CntTo250ms = 0;

		if(CntTo500ms == 0){

			f_Time500ms = TRUE;

			CntToSec ^= 1;

			if(CntToSec == 0){
				CntSec++;
				CntTo2min++;
				if(CntTo2min == 120){// 2 минуты
					CntTo2min = 0;
					f_Time2min = TRUE;
				}
			}

		}

		CntTo500ms ^= 1;

	}
}

//==============================================================================

uint16_t serviceTimer;

void serviceTimerStop(void);

void serviceTimerProc(void){

	if(serviceTimer > 1){
		serviceTimer--;
	}else{
		if(serviceTimer == 1){
			serviceTimerStop();
		}
	}

}

void serviceTimerStart(uint16_t time){
	serviceTimer = time;
}

void serviceTimerStop(void){

	mbServiceMode = FALSE;
	dev.Status &=~ (1 << STATUS_BIT_MAIN_MODE);
	mbDevStatus = dev.Status;

	mbUnlock = FALSE;
	AccessCode = 0;

	serviceTimer = 0;
}

uint16_t ADC_in[3];
uint16_t ADC_in_Temper;
uint8_t adc_cnt = 0;
uint16_t ADC_in_mVolt;

uint16_t ADC_in_mVolt_TIA;
uint16_t ADC_in_mVolt_Temper;

int16_t ADC_in_Celsius;
uint32_t ADC_in_RefVoltage = ((uint32_t)3300);
int16_t LMP_temper;

BOOL f_AdcCycleEnd;

//==============================================================================
///000
void dev_set_config_default(void){
#ifdef CONFIG_EC
	//	dev.Config.TypeSensor = 0;
	//	dev.Config.Unit = 0;
	//	dev.Config.ValueLow = 0;
		dev.Config.ValueHigh = 1000;
		dev.Config.ValueCalib = 500;
	//	dev.Config.ADCLow	= 0;
		dev.Config.ADCHigh	= 4095;

		//dev.Config.CalibZeroADC = 0;
		dev.Config.CalibConcADC = 4095;

//	for(int i = 0; i < 8; i++)
//		dev.Config.Text[i] = 0;

	dev.Config.LMP_Gain 	= 0x0105;
	dev.Config.LMP_BiasSign = 0x0000;
	dev.Config.LMP_Source	= 0x0001;
	dev.Config.LMP_FET		= 0x0700;
#endif

#ifdef CONFIG_IR
	//	dev.Config.TypeSensor = 0;
	//	dev.Config.Unit = 0;
	//	dev.Config.ValueLow = 0;
		dev.Config.ValueHigh = 1000;
		dev.Config.ValueCalib = 500;
	//	dev.Config.ADCLow	= 0;
		dev.Config.ADCHigh	= 0;

		//dev.Config.CalibZeroADC = 0;
		dev.Config.CalibConcADC = 0;

	//	for(int i = 0; i < 8; i++)
	dev.Config.LMP_Gain = ADS_CONFIG_REG_PGA_1_024V;
#endif


}

//==============================================================================

void dev_init(void){

//	dev_set_config_default();

	dev.RegInput.cod_8216 = 8216;

	dev.RegInput.VerSW = 0x0101;
	dev.RegInput.VerSW_Build = 0x0001;

	dev.Status = (1 << STATUS_BIT_MAIN_INIT);
}

//==============================================================================

void dev_proc(void){

	serviceTimerProc();

	mbDevStatus = dev.Status;

	if(!f_readADC)
		return;

//#define DEBUG_ADC

#ifdef DEBUG_ADC
		d_printf("\n\r");
		d_printf("(%04X %04X) %04X %04X", ADC_in_Temper, ADC_in[0], ADC_in[1], ADC_in[2]);
		d_printf(" |  (TV:%05d) TIA:%05d V:%05d T_LMP:%02d (T_MPU:%02d)", ADC_in_mVolt_Temper, ADC_in_mVolt_TIA,  ADC_in_RefVoltage, LMP_temper, ADC_in_Celsius);
#endif

}

//==============================================================================
//
// Расчёт коэффициентов по точкам -25 и 45

#define C_SUB	1556928
#define C_DIV 	8042

void Adc_Eoc_Callback(void){

	ADC_in[adc_cnt] = LL_ADC_REG_ReadConversionData12(ADC1);

	if(adc_cnt >= 2){

		f_AdcCycleEnd = TRUE;
		adc_cnt = 0;

	}else{

		adc_cnt++;

	}

}

///000
#ifdef CONFIG_EC
BOOL lmp_tia_or_temper;
void Adc_read_data(void){
	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);
	ADC_in_mVolt = __LL_ADC_CALC_DATA_TO_VOLTAGE(ADC_in_RefVoltage, ADC_in[0], LL_ADC_RESOLUTION_12B);

	//LMP_temper = (1562.2 - ADC_in_mVolt)/8.16;
	//ADC_in_mVolt = 1797;

	int32_t tmp;

	if(lmp_tia_or_temper){

		LMP_Set_Mode(MODE_TEMPERAT);

		ADC_in_mVolt_TIA = ADC_in_mVolt;
		dev.RegInput.ADC_0 = ADC_in[0];

		SetGasValue();

	}
	else{

		LMP_Set_Mode(MODE_TIA);

		ADC_in_Temper = ADC_in[0];

		ADC_in_mVolt_Temper = ADC_in_mVolt;

		tmp = 10 * (C_SUB - ADC_in_mVolt * 1000);

		if(tmp > 0)
			LMP_temper = (tmp + C_DIV/2)/C_DIV;
		else
			LMP_temper = (tmp - C_DIV/2)/C_DIV;

		dev.RegInput.TempSensor = LMP_temper;

	//LMP_temper = (((int)(1562.2*256) - (ADC_in_mVolt<<8)) * (int)(256/8.16)) >> 16;

		f_readADC = TRUE;

	}

	lmp_tia_or_temper = !lmp_tia_or_temper;

}
#endif

#ifdef CONFIG_IR
void Adc_read_data(void){

	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);

	// 2 минуты
	if(f_Time2min || f_TimeCalibFid){
		f_Time2min = FALSE;
		// Включаем питание на сенсоре
		dev.RegInput.ADC_0 = ADS_Read_adc(dev.Config.LMP_Gain);
		dev.RegInput.Volt_Sens = ADS_Read_volt(dev.RegInput.ADC_0);
		dev.RegInput.TempSensor = ADC_in_Celsius;
	}

#define DEBUG_ADS1115
#ifdef DEBUG_ADS1115
		d_printf("ADC - %05d Volt - %05d Temp:%d", dev.RegInput.ADC_0, dev.RegInput.Volt_Sens,  dev.RegInput.TempSensor);
		d_printf("\n\r");
#endif
	SetGasValue();
}
#endif
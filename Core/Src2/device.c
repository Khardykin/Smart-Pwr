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
#include "msi.h"
#include "Mipex_command.h"

//stMain_td stMain;

dev_td dev;

uint8_t CntTo250ms;
BOOL f_Time250ms = FALSE;

uint8_t CntTo500ms;
BOOL f_Time500ms = FALSE;

///000
#ifdef CONFIG_MIPEX
	uint8_t CntTo3s = 0;
	BOOL f_Time3s = TRUE;
#endif

///000
uint8_t CntTo2min = 0;
BOOL f_Time2min = TRUE;
#if defined(CONFIG_PI) || defined(CONFIG_FID)
	BOOL f_TimeCalibFid = FALSE;
#endif

uint8_t CntToSec = 0;
int32_t CntSec = 0;
int32_t HourTimer = 0;

static uint32_t Cnt_1_128 = 0;

BOOL f_readADC = FALSE;

void timer_1_128(void)
{

	Cnt_1_128++;
	///000
	if(TimerRxMipex){
		TimerRxMipex--;
	}
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
#ifdef CONFIG_MIPEX
				CntTo3s++;
				if(CntTo3s == 3){// 2 секунды
					CntTo3s = 0;
					f_Time3s = TRUE;
				}
#endif
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

void serviceTimerProc(void)
{

	if(serviceTimer > 1){
		serviceTimer--;
	}else{
		if(serviceTimer == 1){
			serviceTimerStop();
		}
	}

}

void serviceTimerStart(uint16_t time)
{
	serviceTimer = time;
}

void serviceTimerStop(void)
{

	mbServiceMode = FALSE;
	dev.Status &=~ (1 << STATUS_BIT_MAIN_MODE);
	mbHoldDevStatus = dev.Status;

	mbUnlock = FALSE;
	AccessCode = 0;

	serviceTimer = 0;
}

//==============================================================================

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
BOOL f_AdcDataBad;

//==============================================================================
///000
void dev_set_config_default(void)
{
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
	dev.Config.LMP_Mode		= 0x0700;
#endif

#ifdef CONFIG_PI
	dev.Config.TypeSensor = (SENSOR_TYPE_PI << 8);
	dev.Config.Unit = 0x0200|(1 << CFG_UNIT_VALUE_lel);

	dev.Config.ValueLow = 0;
	dev.Config.ValueHigh = 10000;

	dev.Config.ScaleKoef = 10;
	dev.Config.FID = ADS_CONFIG_REG_PGA_0_256V;
#endif
#ifdef CONFIG_FID
	dev.Config.TypeSensor = (SENSOR_TYPE_FID << 8);
	dev.Config.Unit = 0x0200|(1 << CFG_UNIT_VALUE_lel);

	dev.Config.ValueLow = 0;
	dev.Config.ValueHigh = 10000;

	dev.Config.ScaleKoef = 10;
	dev.Config.FID = ADS_CONFIG_REG_PGA_0_256V;
#endif

#ifdef CONFIG_MIPEX
	dev.Config.TypeSensor = (SENSOR_TYPE_MX << 8);
	dev.Config.Unit = 0x0200|(1 << CFG_UNIT_VALUE_lel);

	dev.Config.ValueLow = 0;
	dev.Config.ValueHigh = 10000;

	dev.Config.ScaleKoef = 10;
#endif


}

//==============================================================================
#ifdef CONFIG_EC
	#define INIT_MODE_TIME 20
#endif
#ifdef CONFIG_MIPEX
	#define INIT_MODE_TIME 60
#endif
#if defined(CONFIG_PI)
	#define INIT_MODE_TIME 5
#elif defined(CONFIG_FID)
	#define INIT_MODE_TIME 30
#endif

void dev_init(void){

//	dev_set_config_default();

	dev.RegInput.cod_8225 = 8225;

	dev.RegInput.VerSW = 0x0101;
	dev.RegInput.VerSW_Build = 0x0001;

	dev.Status = (1 << STATUS_BIT_MAIN_INIT);

	dev.RegInput.TimeToOffHeat = INIT_MODE_TIME;
}

//==============================================================================
#ifdef CONFIG_EC

void heat_proc(void)
{

	if(dev.f_heat){
		if(dev.RegInput.TempSensor > -100){
			dev.f_heat = FALSE;
		}

	}else{
		if(dev.RegInput.TempSensor < -150){
			dev.f_heat = TRUE;
		}
	}

	if(!f_readADC)
		dev.f_heat = FALSE;

//	dev.f_heat = TRUE;

	if(dev.f_heat)
		SET_HEAT_ON;
	else
		SET_HEAT_OFF;



}
#endif
#ifdef CONFIG_PI

#define HEAT_TIME_PERIOD		(20)
#define HEAT_TIME_PULSE			(1)
#define HEAT_TIME_DEC_PERIOD	(INIT_MODE_TIME*1000/HEAT_TIME_PERIOD)
uint8_t flag_1ms;
#define WARM_OPTION 			(1)
#if WARM_OPTION
	static uint32_t CounterPulseDuty = HEAT_TIME_PERIOD;
	static uint32_t CounterPulse = HEAT_TIME_PULSE;
	static uint32_t time_warm_all = (INIT_MODE_TIME*1000);
	static uint32_t time_warm_dec_period;// Декремент периода
	static float CounterDec = 0;

	void delay_us(uint32_t us)
	{
		TIM2->CNT = 0;
		while (TIM2->CNT < us);
	}
#else
	static uint16_t CountPeriod = 0;
	static uint16_t Counter = 0;
	static uint16_t CounterDecPeriod = 0;
	static uint8_t flagPulse;
#endif
void heat_proc(void)
{

#if WARM_OPTION
	if(dev.Status & (1 << STATUS_BIT_MAIN_INIT)){
		time_warm_dec_period = (uint32_t)((float)time_warm_all/((CounterPulse + CounterPulseDuty)/1000.0));
		CounterDec = (float)(CounterPulseDuty - CounterPulse)/(float)time_warm_dec_period;
		for(uint32_t i = 0; i < time_warm_dec_period; i++){
			SET_HEAT_OFF;
			delay_us(CounterPulse + (uint32_t)(CounterDec*i + 0.5));
			SET_HEAT_ON;
			delay_us(CounterPulseDuty - (uint32_t)(CounterDec*i + 0.5));
		}
		// Включаем питание на сенсоре
		SET_HEAT_OFF;
	}
#else
	if(dev.Status & (1 << STATUS_BIT_MAIN_INIT)){
		if(flag_1ms){
			flag_1ms = 0;
			if(flagPulse){
				SET_HEAT_OFF;
			}
			else{
				SET_HEAT_ON;
			}
			Counter++;
			CounterDecPeriod++;
			if(CounterDecPeriod >= HEAT_TIME_DEC_PERIOD){
				if(CountPeriod < HEAT_TIME_PERIOD){
					CountPeriod++;
				}
				CounterDecPeriod = 0;
			}
			if(flagPulse){
				if(Counter >= HEAT_TIME_PULSE){
					Counter = 0;
					flagPulse = 0;
				}
			}
			else{
				if((Counter + CountPeriod) >= HEAT_TIME_PERIOD){
					Counter = 0;
					flagPulse = 1;
				}
			}
		}
	}
#endif
	else{
		// Включаем питание на сенсоре
		SET_HEAT_OFF;
	}
}
#endif
//==============================================================================
void dev_proc(void)
{
	if(dev.RegInput.TimeToOffHeat != 0)
		dev.RegInput.TimeToOffHeat = INIT_MODE_TIME - CntSec;

	// Если окончен режим инициализации
	if(((dev.Status & (1 << STATUS_BIT_MAIN_INIT)) != 0) && (CntSec >= INIT_MODE_TIME)){
		dev.Status &=~ (1 << STATUS_BIT_MAIN_INIT);
		dev.Status |= (1 << STATUS_BIT_MAIN_RUN);
		dev.RegInput.TimeToOffHeat = 0;
		// Выключаем системный таймер
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	}

	serviceTimerProc();

	mbHoldDevStatus = dev.Status;
#ifdef CONFIG_EC
	heat_proc();
	if(!f_readADC)
		return;
#endif


//#define DEBUG_ADC

#ifdef DEBUG_ADC
#ifdef DEBUG_MY
		d_printf("\n\r");
		d_printf("(%04X %04X) %04X %04X", ADC_in_Temper, ADC_in[0], ADC_in[1], ADC_in[2]);
		d_printf(" |  (TV:%05d) TIA:%05d V:%05d T_LMP:%02d (T_MPU:%02d)", ADC_in_mVolt_Temper, ADC_in_mVolt_TIA,  ADC_in_RefVoltage, LMP_temper, ADC_in_Celsius);
#endif
#endif

}

//==============================================================================
void Adc_Eoc_Callback(void)
{

	if(LL_ADC_IsActiveFlag_EOS(ADC1)){
		adc_cnt = 2;
	}

	ADC_in[adc_cnt] = LL_ADC_REG_ReadConversionData12(ADC1);

	if(adc_cnt >= 2){

		f_AdcCycleEnd = TRUE;
		adc_cnt = 0;

	}else{

		adc_cnt++;

	}

}

#ifdef CONFIG_EC
//==============================================================================

BOOL lmp_tia_or_temper;

//==============================================================================
// Расчёт температуры по коэффициентам в точках -25 и 45

#define C_SUB	1556928
#define C_DIV 	8042

//==============================================================================
///000

BOOL lmp_tia_or_temper;
void Adc_read_data(void)
{


	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_mVolt = __LL_ADC_CALC_DATA_TO_VOLTAGE(ADC_in_RefVoltage, ADC_in[0], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);

	//LMP_temper = (1562.2 - ADC_in_mVolt)/8.16;
	//ADC_in_mVolt = 1797;

	int32_t tmp;

	if(lmp_tia_or_temper){

		LMP_Set_Mode(MODE_TEMPERAT);

		ADC_in_mVolt_TIA = ADC_in_mVolt;
		dev.RegInput.ADC_0 = ADC_in[0];

		// Если рабочий цикл запущен
		if((dev.Status & (1 << STATUS_BIT_MAIN_RUN)) != 0){

			SetGasValue();
			SetGasValue_mg_m3();
		}

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

#ifdef CONFIG_FID
void Adc_read_data(void)
{
	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);

	// 2 минуты
	if(f_Time2min || f_TimeCalibFid){
		f_Time2min = FALSE;
		// Включаем питание на сенсоре
		LL_GPIO_SetOutputPin(TURN_ON_IR_GPIO_Port, TURN_ON_IR_Pin);
		dev.Status |= (1 << STATUS_BIT_FID_PWR);

		dev.RegInput.ADC_0 = ADS_Read_adc(dev.Config.FID);
		dev.RegInput.Volt_Sens = ADS_Read_volt(dev.RegInput.ADC_0);
		dev.RegInput.TempSensor = ADC_in_Celsius;
#define DEBUG_ADS1115
#ifdef DEBUG_ADS1115
#ifdef DEBUG_MY
		d_printf("ADC - %05d Volt - %05d Temp:%d", dev.RegInput.ADC_0, dev.RegInput.Volt_Sens,  dev.RegInput.TempSensor);
		d_printf("\n\r");
#endif
#endif
		if(!f_TimeCalibFid){
			// Выключаем питание на сенсоре
			LL_GPIO_ResetOutputPin(TURN_ON_IR_GPIO_Port, TURN_ON_IR_Pin);
			dev.Status &=~ (1 << STATUS_BIT_FID_PWR);
		}
		SetGasValue();
		//--------------------------------------------------------------------
		// Перевод в единицу измерения НКПР
		if(dev.Config.Unit & (1 << CFG_UNIT_VALUE_vol)){
			dev.RegInput.dwValue_mg_m3 = (dev.RegInput.Value*dev.Config.ScaleKoef)/10;
		}
	}

}
#endif

#ifdef CONFIG_PI
uint16_t ads0, ads1;
void Adc_read_data(void)
{
	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);

	dev.RegInput.ADC_0 = ADS_Read_adc(ADS_CONFIG_REG_PGA_0_256V);
	dev.RegInput.Volt_Sens = ADS_Read_volt(dev.RegInput.ADC_0);
	dev.RegInput.TempSensor = ADC_in_Celsius;
//	ads0 = ADS_Read_Diff(ADS_CONFIG_REG_MUX_DIF_0_N, ADS_CONFIG_REG_PGA_0_256V);
//	ads1 = ADS_Read_Diff(ADS_CONFIG_REG_MUX_DIF_1_N, ADS_CONFIG_REG_PGA_0_256V);
#define DEBUG_ADS1115
#ifdef DEBUG_ADS1115
#ifdef DEBUG_MY
		d_printf("ADC - %05d Volt - %05d Temp:%d", dev.RegInput.ADC_0, dev.RegInput.Volt_Sens,  dev.RegInput.TempSensor);
		d_printf("\n\r");
#endif
#endif
	SetGasValue();
	//--------------------------------------------------------------------
	// Перевод в единицу измерения НКПР
	if(dev.Config.Unit & (1 << CFG_UNIT_VALUE_vol)){
		dev.RegInput.dwValue_mg_m3 = (dev.RegInput.Value*dev.Config.ScaleKoef)/10;
	}
}
#endif

#ifdef CONFIG_MIPEX
void Adc_read_data(void)
{
	ADC_in_RefVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_in[1], LL_ADC_RESOLUTION_12B);
	ADC_in_Celsius = 10 * __LL_ADC_CALC_TEMPERATURE(ADC_in_RefVoltage, ADC_in[2], LL_ADC_RESOLUTION_12B);

	// 2 минуты
	if(f_Time2min){
		f_Time2min = FALSE;
		uint32_t MSIFrequencyCalib;
		MSI_CalibrateFixedError(20000, &MSIFrequencyCalib);
//#define DEBUG_MSI_Calib
#ifdef DEBUG_MSI_Calib
#ifdef DEBUG_MY
	d_printf("MSIFreqCalib %d", MSIFrequencyCalib);
	d_printf("\n\r");
#endif
#endif
	}
	// 3 секунды
	if(f_Time3s){
		f_Time3s = FALSE;
		Mipex_transmit_commmand(COMMAND_DATAE2);
		//--------------------------------------------------------------------
		// Перевод в единицу измерения НКПР
		if(dev.Config.Unit & (1 << CFG_UNIT_VALUE_vol)){
			dev.RegInput.dwValue_mg_m3 = (dev.RegInput.Value*dev.Config.ScaleKoef)/10;
		}
//		//--------------------------------------------------------------------
//		// Дискретность
//		uint8_t disc = (dev.Config.Unit & 0x03000)>>16;
//		if(disc > MIPEX_DISC){
//			for(uint8_t i = MIPEX_DISC; i < disc; i++){
//				dev.RegInput.Value *= 10;
//			}
//		}
//		else{
//			for(uint8_t i = disc; i < MIPEX_DISC; i++){
//				dev.RegInput.Value /= 10;
//			}
//		}
		//--------------------------------------------------------------------
		// Temp
		dev.RegInput.TempSensor = ADC_in_Celsius;
	}

//#define DEBUG_ADS1115
#ifdef DEBUG_ADS1115
#ifdef DEBUG_MY
	d_printf("ADC - %05d Volt - %05d Temp:%d", dev.RegInput.ADC_0, dev.RegInput.Volt_Sens,  dev.RegInput.TempSensor);
	d_printf("\n\r");
#endif
#endif

//	SetGasValueMipex();

}
#endif

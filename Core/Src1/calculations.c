//===============================================================================
//
//
//
//
//	calculations.c
//                                                   				  21.09.2021
//
//==============================================================================

#include "main.h"
#include "defines.h"
#include "debug.h"
#include "calculations.h"
#include "device.h"
#include "arhiv.h"
#include "flash.h"

//==============================================================================

void CalibGasZero(void){

	uint32_t tmp;

	dev.Config.CalibZeroTemper = dev.RegInput.TempSensor;
	dev.Config.CalibZeroADC = dev.RegInput.ADC_0;

	tmp = dev.RegInput.TempSensor << 16;
	tmp |= dev.RegInput.ADC_0;

	ArhivStoreNote(ARCHIVE_SET_CALIB_ZERO,tmp);

}

void CalibGasConc(void){

	uint32_t tmp;

	dev.Config.CalibConcTemper = dev.RegInput.TempSensor;
	dev.Config.CalibConcADC = dev.RegInput.ADC_0;

	tmp = dev.Config.ValueCalib << 16;
	tmp |= dev.RegInput.ADC_TK;

	ArhivStoreNote(ARCHIVE_SET_CALIB_CONC,tmp);
}

//==============================================================================

#define K_MUL 1

uint32_t get_koef_temper_conc(int16_t temperat){

	uint32_t temper_koef = 1000 * K_MUL;

	uint16_t i;

	BOOL find_temper = FALSE;
	BOOL find_null = FALSE;

	int16_t dt, temper_1, temper_2;
	uint32_t dk, koef_1, koef_2;

	temper_1 = dev.Config.temp_corr_conc[0].Temp;
	koef_1 = dev.Config.temp_corr_conc[0].Koef;

	temper_2 = dev.Config.temp_corr_conc[1].Temp;
	koef_2 = dev.Config.temp_corr_conc[1].Koef;

	if(koef_1 == 0){

		return temper_koef;

	}

	if(koef_2 == 0){

		temper_koef =  koef_1 * K_MUL;

		return temper_koef;

	}

	if(temperat < temper_1){

		temper_koef = koef_1 * K_MUL;

		return temper_koef;
	}

	if((temperat >= temper_1) && (temperat <= temper_2))
		find_temper = TRUE;

	for(i = 1; (i < 6) && (!find_temper) && (!find_null); i++){

		temper_1 = dev.Config.temp_corr_conc[i].Temp;
		koef_1 = dev.Config.temp_corr_conc[i].Koef;

		temper_2 = dev.Config.temp_corr_conc[i+1].Temp;
		koef_2 = dev.Config.temp_corr_conc[i+1].Koef;

		if(koef_2 == 0){
			temper_koef = koef_1 * K_MUL;
			find_null = TRUE;
		}

		if((temperat >= temper_1) && (temperat <= temper_2))
			find_temper = TRUE;

	}

	if(find_temper){

		dt = temperat - temper_1;

		dk = (koef_2 - koef_1) * dt * K_MUL / (temper_2 - temper_1);

		temper_koef =  (koef_1 * K_MUL) + dk;

	}else if(temperat > temper_2){

		temper_koef = koef_2 * K_MUL;

	}

	return temper_koef;
}

/*
void SetGasValue_(void){

	uint32_t val;
	uint16_t adc_0_20c;
	uint32_t kc, k;
	uint32_t koef_tc;

	adc_0_20c = dev.Config.CalibZeroADC;

	if(dev.RegInput.ADC_0 > adc_0_20c)
		val = dev.RegInput.ADC_0 - adc_0_20c;
	else
		val = 0;

	kc = get_koef_temper_conc(dev.Config.CalibConcTemper);

	if((dev.Config.CalibConcADC - dev.Config.CalibZeroADC) > 0)
		k = dev.Config.ValueCalib * kc / (dev.Config.CalibConcADC - dev.Config.CalibZeroADC);
	else
		k = 1000;

	val *= k;
	val /= 1000;

	koef_tc = get_koef_temper_conc(dev.RegInput.TempSensor);

	val *= 1000;
	val /= koef_tc;

	dev.RegInput.Value = val;

	//	#define DEBUG_CALC

#ifdef DEBUG_CALC
	d_printf("\n\r");
	d_printf("Val: %04X", dev.RegInput.Value);
#endif



}
*/

//==============================================================================

void SetGasValue(void){

	uint32_t val, adc;
	uint32_t kc, k;
	uint32_t koef_tc;

//	dev.RegInput.ADC_0 = dev.Config.CalibConcADC;
//	dev.RegInput.TempSensor = dev.Config.CalibConcTemper;

	if(dev.RegInput.ADC_0 > dev.Config.CalibZeroADC)
		adc = dev.RegInput.ADC_0 - dev.Config.CalibZeroADC;
	else
		adc = 0;

	if((dev.Config.CalibConcADC - dev.Config.CalibZeroADC) > 0){
		k = dev.Config.ValueCalib  * 10000/ (dev.Config.CalibConcADC - dev.Config.CalibZeroADC);
	}
	else{
		k = 10000;
	}

	val = adc*k;
	val += 5000;
	val /= 10000;

	dev.RegInput.Value_0 = val;

	kc = get_koef_temper_conc(dev.Config.CalibConcTemper);
	koef_tc = get_koef_temper_conc(dev.RegInput.TempSensor);

	val *= kc;
	val += (koef_tc >> 1);
	val /= koef_tc;

	dev.RegInput.Value = val;

	val = 1000 * adc;
	val += (koef_tc >> 1);
	val /= koef_tc;

	dev.RegInput.ADC_TK = val;

//#define DEBUG_CALC

#ifdef DEBUG_CALC
	d_printf("\n\r");
	d_printf("ADC: %04d, ADC_TK %04d", adc, dev.RegInput.ADC_TK);
//	d_printf("\n\r");
//	d_printf("Val: %04d, Val_0 %04d", dev.RegInput.Value,dev.RegInput.Value_0);
#endif

}

//==============================================================================

void test_temp_korr(void){

	int32_t koef;

	for(int16_t temper = -400; temper <= 500; temper++){

		koef = get_koef_temper_conc(temper);
		d_printf("\n\r%03d %04d", temper, koef);
		LL_mDelay(4);

	}

}

//==============================================================================

void SetGasValue_mg_m3(void){

	uint64_t k;

	k = 12;	// 0.00012 // *100000
	k *= dev.Config.MolarMass; // *100
	k *= 101325;
	k /= 29315; // /100

	dev.RegInput.dwValue_mg_m3 = dev.RegInput.Value * k / 1000000;
}

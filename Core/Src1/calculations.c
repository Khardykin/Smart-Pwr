//===============================================================================
//
//
//
//
//	calculations.c
//                                                   				  31.03.2022
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

	if(temperat < -600)
		temperat = -600;

	if(temperat > 500)
		temperat = 500;

	uint32_t temper_koef = 1000 * K_MUL;

	uint16_t i;

	BOOL find_temper = FALSE;
	BOOL find_null = FALSE;

	int16_t dt, temper_1, temper_2;

	int32_t dk;

	uint32_t koef_1, koef_2;

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

	for(i = 1; (i < 7) && (!find_temper) && (!find_null); i++){

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
void LinearizInit_O2(void){

	dev.Config.linear[0].Conc = 0;
	dev.Config.linear[0].Koef = 1000;

	dev.Config.linear[1].Conc = 45;
	dev.Config.linear[1].Koef = 1000 + ((1000.0 * 5.0)/45.0);

	dev.Config.linear[2].Conc = 140;
	dev.Config.linear[2].Koef = 1000 + ((1000.0 * 10.0)/140.0);

	dev.Config.linear[3].Conc = 190;
	dev.Config.linear[3].Koef = 1000;

	dev.Config.linear[4].Conc = 220;
	dev.Config.linear[4].Koef = 1000;

	dev.Config.linear[5].Conc = 300;
	dev.Config.linear[5].Koef = 1000 + ((1000.0 * (-10.0))/300.0);

	dev.Config.linear[6].Conc = 0;
	dev.Config.linear[6].Koef = 0;


}

#define TYPE_SEN_O2

void LinearizInit(void){

#ifdef TYPE_SEN_O2
	LinearizInit_O2();
#else
	memset(dev.Config.linear,0,sizeof(dev.Config.linear));
#endif

}

uint16_t LinearizKoef(uint32_t value){

	uint32_t linear_koef = 1000 * K_MUL;

	uint16_t i;

	BOOL find_val = FALSE;
	BOOL find_null = FALSE;

	int16_t dt, val_1, val_2;
	int32_t dk;
	uint32_t koef_1, koef_2;

	val_1 = dev.Config.linear[0].Conc;
	koef_1 = dev.Config.linear[0].Koef;

	val_2 = dev.Config.linear[1].Conc;;
	koef_2 = dev.Config.linear[1].Koef;

	if(koef_1 == 0){

		return linear_koef;

	}

	if(koef_2 == 0){

		linear_koef =  koef_1 * K_MUL;

		return linear_koef;

	}

	if(value < val_1){

		linear_koef = koef_1 * K_MUL;

		return linear_koef;
	}

	if((value >= val_1) && (value <= val_2))
		find_val = TRUE;

	for(i = 1; (i < 7) && (!find_val) && (!find_null); i++){

		val_1 = dev.Config.linear[i].Conc;
		koef_1 = dev.Config.linear[i].Koef;

		val_2 = dev.Config.linear[i+1].Conc;
		koef_2 = dev.Config.linear[i+1].Koef;

		if(koef_2 == 0){
			linear_koef = koef_1 * K_MUL;
			find_null = TRUE;
		}

		if((value >= val_1) && (value <= val_2))
			find_val = TRUE;

	}

	if(find_val){

		dt = value - val_1;

//		dk = (koef_2 - koef_1) * dt * K_MUL / (val_2 - val_1);

		dk = (koef_2 - koef_1) * dt * K_MUL;
		dk /= (val_2 - val_1);

		linear_koef =  (koef_1 * K_MUL) + dk;

	}else if(value > val_2){

		linear_koef = koef_2 * K_MUL;

	}

//	value_tmp = value * linear_koef;

//	value_tmp += 500;
//	value_tmp /= 1000;



	return (uint16_t) linear_koef;

}

void test_lineariz(void){

	int32_t koef;

	for(int16_t val = 0; val <= 300; val++){

		koef = LinearizKoef(val);
		d_printf("\n\r%03d %04d", val, koef);
		LL_mDelay(100);

	}

}

//==============================================================================

void SetGasValue(void){

	uint32_t val, val0;
	int32_t adc;
	int32_t k;
	uint32_t kc;
	uint32_t koef_tc;

//	dev.RegInput.ADC_0 = dev.Config.CalibConcADC;
//	dev.RegInput.TempSensor = dev.Config.CalibConcTemper;
#if 1
	adc = (int16_t)(dev.RegInput.ADC_0 - dev.Config.CalibZeroADC);
#else
	if(dev.RegInput.ADC_0 > dev.Config.CalibZeroADC)
		adc = (int16_t)(dev.RegInput.ADC_0 - dev.Config.CalibZeroADC);
	else
		adc = 0;
#endif

	if((dev.Config.CalibConcADC - dev.Config.CalibZeroADC) != 0){
		k = (int16_t)(dev.Config.ValueCalib * 10000) / (int16_t)(dev.Config.CalibConcADC - dev.Config.CalibZeroADC);
	}
	else{
		k = 10000;
	}

	if(!(((adc < 0) && (k < 0)) || ((adc > 0) && (k > 0)))){
		adc = 0;
	}

	val = adc*k;
	val += 5000;
	val /= 10000;

//	dev.RegInput.Value_0 = val;

	kc = get_koef_temper_conc(dev.Config.CalibConcTemper);
	koef_tc = get_koef_temper_conc(dev.RegInput.TempSensor);

	val *= kc;

	val0 = val;
	val0 += 500;
	val0 /= 1000;
	dev.RegInput.Value_0 = val0;

	val += (koef_tc >> 1);
	val /= koef_tc;

	uint32_t lin_val;
	uint16_t koef_lin;

	koef_lin = LinearizKoef(val);;

	lin_val = val * koef_lin;
	lin_val = (lin_val + 500) / 1000;

	dev.RegInput.Value = lin_val;


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

	if(arh.ValueMax < dev.RegInput.Value)
		arh.ValueMax = dev.RegInput.Value;

	if(arh.ValueMin > dev.RegInput.Value)
		arh.ValueMin = dev.RegInput.Value;

}

//==============================================================================

void test_temp_korr(void){

	int32_t koef;

	for(int16_t temper = -600; temper <= 500; temper++){

		koef = get_koef_temper_conc(temper);
		d_printf("\n\r%03d %04d", temper, koef);
		LL_mDelay(10);

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

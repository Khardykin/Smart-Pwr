//------------------------------------------------------------------------------
//
//
//  device.h
//                                                    	              03.09.2021
//
//------------------------------------------------------------------------------

#ifndef INC1_DEVICE_H_
#define INC1_DEVICE_H_

#include "defines.h"
///000
//#define CONFIG_EC		// Конфигурация для электрохимии
#define CONFIG_PI		// Конфигурация для термокатализа
//#define CONFIG_FID	// Конфигурация для MINIPID
//#define CONFIG_MIPEX	// Конфигурация для MIPIX

void delay_ms(uint32_t time);
void timer_1_128(void);

void dev_init(void);
void dev_proc(void);
void heat_proc(void);

#if defined(CONFIG_PI) || defined(CONFIG_EC)
	#define SET_HEAT_ON 	WRITE_REG(HEAT_GPIO_Port->BSRR, HEAT_Pin)
	#define SET_HEAT_OFF 	WRITE_REG(HEAT_GPIO_Port->BRR, HEAT_Pin)
#endif
#ifdef CONFIG_PI
	#define SET_TURN_ON 	WRITE_REG(TURN_ON_IR_GPIO_Port->BSRR, TURN_ON_IR_Pin);
	#define SET_TURN_OFF 	WRITE_REG(TURN_ON_IR_GPIO_Port->BRR, TURN_ON_IR_Pin);
#endif

extern BOOL f_Time250ms;
extern BOOL f_Time500ms;
///000
#ifdef CONFIG_MIPEX
	extern BOOL f_Time3s;
#endif
///000
extern uint16_t CntPeriodFid;
extern uint16_t CntReadFid;
extern BOOL f_ReadFid;
extern BOOL f_PeriodFid;
#ifdef CONFIG_FID
	#define TIMER_CALIB_FID 	(5)
	extern BOOL f_TimeCalibFid;
	extern BOOL f_TimeCalibFidStart;
	extern uint16_t CntCalibFid;
#endif

#ifdef CONFIG_PI
	extern 	uint8_t flag_1ms;
#endif

extern int32_t CntSec;
extern int32_t HourTimer;

#define SERVICE_TIME_MODBUS (120*2+1)
#define SEC_PER_MHOUR (60*60)

void serviceTimerStart(uint16_t );

void serviceTimerStop(void);

void Adc_Eoc_Callback(void);
void Adc_read_data(void);

extern uint16_t ADC_in[3];
extern uint8_t adc_cnt;

extern BOOL lmp_tia_or_temper;
extern BOOL f_AdcCycleEnd;
extern BOOL f_AdcDataBad;

typedef struct{

	uint32_t Counter;

	uint16_t TypeSensor;		/* Тип сенсора (Код)					*/
	uint16_t Unit;				/* Единица измерения концентрации		*/

	uint16_t ValueLow;			/* Нижнее значение 						*/
	uint16_t ValueHigh;			/* Верхнее значение: отображаемое		*/

	uint16_t ValueCalib;		/* Концентрация калибровочного газа 	*/
	uint16_t ADCLow;			/* Нижнее значение с АЦП 				*/

	uint16_t ADCHigh;			/* Верхнее значение с АЦП 				*/
	uint16_t Text[8];			/* Текстовый описатель газа ASCII		*/

	uint16_t SNum[8];			/* Заводской номер самого сенсора ASCII	*/

	uint16_t ScaleKoef;			/* Масштабный коэффициент концентрации	*/
	uint16_t ScaleADC;			/* Коэффициент усиления для АЦП			*/

	uint16_t FID;				/* Настройки управления ФИД'ом			*/
	uint16_t VerHW;				/* Версия аппаратной реализации			*/


	uint16_t LMP_Gain;			/* Настройки LMP ucRLoad & ucGain		*/
	uint16_t LMP_BiasSign;		/* Настройки LMP ucBias & ucBiasSign 	*/
	uint16_t LMP_Source;		/* Настройки LMP ucIntZ & ucSource		*/
	uint16_t LMP_Mode;			/* Настройки LMP  ucMode & ucFET		*/

	uint16_t MolarMass; 		/* Молярная масса газа					*/
	uint16_t ValueHighMeausure;	/* Верхнее значение: измеряемое			*/



	struct{
		int16_t Temp;
		uint16_t Koef;
	}temp_corr_zero[8];

	struct{
		int16_t Temp;
		uint16_t Koef;
	}temp_corr_conc[8];

	struct{
		uint16_t Conc;
		uint16_t Koef;
	}linear[16];

	uint16_t UnitKoef[6];			/* 0x1070 */

	uint16_t diskret_1_2;			/* 0x1076 */
	uint16_t diskret_3_4;			/* 0x1077 */
	uint16_t diskret_5_6;			/* 0x1078 */
	uint16_t UnitView;				/* 0x1079 */

	uint16_t CalibZeroTemper;
	uint16_t CalibZeroADC;

	uint16_t CalibConcTemper;
	uint16_t CalibConcADC;

	uint32_t Serial;

	uint32_t crc;

}Config_td;

typedef struct{

	uint16_t cod_8225;			/* 0x0000 */	/* cod_8225											*/
	uint16_t VerSW;				/* 0x0001 */	/* Код версии ПО								*/
	uint16_t VerSW_Build;		/* 0x0002 */	/* Код версии ПО. Build							*/
	uint16_t ZavNumHI;			/* 0x0003 */	/* Заводской номер HI							*/
	uint16_t ZavNumLO;			/* 0x0004 */	/* Заводской номер LO							*/
	uint16_t Value;				/* 0x0005 */	/* Концентрация									*/
//	uint16_t Status;			/* 0x0006 */	/* Статус										*/
	uint16_t Volt_3V;			/* 0x0007 */	/* Напряжение в цепи 3V							*/
	uint16_t Volt_5V;			/* 0x0008 */	/* Напряжение в цени 5V							*/
	int16_t TempSensor;			/* 0x0009 */	/* Температура *10								*/
	uint16_t Volt_Sens;			/* 0x000A */	/* Напряжение с сенсора *1000					*/
	uint16_t Volt_Bridge;		/* 0x000B */	/* PI. Напряжение моста *100					*/
	uint16_t Volt_BridgeV2;		/* 0x000C */	/* PI. Напряжение с середины моста *1000		*/
	uint16_t Volt_Sens_PI; // ??/* 0x000D */	/* PI. Напряжение на сенсоре					*/
	uint16_t Volt_Pelistor;		/* 0x000E */	/* Внутреннее опорное напряжение *10			*/
	uint16_t Volt_Heat;			/* 0x000F */	/* Напряжение подогрева *10 					*/
	uint32_t dwHours;			/* 0x0010 */	/* Счётчик времени наработки HI					*/
	//							/* 0x0011 */	/* Счётчик времени наработки LO					*/
	uint32_t dwPPM_Min;			/* 0x0012 */	/* Счётчик наработки по концентрации HI			*/
	//							/* 0x0013 */	/* Счётчик наработки по концентрации LO			*/
	uint32_t dwValue_mg_m3;		/* 0x0014 */	/* Концентрация в мг/м3	LO						*/
	//							/* 0x0015 */	/* Концентрация в мг/м3	HI						*/
	uint16_t TimeToOffHeat;		/* 0x0016 */	/* Время до конца прогрева						*/
	uint16_t CntMaxMin;			/* 0x0017 */	/* счётчик времени замера предельных значений	*/
	uint16_t ValueMin5m;		/* 0x0018 */	/* Концентрация минимальная за 5 минут			*/
	uint16_t ValueMax5m;		/* 0x0019 */	/* Концентрация максимальная за 5 минут 		*/

	uint16_t Value_0;			/* 0x001C */	/* Текущая концентрация газа до температурной коррекции */
	uint16_t ADC_0;				/* 0x001F */	/* Значение АЦП до температурной коррекции				*/
	uint16_t ADC_TK;			/* 0x0020 */	/* Значение АЦП после температурной коррекции			*/
	uint16_t TK_Mul;			/* 0x0021 */	/* Коэфициент при температурной коррекции (множитель)	*/
	uint16_t TK_Add;			/* 0x0022 */	/* Коэфициент при температурной коррекции (смещение)	*/

}RegInput_td;

typedef struct {

	Config_td Config;
	RegInput_td RegInput;

	uint16_t Status;

	BOOL f_heat;

}dev_td;

extern dev_td dev;

typedef struct{
	uint32_t Serial;

	uint32_t res;

	uint32_t crc;

}stMain_td;

//extern stMain_td stMain;

void dev_set_config_default(void);

#endif /* INC1_DEVICE_H_ */

//------------------------------------------------------------------------------
//
//
// modbus.h
//                                                					 26.08.2021
//
//------------------------------------------------------------------------------

#ifndef INC1_MODBUS_H_
#define INC1_MODBUS_H_

void mb_proc(void);

//======================================================================================================================================
// Регистр статуса 0x1000 (биты)
enum
{
/*  0 */	STATUS_BIT_MAIN_INIT = 0,		// Инициализация модуля
/*  1 */	STATUS_BIT_MAIN_RUN,			// Рабочий цикл запущен
/*  2 */	STATUS_BIT_MAIN_MODE,			// 0 - рабочий режим, 1 - сервисный
/*  3 */	STATUS_BIT_MAIN_ERROR,			// Признак наличия каких либо неисправностей
/*  4 */	STATUS_BIT_ERROR_OVER,			// Превышение сигнала
/*  5 */	STATUS_BIT_ERROR_LMP,			// Нет связи с ОУ
/*  6 */	STATUS_BIT_ERROR_STLM,			// Нет связи с датчиком температуры
/*  7 */	STATUS_BIT_ERROR_ADS,			// Нет связи с АЦП
/*  8 */	STATUS_BIT_ERROR_STU_3V,		// Питание 3.3В не в допуске
/*  9 */	STATUS_BIT_ERROR_STU_5V,		// Питание 5.0В не в допуске (внешнее)
/* 10 */	STATUS_BIT_ERROR_STU_HEAT,		// Питание нагревателя не в допуске
/* 11 */	STATUS_BIT_ERROR_STU_BRIDGE,	// Питание измерительного моста не в допуске
/* 12 */	STATUS_BIT_NO_SENSOR,			// Нет подключенного сенсора либо сенсор повреждён
/* 13 */	STATUS_BIT_ERROR_AT25_TOUT,		// Проблемы с памятью AT25
/* 14 */	STATUS_BIT_SENSOR_ERROR,		// Неисправности в самом сенсоре
/* 15 */	STATUS_BIT_FID_PWR				// Питание ФИД включено
};


// Типы возможных сенсоров( старший байт регистра wTypeSensor 0x1002 )
enum
{
	/* 0 */	SENSOR_TYPE_NO = 0,
	/* 1 */	SENSOR_TYPE_EL,			// Электрохимия 3P
	/* 2 */	SENSOR_TYPE_O2,			// Электрохимия O2 2P
	/* 3 */	SENSOR_TYPE_PI,			// Термокатализ
	/* 4 */	SENSOR_TYPE_MX,			// ИК. MIPEX
	/* 5 */	SENSOR_TYPE_FID,		// ФИД
	/* 6 */	SENSOR_TYPE_DY,			// ИК. DYNOMENT
	/* 7 */	SENSOR_TYPE_BASICevo,	// ИК. Basic EVO
	/* 8 */	SENSOR_TYPE_SGX,		// ИК. SGX
};

// Единицы измерения ( старший байт регистра wUnit 0x1003 )
enum
{
	CFG_UNIT_VALUE_vol = 0,		// "%Vol" / "%об.д."
	CFG_UNIT_VALUE_ppm,			// "ppm"
	CFG_UNIT_VALUE_ppb,			// "ppb"
	CFG_UNIT_VALUE_lel,			// "%LEL" / "%НКПР"
	CFG_UNIT_VALUE_g_cm3,		// "г/см3"
	CFG_UNIT_VALUE_ug_m3,		// "мкг/м3"
};

// Младший байт регистра wUnit 0x1003 содержит дискретность концентрации (количество знаков после точки) 0-3

extern uint16_t AccessCode;
extern BOOL mbUnlock;
extern BOOL mbServiceMode;
extern uint16_t mbHoldDevStatus;


#endif /* INC1_MODBUS_H_ */

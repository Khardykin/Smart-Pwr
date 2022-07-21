#ifndef _FILTER_MIDDLE_H_
#define _FILTER_MIDDLE_H_

//===========================================================================================================================
#define FIL_MIDDLE_COUNT_STAGE		6
typedef struct
{
	int32_t iMiddle[ FIL_MIDDLE_COUNT_STAGE ];		// Массив отсчётов АЦП
	int32_t iOldVal;								// Предыдущее расчётчное значение
} type_filter_middle;


// Инициализация фильтра
extern void	adcFilterMiddleInit(type_filter_middle *pFilter);

// Вычисление фильтра
extern int32_t	adcFilterMiddleRun(type_filter_middle *pFilter, int32_t iNewVal);
//===========================================================================================================================

extern type_filter_middle stFilterSensor;
extern type_filter_middle stFilterSensorisrs;
//===========================================================================================================================
#endif

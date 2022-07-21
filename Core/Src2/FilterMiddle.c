#include "main.h"
#include "FilterMiddle.h"
//======================================================================================================================================
type_filter_middle stFilterSensor;
type_filter_middle stFilterSensorisrs;
//======================================================================================================================================
//======================================================================================================================================
//======================================================================================================================================
/* 		Алгоритм фильтрации
 * 1) Массив iMiddle заполняем новыми значениями iNewVal. С каждым запросом сдвигаем массив влево.
 * 2) Вычисляем среднее значение
 * 3) Вычисляем разницу СР-iOldVal;
 *    - если положительная, то возможно идёт возростание функции;
 *    - если отрицательная, то возможно идёт убывание функции.
 * 4) Выборка результата
 *    Разница положительная:
 * 	  - выбираем результат из массива в диапазоне iOldVal до СР, первое ближайщее к iOldVal.
 * 	  - если значение в масстве отсутствует, выбираем среднее из iOldVal и среднегго значения.
 *    Разница отрицательная:
 * 	  - выбираем результат из массива в диапазоне СР до iOldVal, первое ближайщее к iOldVal.
 * 	  - если значение в масстве отсутствует, выбираем среднее из iOldVal и среднегго значения.
 *    Разница нулевае:
 * 	  - выбираем iOldVal.
 */
//======================================================================================================================================
static int32_t __round(int32_t iVal)
{
	if( iVal % 10 > 5 )	return( iVal / 10 + 1 );
	return( iVal / 10 );
}
//======================================================================================================================================
// Инициализация фильтра
void adcFilterMiddleInit(type_filter_middle *pFilter)
{
	for(uint16_t i = 0; i < sizeof(type_filter_middle); i++){
		*(uint8_t*)&pFilter[i] = 0;
	}
//	memset( (uint8_t*)pFilter, 0, sizeof(type_filter_middle) );
}
//======================================================================================================================================
// Вычисление фильтра
int32_t adcFilterMiddleRun(type_filter_middle *pFilter, int32_t iNewVal)
{
	int32_t rezult = pFilter->iOldVal;
	int32_t middle = 0;
	int32_t prez;
	int i;
	uint8_t b;
	
	// Сдвигаем массив и вычисляем среднее значение
	for( i=1; i < FIL_MIDDLE_COUNT_STAGE; i++ )
	{
		pFilter->iMiddle[i-1] = pFilter->iMiddle[i];
		middle += pFilter->iMiddle[i];
	}
	pFilter->iMiddle[FIL_MIDDLE_COUNT_STAGE-1] = iNewVal;
	middle += iNewVal;
	middle = middle * 10 / FIL_MIDDLE_COUNT_STAGE;
	middle = __round( middle );
	prez = middle;

	// Вычисляем разницу
	middle -= pFilter->iOldVal;

	// Делаем выборку
	if( middle > 0 )
	{
		b = DISABLE;
		for( i=1; i < FIL_MIDDLE_COUNT_STAGE; i++ )
		{
			if( ( pFilter->iMiddle[i] > pFilter->iOldVal ) && ( pFilter->iMiddle[i] < prez ) )
			{
				prez = pFilter->iMiddle[i];
				b = ENABLE;
			}
		}
		//rezult = (b == TRUE) ? prez : pFilter->iOldVal;
		rezult = (b == ENABLE) ? prez : ((pFilter->iOldVal + prez)/2);
	} else
	if( middle < 0 )
	{
		b = DISABLE;
		for( i=1; i < FIL_MIDDLE_COUNT_STAGE; i++ )
		{
			if( ( pFilter->iMiddle[i] < pFilter->iOldVal ) && ( pFilter->iMiddle[i] > prez ) )
			{
				prez = pFilter->iMiddle[i];
				b = ENABLE;
			}
		}
		//rezult = (b == TRUE) ? prez : pFilter->iOldVal;
		rezult = (b == ENABLE) ? prez : ((pFilter->iOldVal + prez)/2);
	}

	pFilter->iOldVal = rezult;
	return( rezult );
}
//======================================================================================================================================
//======================================================================================================================================
//======================================================================================================================================



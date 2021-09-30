#ifndef __MSI_MEASUREMENT_H
#define __MSI_MEASUREMENT_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define USE_REFERENCE_LSE

//#define HAL_CONFIG_TIM
#define LL_CONFIG_TIM

#define CAPTURE_START        			((uint32_t) 0x00000001)
#define CAPTURE_ONGOING      			((uint32_t) 0x00000002)
#define CAPTURE_COMPLETED    			((uint32_t) 0x00000003)

#ifdef HAL_CONFIG_TIM
	#define __TIMx_CLK_ENABLE()  			__HAL_RCC_TIM21_CLK_ENABLE()
	#define	TIM_CHANNEL_y							TIM_CHANNEL_1
	#define	HAL_TIM_ACTIVE_CHANNEL_y 	HAL_TIM_ACTIVE_CHANNEL_1
#endif
#ifdef LL_CONFIG_TIM
	#define	TIM_CHANNEL_y							LL_TIM_CHANNEL_CH1
#endif

#define	TIMx 									TIM21
#define	TIM_TIMx_GPIO							TIM21_TI1_GPIO
#define	TIM_TIMx_LSE							TIM21_TI1_LSE
#define	TIM_TIMx_MCO							TIM21_TI1_MCO
#define	TIMx_IRQn								TIM21_IRQn
#define INITIAL_ERROR							((uint32_t)99999000)
#ifdef HAL_CONFIG_TIM
	#define __HAL_GET_TIM_PRESCALER(__HANDLE__)       ((__HANDLE__)->Instance->PSC)
#endif
#define ABS_RETURN(x)         					((x < 0) ? (-x) : (x))

/* Exported functions ------------------------------------------------------- */
extern void 		MSI_MeasurementInit(void);
extern uint32_t 	MSI_FreqMeasure(void);
extern uint32_t 	MSI_CalibrateMinError(void);
extern ErrorStatus 	MSI_CalibrateFixedError(uint32_t MaxAllowedError, uint32_t* Freq);
extern ErrorStatus 	MSI_CalibrateCurve(uint32_t* Freq);
extern void 		MSI_GetCurve(void);
extern void 		MSI_Callback_Capture_Timer(void);

/* extern variables ------------------------------------------------------- */
extern uint32_t 				IC1ReadValue1;
extern uint32_t 				IC1ReadValue2;
extern uint32_t __IO 			CaptureState;
extern uint32_t __IO 			Capture;
extern uint32_t 				StartCalibration;
extern int32_t 					aFrequenceChangeTable[256];
extern uint32_t 				TrimmingCurveMeasured;
#endif
/*--------------------------------------------------------------------------*/

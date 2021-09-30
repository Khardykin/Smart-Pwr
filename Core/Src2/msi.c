#include "main.h"
#include "msi.h"
/* Private typedef -----------------------------------------------------------*/
typedef enum {
	ENABLE_CLOCK = 0,
	DISABLE_CLOCK,
}StateTurnClockPerif;
/* Private define ------------------------------------------------------------*/

#define MSI_TIMEOUT         		((uint32_t)0xFFFFFF)
/* Get actual trimming settings of MSI */
#define GET_MSI_TRIMMING_VALUE()	((RCC->ICSCR & RCC_ICSCR_MSITRIM) >> 24)

#ifdef USE_REFERENCE_LSE
	#define MSI_TIMx_COUNTER_PRESCALER  ((uint32_t)0)

	/* The LSE is divided by 8 => LSE/8 = 32768/8 = 4096 */
	#define MSI_REFERENCE_FREQUENCY     ((uint32_t)4096) /* The reference frequency value in Hz */

	#define MSI_NUMBER_OF_LOOPS       	((uint32_t)50)

#endif /* USE_REFERENCE_LSE */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t __IO 	CaptureState = 0;
uint32_t __IO 	Capture = 0;
uint32_t  		StartCalibration = 0;
int32_t   		aFrequenceChangeTable[256]; /* 2^8 positions for MSI */
uint32_t  		TrimmingCurveMeasured;

uint32_t  		IC1ReadValue1 = 0;
uint32_t		IC1ReadValue2 = 0;
/* Private function prototypes -----------------------------------------------*/
static void 		MSI_TurnClock(StateTurnClockPerif StateTurnClock);
static void 		MSI_RCC_AdjustCalibrationValue(uint8_t TrimmingValue);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Calibrates internal oscillators MSI to the minimum computed error.
  * @param  None.
  * @retval The optimum computed frequency of MSI oscillator.
  */
uint32_t MSI_CalibrateMinError(void)
{
  uint32_t  measuredfrequency = 0;
  uint32_t  sysclockfrequency = 0;
  uint32_t  optimumfrequency = 0;
  uint32_t  frequencyerror = 0;
  uint32_t  optimumfrequencyerror = INITIAL_ERROR; /* Large value */
  uint32_t  numbersteps = 0;         /* Number of steps: size of trimming bits */
  uint32_t  trimmingvalue = 0;
  uint32_t  optimumcalibrationvalue = 0;

  /* Set measurement environment */
  MSI_MeasurementInit();

  /* Get system clock frequency */
  LL_RCC_ClocksTypeDef RCC_Clocks;
  LL_RCC_GetSystemClocksFreq(&RCC_Clocks);
  sysclockfrequency = RCC_Clocks.SYSCLK_Frequency;

  if (StartCalibration != 0)
  {
    /* MSITRIM is 8-bit length */
    numbersteps = 256; /* number of steps is 2^8 = 256 */
  }
  else
  {
    /* Without Calibration */
    numbersteps = 1;
  }

  /* Internal Osc frequency measurement for numbersteps */
  for (trimmingvalue = 0; trimmingvalue < numbersteps; trimmingvalue++)
  {
    if (StartCalibration != 0)
    {
      /* Set the Intern Osc trimming bits to trimmingvalue */
      MSI_RCC_AdjustCalibrationValue(trimmingvalue);
    }

    /* Get actual frequency value */
    measuredfrequency = MSI_FreqMeasure();

    if (StartCalibration != 0)
    {
      /* Compute current frequency error corresponding to the current trimming value:
      measured value is subtracted from the typical one */
      frequencyerror = ABS_RETURN((int32_t) (measuredfrequency - sysclockfrequency));

      /* Get the nearest frequency value to typical one */
      if (optimumfrequencyerror > frequencyerror)
      {
        optimumfrequencyerror = frequencyerror;
        optimumcalibrationvalue = trimmingvalue;
        optimumfrequency = measuredfrequency;
      }
    }
  }

  if (StartCalibration != 0)
  {
    /* Set trimming bits corresponding to the nearest frequency */
    MSI_RCC_AdjustCalibrationValue(optimumcalibrationvalue);
    /* Return the intern oscillator frequency after calibration */
    return (optimumfrequency);
  }
  else
  {
    /* Return the intern oscillator frequency before calibration */
    return (measuredfrequency);
  }
}

/**
  * @brief  Calibrates the internal oscillator with the maximum allowed
  *         error value set by user.
  *         If this value was not found, this function sets the oscillator
  *         to default value.
  * @param  MaxAllowedError: maximum absolute value allowed of the MSI frequency
  *                          error given in Hz.
  * @param  Freq: pointer to an uint32_t variable that will contain the value
  *               of the internal oscillator frequency after calibration.
  * @retval ErrorStatus:
  *            - SUCCESS: a frequency error =< MaxAllowedError was found.
  *            - ERROR: a frequency error =< MaxAllowedError was not found.
  */
ErrorStatus MSI_CalibrateFixedError(uint32_t MaxAllowedError, uint32_t* Freq)
{
  uint32_t  measuredfrequency = 0;
  int32_t  	frequencyerror = 0;
  uint32_t 	absfrequencyerror = 0;
  uint32_t  sysclockfrequency = 0;
  uint32_t 	max = 0xFF; 	/* number of steps is 2^8 = 256 */
  uint32_t 	min = 0x00;
  uint32_t 	mid = 0x00;	
  uint32_t  trimmingindex = 0;
  uint32_t  trimmingvalue = 16;
  ErrorStatus calibrationstatus = ERROR;
  uint32_t  numbersteps = 256;         /* number of steps is 2^8 = 256 */

  MSI_TurnClock(DISABLE_CLOCK);
  /* Set measurement environment */
  MSI_MeasurementInit();

  /* Get system clock frequency */
  LL_RCC_ClocksTypeDef RCC_Clocks;
  LL_RCC_GetSystemClocksFreq(&RCC_Clocks);
  sysclockfrequency = RCC_Clocks.SYSCLK_Frequency;

  /* Set the MSITRIMR register to trimmingvalue to be ready for measurement */
  MSI_RCC_AdjustCalibrationValue(numbersteps / 2);
  /****************************************************************************/
  /* RC Frequency measurement for different values */
  while ((trimmingindex < 256) && (calibrationstatus == ERROR))
  {
		
    /* Compute the middle */
    mid = ((max + min) >> 1);
    trimmingvalue = mid;
    /* Set the MSITRIM bits to trimmingvalue to be ready for measurement */
    MSI_RCC_AdjustCalibrationValue(trimmingvalue);

    /* Start frequency measurement for current trimming value */
    measuredfrequency = 0;
		
    /* Get actual frequency value */
    measuredfrequency = MSI_FreqMeasure();

    /* Compute current frequency error corresponding to the current trimming value */
    frequencyerror = (measuredfrequency - sysclockfrequency);
    absfrequencyerror = ABS_RETURN(frequencyerror);

    if (absfrequencyerror < MaxAllowedError)
    {
      /* Calibration succeeded */
      calibrationstatus = SUCCESS;

      /* Save the MSI measured value */
      *Freq = measuredfrequency;
    }
    else if (frequencyerror < 0)
    {
      /* Update the minimum */
      min = mid;
    }
    else
    {
      /* Update the maximum */
      max = mid;
    }

    /* Increment trimming index */
    trimmingindex++;
		
  }

  /* If the frequency error set by the user was not found */
  if (calibrationstatus == ERROR)
  {
    /* Set the MSITRIMR register to default value */
	  MSI_RCC_AdjustCalibrationValue(numbersteps / 2);
  }

  MSI_TurnClock(ENABLE_CLOCK);
  /* Return the calibration status: ERROR or SUCCESS */
  return (calibrationstatus);
}

/**
  * @brief  For all possible trimming values change of frequency is measured
  * @retval None.
  */
void MSI_GetCurve(void)
{
  uint32_t measuredfrequency;
  uint32_t trimmingindex = 0;
  uint32_t trimmingindexorig;
  uint32_t orig_frequency;
  uint32_t numbersteps;
//uint8_t txbuffer[40];
//uint16_t len;
  /* Set measurement environment */
  MSI_MeasurementInit();

  if (StartCalibration != 0)
  {
    /* MSITRIM is 8-bit length */
    numbersteps = 256; /* number of steps is 2^8 = 256 */
  }
  else
  {
    /* Without Calibration */
    numbersteps = 1;
  }

  /* Keep original values */
  trimmingindexorig = GET_MSI_TRIMMING_VALUE();
  orig_frequency = MSI_FreqMeasure();

  /* RC Frequency measurement for different values */
  for (trimmingindex = 0; trimmingindex < numbersteps; trimmingindex++)
  {

    /* Set the MSITRIMR register to trimmingvalue to be ready for measurement */
		MSI_RCC_AdjustCalibrationValue(trimmingindex);
    /************ Start measuring Internal Oscillator frequency ***************/
    measuredfrequency = MSI_FreqMeasure();

    /* Compute current frequency error corresponding to the current trimming value:
      measured value is subtracted from the typical one */
    aFrequenceChangeTable[trimmingindex] =   ((int32_t)(measuredfrequency - orig_frequency));
//    len = sprintf((char*)txbuffer, "%d \n\r", aFrequenceChangeTable[trimmingindex]);
//    HAL_UART_Transmit(&huart2, txbuffer, len, 30);
  }

  if (TrimmingCurveMeasured == 0)
  {
    TrimmingCurveMeasured = 1;
  }

  MSI_RCC_AdjustCalibrationValue(trimmingindexorig);
}

/**
  * @brief  Adjust calibration value (writing to trimming bits) of selected oscillator.
  * @param  Freq: pointer to an uint32_t variable that will contain the value
  *               of the internal oscillator frequency after calibration.
  * @retval ErrorStatus:
  *            - SUCCESS: a frequency correction value was found.
  *            - ERROR: a frequency correction value was not found.
  */
ErrorStatus MSI_CalibrateCurve(uint32_t* Freq)
{
  uint32_t measuredfrequency;
  uint32_t optimumcalibrationvalue;
  uint32_t i;
  uint32_t frequencyerror;
  uint32_t size;
  uint32_t optimumfrequencyerror = INITIAL_ERROR; /* Large value */
  ErrorStatus returnvalue = ERROR;

  optimumcalibrationvalue = GET_MSI_TRIMMING_VALUE();
  size = 32;

  /* Get position */
  measuredfrequency = MSI_FreqMeasure();

  /* Find the closest difference */
  for (i = 0; i < size; i++)
  {
    frequencyerror = ABS_RETURN((int32_t) (MSI_VALUE - (int32_t)(measuredfrequency + aFrequenceChangeTable[i])));

    /* Get the nearest frequency value to typical one */
    if (frequencyerror < optimumfrequencyerror)
    {
      optimumfrequencyerror = frequencyerror;
      optimumcalibrationvalue = i;
    }
  }

  if (optimumcalibrationvalue < size)
  {
	  MSI_RCC_AdjustCalibrationValue(optimumcalibrationvalue);
    /* Save the MSI measured value */
    *Freq = measuredfrequency + aFrequenceChangeTable[optimumcalibrationvalue];
    return returnvalue = SUCCESS;
  }

  return returnvalue;
}

/**
  * @brief Measures actual value of MSI
  * @param  None.
  * @retval Actual MSI frequency
  */
uint32_t MSI_FreqMeasure(void)
{
  uint32_t  measuredfrequency;
  uint8_t  loopcounter = 0;
  uint32_t  timeout = MSI_TIMEOUT;

  /* Start frequency measurement for current trimming value */
  measuredfrequency = 0;
  loopcounter = 0;
  /************ Start measuring Internal Oscillator frequency ***************/
  while (loopcounter <= MSI_NUMBER_OF_LOOPS)
  {
    CaptureState = CAPTURE_START;

    /* Enable capture 1 interrupt */
#ifdef HAL_CONFIG_TIM
    HAL_TIM_IC_Start_IT(&TimHandle, TIM_CHANNEL_y);
#endif
#ifdef LL_CONFIG_TIM
    LL_TIM_EnableIT_CC1(TIMx);
    LL_TIM_CC_EnableChannel(TIMx, TIM_CHANNEL_y); //LL_TIM_CHANNEL_CH1
    LL_TIM_EnableCounter(TIMx);
#endif
    /* Enable the TIMx IRQ channel */
    NVIC_EnableIRQ(TIMx_IRQn);

    /* Wait for end of capture: two consecutive captures */
    while (CaptureState != CAPTURE_COMPLETED)
    {
      if (--timeout == 0)
      {
        return ERROR;
      }
    }

    /* Disable IRQ channel */
    NVIC_DisableIRQ(TIMx_IRQn);

    /* Disable TIMx */
#ifdef HAL_CONFIG_TIM
    HAL_TIM_IC_Stop_IT(&TimHandle, TIM_CHANNEL_y);
#endif
#ifdef LL_CONFIG_TIM
    LL_TIM_DisableIT_CC1(TIMx);
    LL_TIM_CC_DisableChannel(TIMx, TIM_CHANNEL_y); //LL_TIM_CHANNEL_CH1
    LL_TIM_DisableCounter(TIMx);
    LL_TIM_ClearFlag_CC1(TIMx);
#endif

    if (loopcounter != 0)
    {
      /* Compute the frequency (the Timer prescaler isn't included) */
      measuredfrequency += (uint32_t) (MSI_REFERENCE_FREQUENCY * Capture);
    }

    /* Increment loop counter */
    loopcounter++;
  }
  /**************************** END of Measurement **************************/

  /* Compute the average value corresponding the the current trimming value */
#ifdef HAL_CONFIG_TIM
  measuredfrequency = (uint32_t)((__HAL_GET_TIM_PRESCALER(&TimHandle) + 1) * (measuredfrequency / MSI_NUMBER_OF_LOOPS));
#endif
#ifdef LL_CONFIG_TIM
  measuredfrequency = (uint32_t)((LL_TIM_GetPrescaler(TIMx) + 1) * (measuredfrequency / MSI_NUMBER_OF_LOOPS));

#endif
  return measuredfrequency;
}

/**
  * @brief Configures all the necessary peripherals necessary from frequency calibration.
  * @param  None.
  * @retval None.
  */
void MSI_MeasurementInit(void)
{
	StartCalibration = 1;
#ifdef HAL_CONFIG_TIM
	TimHandle = htim21;
#endif
}

// Включение и выключение периферии
static void MSI_TurnClock(StateTurnClockPerif StateTurnClock)
{
#ifdef LL_CONFIG_TIM
	if(StateTurnClock == ENABLE_CLOCK){
		LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
		LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
		LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);

		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
//		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM22);

		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
	}
	else if(StateTurnClock == DISABLE_CLOCK){
		LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOA);
		LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOB);
		LL_IOP_GRP1_DisableClock(LL_IOP_GRP1_PERIPH_GPIOC);

		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);

		LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_ADC1);
//		LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM22);

		LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_CRC);
	}
#endif
#ifdef HAL_CONFIG_TIM

#endif
}



/**
  * @brief  Adjust calibration value (writing to trimming bits) of selected oscillator.
  * @param  TrimmingValue: calibration value to be written in trimming bits.
  * @retval None.
  */
static void MSI_RCC_AdjustCalibrationValue(uint8_t TrimmingValue)
{
	LL_RCC_MSI_SetCalibTrimming(TrimmingValue);
//  __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(TrimmingValue);
}

// Обработчик прерывания таймера
void MSI_Callback_Capture_Timer(void)
{
#ifdef LL_CONFIG_TIM
  if(LL_TIM_IsActiveFlag_CC1(TIMx) == 1)
  {
  	LL_TIM_ClearFlag_CC1(TIMx);
  }
  if (CaptureState == CAPTURE_START)
  {
	  /* Get the 1st Input Capture value */
	  IC1ReadValue1 = LL_TIM_IC_GetCaptureCH1(TIMx);
	  CaptureState = CAPTURE_ONGOING;
  }
  else if (CaptureState == CAPTURE_ONGOING)
  {
	  /* Get the 2nd Input Capture value */
	  IC1ReadValue2 = LL_TIM_IC_GetCaptureCH1(TIMx);

	  /* Capture computation */
	  if (IC1ReadValue2 > IC1ReadValue1)
	  {
		  Capture = (IC1ReadValue2 - IC1ReadValue1);
	  }
	  else if (IC1ReadValue2 < IC1ReadValue1)
	  {
		  Capture = ((0xFFFF - IC1ReadValue1) + IC1ReadValue2);
	  }
	  else
	  {
		  Error_Handler();
	  }
	  CaptureState = CAPTURE_COMPLETED;
  }
#endif
}


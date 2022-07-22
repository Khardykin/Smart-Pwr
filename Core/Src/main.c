/* USER CODE BEGIN Header */
//===============================================================================
//
//
//
//
//	main.c
//                                                   				  03.09.2021
//
//==============================================================================

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "crc.h"
#include "i2c.h"
#include "lptim.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tim.h"
#include "defines.h"
//#include "string.h"
#include "debug.h"
#include "lmp91000.h"
#include "ADS1115.h"
#include "device.h"
#include "modbus_lpuart.h"
#include "modbus.h"
#include "Mipex_command.h"
#include "eeprom.h"
#include "flash.h"
#include "arhiv.h"				  
#include "calculations.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

char date[13] = { __DATE__ };
char time[9] = { __TIME__ };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void printUID(void){
#ifdef DEBUG_MY
	uint32_t *idBase = (uint32_t*)(UID_BASE);
	uint8_t s[4];
	int i;
	//memcpy(s0,(uint8_t*)idBase0,3);
	for(i = 0; i < 3; i++)
		s[2-i] = (*idBase >> (8 * i)) & 0xff;
	s[3]= 0;

	uint32_t *idBase2 = (uint32_t*)(UID_BASE + 0x04);
	uint8_t s2[5];

	for(i = 0; i < 4; i++)
		s2[3-i] = (*idBase2 >> (8 * i)) & 0xff;
	s2[4]= 0;

	idBase = (uint32_t*)(UID_BASE + 0x14);

	d_printf("\n\rUID %02X-\"%s\"-\"%s\"-%08lx", (*idBase >> 24) & 0xff, s, s2, *idBase2);
#endif
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM22_Init();
  MX_CRC_Init();
  MX_TIM21_Init();
  MX_LPTIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

#ifdef DEBUG_MY
  	debug_init();
	d_printf("\n\r\n\r%s %s", date, time);
	printUID();
#endif

	//LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR);
	ADC1_COMMON->CCR |= LL_ADC_PATH_INTERNAL_TEMPSENSOR;

	LL_ADC_StartCalibration(ADC1);
	while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0);
	LL_mDelay(2);

	read_config_from_eeprom();
	dev_init();

#ifdef DEBUG_MY
	d_printf("\n\rSN %09lu", dev.Config.Serial);
	d_printf("\n\r");
#endif

	//  test_temp_korr();
#ifdef CONFIG_PI
	SET_TURN_ON;
	LL_mDelay(500);
#endif
	modbus_init();
	///000
#ifdef CONFIG_EC
	LMP_Init();
	LMP_Set_Mode(MODE_TEMPERAT);
#endif
	///000
#if defined(CONFIG_PI) || defined(CONFIG_FID)
	ADS_Init(dev.Config.FID);
#endif
	LL_TIM_EnableCounter(TIM2);

	LL_ADC_Enable(ADC1);
	LL_ADC_EnableIT_EOC(ADC1);

	LL_TIM_EnableIT_CC1(TIM22);

	LL_LPTIM_Enable(LPTIM1);
	// От LSE 32.768 (прескалер 128) с делением на 2
	LL_LPTIM_SetAutoReload(LPTIM1, 1);
	LL_LPTIM_EnableIT_ARRM(LPTIM1);
	LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

	HourTimer = CntSec;

	///000
#ifdef CONFIG_MIPEX
	Mipex_Init();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		mb_proc();
#ifdef CONFIG_PI
		// Прогрев и включение термокаталики
		heat_proc();
#endif
		if(f_Time500ms){

			f_Time500ms = FALSE;

			dev_proc();
			///000
#ifdef CONFIG_EC
			lmp_proc();
#endif
		}

		if(f_Time250ms){

			f_Time250ms = FALSE;

#ifdef DEBUG_ADC_TIME
#ifdef DEBUG_MY
			if(!lmp_tia_or_temper)
				d_printf("\n");
			else
				d_printf("\r");
#endif
#endif

#ifdef CONFIG_PI
			if(!(dev.Status & (1 << STATUS_BIT_MAIN_INIT))){
				LL_ADC_REG_StartConversion(ADC1);
			}
#else
			LL_ADC_REG_StartConversion(ADC1);
#endif

		}

		if(f_AdcCycleEnd){

			f_AdcCycleEnd = FALSE;
#ifdef CONFIG_EC
			if(!f_AdcDataBad)
				Adc_read_data();
			f_AdcDataBad = FALSE;
#else
			Adc_read_data();
#endif
		}

		if((uint32_t)(CntSec - HourTimer) >= SEC_PER_MHOUR){

			HourTimer = CntSec;

			arh.MHour++;

			if((arh.MHour % 24) == 0){
				DayArhivStore();
			}


		}


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_5);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_PWR_EnableBkUpAccess();
  LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
  LL_RCC_LSE_Enable();

   /* Wait till LSE is ready */
  while(LL_RCC_LSE_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
  {

  }

  LL_Init1msTick(2097000);

  LL_SetSystemCoreClock(2097000);
  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
  LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
  LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

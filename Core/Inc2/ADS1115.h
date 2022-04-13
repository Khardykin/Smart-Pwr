/*
 * ADS1115.h
 *
 *  Created on: Sep 7, 2021
 *      Author: Khardykin
 */

#ifndef INC1_ADS1115_H_
#define INC1_ADS1115_H_
//==============================================================================
#define I2C_TO 			(1)

#define ADS_PORT 		I2C1
#define ADS_I2C_ADDRESS (0x48 << 1)

#define ADS_CONVERSION_REG                     	(0x00)
#define ADS_CONFIG_REG                         	(0x01)
#define ADS_LO_TH_REG                          	(0x10)
#define ADS_HI_TH_REG                          	(0x11)

#define ADS_CONFIG_REG_OS_MASK 					(0x8000) // Mask
#define ADS_CONFIG_REG_OS_SINGLE  				(0x8000) // W:Start a single conversion
#define ADS_CONFIG_REG_OS_BUSY 					(0x0000) // R:Device is currently performing a conversion
#define ADS_CONFIG_REG_OS_NOTBUSY  				(0x8000) // R: Device is not currently performing a conversion

#define ADS_CONFIG_REG_PGA_MASK 				(0x0E00) // Mask
#define ADS_CONFIG_REG_PGA_6_144V 				(0x0000) // +/-6.144V range = Gain 2/3
#define ADS_CONFIG_REG_PGA_4_096V 				(0x0200) // +/-4.096V range = Gain 1
#define ADS_CONFIG_REG_PGA_2_048V   			(0x0400) // +/-2.048V range = Gain 2 (default)
#define ADS_CONFIG_REG_PGA_1_024V 				(0x0600) // +/-1.024V range = Gain 4
#define ADS_CONFIG_REG_PGA_0_512V 				(0x0800) // +/-0.512V range = Gain 8
#define ADS_CONFIG_REG_PGA_0_256V 				(0x0A00) // +/-0.256V range = Gain 16

#define ADS_CONFIG_REG_MUX_MASK 				(0x7000)
#define ADS_CONFIG_REG_MUX_DIF_0_1 				(0x0000) // AINP = AIN0 and AINN = AIN1 (default)
#define ADS_CONFIG_REG_MUX_DIF_0_3 				(0x1000) // AINP = AIN0 and AINN = AIN3
#define ADS_CONFIG_REG_MUX_DIF_1_3 				(0x2000) // AINP = AIN1 and AINN = AIN3
#define ADS_CONFIG_REG_MUX_DIF_2_3 				(0x3000) // AINP = AIN2 and AINN = AIN3
#define ADS_CONFIG_REG_MUX_DIF_0_N 				(0x0000) // AINP = AIN0 and AINN = GND
#define ADS_CONFIG_REG_MUX_DIF_1_N 				(0x1000) // AINP = AIN0 and AINN = GND
#define ADS_CONFIG_REG_MUX_DIF_2_N 				(0x2000) // AINP = AIN1 and AINN = GND
#define ADS_CONFIG_REG_MUX_DIF_3_N 				(0x3000) // AINP = AIN2 and AINN = GND

#define ADS_CONFIG_REG_MODE_MASK 				(0x0100) // Mask
#define ADS_CONFIG_REG_MODE_CONTIN 				(0x0000) // Continuous-conversion mode
#define ADS_CONFIG_REG_MODE_SINGLE  			(0x0100) // Single-shot mode or power-down state (default)

#define ADS_CONFIG_REG_RATE_MASK 				(0x00E0) // Data Rate Mask
#define ADS_CONFIG_REG_RATE_8SPS 				(0x0000) // 8 samples per second
#define ADS_CONFIG_REG_RATE_16SPS 				(0x0020) // 16 samples per second
#define ADS_CONFIG_REG_RATE_32SPS 				(0x0040) // 32 samples per second
#define ADS_CONFIG_REG_RATE_64SPS 				(0x0060) // 64 samples per second
#define ADS_CONFIG_REG_RATE_128SPS 				(0x0080) // 128 samples per second (default)
#define ADS_CONFIG_REG_RATE_250SPS 				(0x00A0) // 250 samples per second
#define ADS_CONFIG_REG_RATE_475SPS 				(0x00C0) // 475 samples per second
#define ADS_CONFIG_REG_RATE_860SPS 				(0x00E0) // 860 samples per second

#define ADS_CONFIG_REG_CMODE_MASK 				(0x0010) // Mask
#define ADS_CONFIG_REG_CMODE_TRAD       		(0x0000) // Traditional comparator with hysteresis (default)
#define ADS_CONFIG_REG_CMODE_WINDOW 			(0x0010) // Window comparator

#define ADS_CONFIG_REG_CPOL_MASK 				(0x0008) // Mask
#define ADS_CONFIG_REG_CPOL_ACTVLOW  			(0x0000) // ALERT/RDY pin is low when active (default)
#define ADS_CONFIG_REG_CPOL_ACTVHI    			(0x0008) // ALERT/RDY pin is high when active

#define ADS_CONFIG_REG_CLAT_MASK  				(0x0004) // Determines if ALERT/RDY pin latches once asserted
#define ADS_CONFIG_REG_CLAT_NONLAT  			(0x0000) // Non-latching comparator (default)
#define ADS_CONFIG_REG_CLAT_LATCH 				(0x0004) // Latching comparator

#define ADS_CONFIG_REG_CQUE_MASK 				(0x0003) // Mask
#define ADS_CONFIG_REG_CQUE_1CONV   			(0x0000) // Assert ALERT/RDY after one conversions
#define ADS_CONFIG_REG_CQUE_2CONV  				(0x0001) // Assert ALERT/RDY after two conversions
#define ADS_CONFIG_REG_CQUE_4CONV       		(0x0002) // Assert ALERT/RDY after four conversions
#define ADS_CONFIG_REG_CQUE_NONE   				(0x0003) // Disable the comparator and put ALERT/RDY in high state (default)
//==============================================================================
typedef enum {
  GAIN_TWOTHIRDS 	= ADS_CONFIG_REG_PGA_6_144V,
  GAIN_ONE 			= ADS_CONFIG_REG_PGA_4_096V,
  GAIN_TWO 			= ADS_CONFIG_REG_PGA_2_048V,
  GAIN_FOUR 		= ADS_CONFIG_REG_PGA_1_024V,
  GAIN_EIGHT 		= ADS_CONFIG_REG_PGA_0_512V,
  GAIN_SIXTEEN 		= ADS_CONFIG_REG_PGA_0_256V
} adsGain_t;
//==============================================================================
extern void 		ADS_Init(uint16_t gain);							// Инициализация
extern uint16_t 	ADS_Read_adc(uint16_t gain);				    	// Чтение значения ацп
extern uint16_t 	ADS_Read_Diff(uint16_t data, uint16_t gain);		// Чтение аналогового дифференциального вывода
extern uint16_t 	ADS_Read_volt(uint16_t adc_data);					// Преобразование ADC в mV

#endif /* INC1_ADS1115_H_ */

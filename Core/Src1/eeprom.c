//===============================================================================
//
//
//
//
//	eeprom.c
//                                                   				  17.09.2021
//
//==============================================================================

#include "main.h"
#include "defines.h"
#include "eeprom.h"
#include "crc.h"
#include "device.h"
#include "debug.h"
#include "arhiv.h"
#include "flash.h"

uint8_t nConfigBank = 0;

#define EEPROM_BASE_ADR 0x08080000U
#define EEPROM_CONFIG_ADDRESS 	0x0000
#define EEPROM_CONFIG_LEN 		sizeof(Config_td)

//=============================================================================
// Ожидание завершения операции (стирание или записи) EEPROM с таймаутом
//
void EEPROM_WaitEOP(){

	// Таймаут 10 мсек
	uint32_t Timeout = 10;

	while(READ_BIT(FLASH->SR,FLASH_SR_EOP) == 0){

		if (LL_SYSTICK_IsActiveCounterFlag())
		{
			if(Timeout-- == 0)
			{
				return;
			}
		}

	}

	FLASH->SR = FLASH_SR_EOP;

}

void writeWordToEEPROM(uint16_t address, uint32_t data) {

	if(*(__IO uint32_t *) (EEPROM_BASE_ADR + address) != data){

		*(__IO uint32_t *) (EEPROM_BASE_ADR + address) = data;

		EEPROM_WaitEOP();

	}

}

void write_mem_to_eeprom(uint16_t address, uint32_t *ptr, uint8_t len){

	int i;

	for(i=0;i<len;i++){

		writeWordToEEPROM(address + i*4, ptr[i]);

	}

}

void eeprom_config_write_bank(uint8_t bank){

	dev.Config.crc = CalcCRC((uint32_t*)&dev.Config,(sizeof(Config_td)/4)-1);

	__enter_critical();

	FLASH->PEKEYR = (uint32_t) 0x89ABCDEFU;
	FLASH->PEKEYR = (uint32_t) 0x02030405U;

	__exit_critical();

	write_mem_to_eeprom(EEPROM_CONFIG_ADDRESS + EEPROM_CONFIG_LEN * bank,
			(uint32_t*)&dev.Config,sizeof(Config_td)/4);


	SET_BIT(FLASH->PECR, FLASH_PECR_PELOCK);

}

void eeprom_config_write(void) {

	dev.Config.Counter++;

	d_printf("\n\r");
	eeprom_config_write_bank(nConfigBank);
	d_printf("Config write");

	f_AdcDataBad = TRUE;

	nConfigBank ^= 1;


}

//=============================================================================
//
// Чтение конфигурации из EEPROM
//
void read_config_from_eeprom(void){

	uint32_t crc;
	Config_td ConfigTmp[2];
	BOOL fValid[2];

	// Чтение 2-х банков данных во временный массив структур

	for(int8_t i = 0; i < 2 * EEPROM_CONFIG_LEN / 4; i ++){

		((uint32_t*) ConfigTmp)[i] = (*(__IO uint32_t *) (EEPROM_BASE_ADR + EEPROM_CONFIG_ADDRESS + (i * 4)));
	}

	// Проверка CRC для двух банков

	for(int8_t i = 0; i < 2; i++){

		crc = CalcCRC((uint32_t*)&ConfigTmp[i], (EEPROM_CONFIG_LEN/4)-1);

		if( ConfigTmp[i].crc == crc){
			fValid[i] = TRUE;
		}else{
			fValid[i] = FALSE;
		}
	}

	// Выбор последнего банка с действительными данными

	if((fValid[0] == TRUE) && (fValid[1] == TRUE)){

		if(ConfigTmp[0].Counter < ConfigTmp[1].Counter){
			nConfigBank = 1;
		}else{
			nConfigBank = 0;
		}

	}else if((fValid[0] == FALSE) && (fValid[1] == TRUE)){
		nConfigBank = 1;
	}else{
		nConfigBank = 0;
	}

	// Если данные в обоих данных недействительны сохранение
	// в первый банк EEPROM начальных данных

	if((fValid[0] == FALSE) && (fValid[1] == FALSE)){

		if(!factory_config_read()){

			dev_set_config_default();
			d_printf("\n\r%SetDef",0);

			// Выбор первого банка
			nConfigBank = 0;

			// Запись значений в первый банк
			eeprom_config_write();

			nConfigBank = 0;

			ArhivStoreNote(ARCHIVE_SET_CONFIG_DEFAULT, 0);

		}

	}
	else{

		// Чтение данных из временной структуры в структуру dgs.Config

		for (int8_t i = 0; i < EEPROM_CONFIG_LEN/4; i ++) {

			((uint32_t*) &dev.Config)[i] = ((uint32_t*)&ConfigTmp[nConfigBank])[i];
		}

		d_printf("\n\rConfigBank-%d, CNT-%d", nConfigBank, dev.Config.Counter);
	}

	nConfigBank ^= 1;

}

//==============================================================================
// Запись заводских настроек во FLASH
//
void factory_config_write(void){

	eeprom_config_write_bank(2);

	d_printf("\n\rFactory config write");

}

//==============================================================================
// Чтение заводских настроек из FLASH
//
BOOL factory_config_read(void){

	uint32_t crc;
	Config_td ConfigTmp;

	uint32_t tmp;

	// Чтение заводских настроек во временную структуру

	for(int8_t i = 0; i < EEPROM_CONFIG_LEN / 4; i ++){

		((uint32_t*) &ConfigTmp)[i] = (*(__IO uint32_t *) (EEPROM_BASE_ADR + EEPROM_CONFIG_ADDRESS + EEPROM_CONFIG_LEN * 2 + (i * 4)));
	}

	// Проверка CRC

	crc = CalcCRC((uint32_t*)&ConfigTmp, (EEPROM_CONFIG_LEN/4)-1);

	if( ConfigTmp.crc != crc){

		d_printf("\n\rNo Data Factory",0);

		return FALSE;

	}

	d_printf("\n\rLoad Factory");

	tmp = dev.Config.Counter;

	// Чтение данных из временной структуры в структуру dev.Config

	for (int8_t i = 0; i < EEPROM_CONFIG_LEN/4; i ++) {

		((uint32_t*) &dev.Config)[i] = ((uint32_t*)&ConfigTmp)[i];
	}

	dev.Config.Counter = tmp;

	eeprom_config_write();

	return TRUE;

}


//===============================================================================
//
//
//  Модуль функций работы с FLASH памятью
//
//	flash.c
//                                                   				  10.09.2021
//
//==============================================================================

#include "main.h"
#include "defines.h"
#include "flash.h"
#include "eeprom.h"
#include "crc.h"
#include "device.h"
#include "arhiv.h"
#include "debug.h"


#define FLASH_PEKEY1               ((uint32_t)0x89ABCDEFU)
#define FLASH_PEKEY2               ((uint32_t)0x02030405U)

#define FLASH_PRGKEY1              ((uint32_t)0x8C9DAEBFU)
#define FLASH_PRGKEY2              ((uint32_t)0x13141516U)

//=============================================================================
// Снятие блокировки FLASH для стирания и записи
//
void FLASH_UNLOCK(void){

  FLASH->PEKEYR = (uint32_t) 0x89ABCDEFU;
  FLASH->PEKEYR = (uint32_t) 0x02030405U;

  if(READ_BIT(FLASH->PECR, FLASH_PECR_PELOCK)){

    __enter_critical();

    WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY1);
    WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY2);

    __exit_critical();

  }

  if (READ_BIT(FLASH->PECR, FLASH_PECR_PRGLOCK)){

    __enter_critical();

    WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY1);
    WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY2);

    __exit_critical();

  }

}

//=============================================================================
// Восстановление блокировки FLASH
//
void FLASH_LOCK(void){

  SET_BIT(FLASH->PECR, FLASH_PECR_PELOCK);
  SET_BIT(FLASH->PECR, FLASH_PECR_PRGLOCK);

}

//=============================================================================
// Ожидание освобождения шины FLASH с таймаутом
//
void FlashWaitNoBSY(void){

//  LL_GPIO_SetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);

  // Таймаут 10 мсек
  uint32_t Timeout = 10;

  while(READ_BIT(FLASH->SR,FLASH_SR_BSY) != 0){

    if (LL_SYSTICK_IsActiveCounterFlag())
    {
      if(Timeout-- == 0)
      {
 //       LL_GPIO_ResetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);
        return;
      }
    }

  }

//  LL_GPIO_ResetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);

}

//=============================================================================
// Ожидание завершения операции (стирание или записи) FLASH с таймаутом
//
void FlashWaitEOP(){

//  LL_GPIO_SetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);

  // Таймаут 10 мсек
  uint32_t Timeout = 10;

  while(READ_BIT(FLASH->SR,FLASH_SR_EOP) == 0){

    if (LL_SYSTICK_IsActiveCounterFlag())
    {
      if(Timeout-- == 0)
      {
//        LL_GPIO_ResetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);
        return;
      }
    }

  }

  FLASH->SR = FLASH_SR_EOP;
//  LL_GPIO_ResetOutputPin(DBG_2_GPIO_Port, DBG_2_Pin);

}

//=============================================================================
// Стирание страницы FLASH
//
void FlashPageErase(uint32_t *ptr){

  SET_BIT(FLASH->PECR, FLASH_PECR_ERASE);

  SET_BIT(FLASH->PECR, FLASH_PECR_PROG);

  *(__IO uint32_t *)((uint32_t)ptr & ~(FLASH_PAGE_SIZE - 1)) = 0x00000000;

  FlashWaitNoBSY();
  FlashWaitEOP();


  CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);

  CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);

}

//=============================================================================
// Запись данных во FLASH память
// *ptr - указатель на записываемую область памяти
// stat - вид записи
// data - данные записи
// Возвращает TRUE, если FLASH не стёрта и записанные
// данные помечены как недействительные
//
BOOL FlashProgram(uint32_t *ptr, uint8_t stat, uint32_t data){

  BOOL no_erase = FALSE;

  SET_BIT(FLASH->PECR, FLASH_PECR_PROG);

  if(*(__IO uint32_t *)((uint32_t)ptr) == 0){
    *(__IO uint32_t *)((uint32_t)ptr) = ((stat & 0x7F) << 24) | (0xFFFFFF & arh.MHour);

  }else{
    // Если FLASH не стерта пометить запись как недействительную
    *(__IO uint32_t *)((uint32_t)ptr) |= (1<<31);
    no_erase = TRUE;
  }

  FlashWaitEOP();

  if(*(__IO uint32_t *)((uint32_t)ptr+4) == 0){
    *(__IO uint32_t *)((uint32_t)ptr+4) = data;
  }else{
    // Если FLASH не стерта пометить запись как недействительную
    *(__IO uint32_t *)((uint32_t)ptr) |= (1<<31);
    no_erase = TRUE;
  }

  FlashWaitEOP();

  CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);

  return no_erase;
}

//=============================================================================
// Сохранение записи во FLASH
// stat - вид записи
// data - данные записи
//
void ArhivStoreNote(uint8_t stat, uint32_t data){

  uint32_t *ptr;
  BOOL flg_no_erase;
  uint16_t write_cnt = 0;

  FLASH_UNLOCK();

  do{

    ptr = (uint32_t *) ARHIV_BASE_ADR + arh.ArhivPtr * ARHIV_SIZE_ITEM;

    // Если указатель на начало страницы
    if(((uint32_t)ptr & (FLASH_PAGE_SIZE - 1)) == 0){
//      DBG_OUT2_HI;
      FlashPageErase(ptr);
    }

    flg_no_erase = FlashProgram(ptr, stat, data);

    arh.ArhivPtr = (arh.ArhivPtr < ARHIV_NUM_ITEMS - 1)? arh.ArhivPtr + 1: 0;

    write_cnt++;
    // Продолжать если была попытка записи в нестёртую ячейку и не превышен размер страницы
  }while((flg_no_erase == TRUE) && (write_cnt < (FLASH_PAGE_SIZE / ARHIV_SIZE_ITEM / 4)));

  FLASH_LOCK();

//  DBG_OUT2_LO;

  //	eeprom_config_write_sync();

}

//==================================================================
// Поиск последней записи в журнале
// Перевод указателя записи на первую свободную ячейку
// Восстановление моточасов по последней записи
//
void ArhivFindLastNote(void){

  uint32_t *ptr;
  arh.ArhivPtr = 0;
  arh.MHour = 0;
  BOOL fFind = FALSE;

  ptr = (uint32_t *) ARHIV_BASE_ADR + arh.ArhivPtr * ARHIV_SIZE_ITEM;

  // Если нулевая ячейка пуста, то завершение поиска
  if(*ptr == 0){
    fFind = TRUE;
  }

  while(!fFind){

    // Если запись действительна, то чтение моточасов
    if((*ptr & (1<<31)) == 0){
      arh.MHour = *ptr & 0xFFFFFF;
    }

    // Перевод указателя на следующую ячейку
    arh.ArhivPtr = (arh.ArhivPtr < ARHIV_NUM_ITEMS - 1)? arh.ArhivPtr + 1: 0;
    ptr = (uint32_t *) ARHIV_BASE_ADR + arh.ArhivPtr * ARHIV_SIZE_ITEM;

    // Если ячейка пуста, то завершение поиска
    if(*ptr == 0){
      fFind = TRUE;
    }

    // Если значение моточасов в ячейке меньше ранее считанных и запись действительна,
    // то завершение поиска
    if((arh.MHour > (*ptr & 0xFFFFFF)) && ((*ptr & (1<<31)) == 0)){
      fFind = TRUE;
    }

    // Если просмотрен весь журнал, то завершение поиска
    if(arh.ArhivPtr == 0){
      fFind = TRUE;
    }
  }

}

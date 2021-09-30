//===============================================================================
//
//
//
//
//	arhiv.c
//                                                   				  21.09.2021
//
//==============================================================================


#include "main.h"
#include "device.h"
#include "flash.h"
#include "arhiv.h"
#include "string.h"

ARCHIVE_TYPEDEF arh;

void DayArhivStore(void){

  uint32_t tmp;

  //ArhivStoreNote(ARCHIVE_CURRENT_VALUE,*(uint32_t*)&dgs.RegState.CurrentValue);

  //memcpy(&tmp,&arh.ValueMax,4);
  tmp = arh.ValueMax << 16;

  tmp |= arh.ValueMin;

  ArhivStoreNote(ARCHIVE_MAX_MIN_VALUE,tmp);

  arh.ValueMax = dev.RegInput.Value;
  arh.ValueMin = dev.RegInput.Value;

/*
  tmp = dev.RegInput.TempSensor/10 + (arh.Temper_Max << 24) + (arh.Temper_Min << 16);

  ArhivStoreNote(ARCHIVE_TEMPERATURA, tmp);
*/
  arh.Temper_Max = dev.RegInput.TempSensor/10;
  arh.Temper_Min = dev.RegInput.TempSensor/10;

}


//------------------------------------------------------------------------------
//
//
// arhiv.h
//                                                					  21.09.2021
//
//------------------------------------------------------------------------------

#ifndef ARHIV_H_
#define ARHIV_H_

enum{
  ARCHIVE_NO_MESSAGE,
  ARCHIVE_POWER_ON,
  ARCHIVE_CURRENT_VALUE,
  ARCHIVE_MAX_MIN_VALUE,
  ARCHIVE_SET_CALIB_ZERO,
  ARCHIVE_SET_CALIB_CONC,
  ARCHIVE_RES_1,
  ARCHIVE_RES_2,
  ARCHIVE_TEMPERATURA,
  ARCHIVE_SET_CONFIG_DEFAULT,

  ARCHIVE_FAIL,
  ARCHIVE_MESSAGE

};

typedef struct{

  uint32_t MHour;

  uint16_t ValueMax;
  uint16_t ValueMin;

  uint16_t ArhivPtr;
  int8_t Temper_Max;
  int8_t Temper_Min;

} ARCHIVE_TYPEDEF;

extern ARCHIVE_TYPEDEF arh;

void DayArhivStore(void);

#endif /* ARHIV_H_ */

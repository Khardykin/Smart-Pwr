//------------------------------------------------------------------------------
//
//
// lmp91000.h
//                                                					  31.08.2021
//
//------------------------------------------------------------------------------

#ifndef INC1_LMP91000_H_
#define INC1_LMP91000_H_

void LMP_Init(void);
void lmp_proc(void);


enum{
	MODE_TIA = 0x03,
	MODE_TEMPERAT = 0x07
};

void LMP_Set_Mode(uint8_t );

#endif /* INC1_LMP91000_H_ */

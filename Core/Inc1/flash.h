//------------------------------------------------------------------------------
//
//
// flash.h
//                                                					  10.09.2021
//
//------------------------------------------------------------------------------

#ifndef FLASH_H_
#define FLASH_H_

#define ARHIV_BASE_ADR 	0x08006000U
#define ARHIV_SIZE_ITEM 2
#define ARHIV_NUM_ITEMS 0x2000 / ARHIV_SIZE_ITEM / 4
#define FLASH_PAGE_SIZE 0x80

void ArhivFindLastNote(void);
void ArhivStoreNote(uint8_t stat, uint32_t data);


#endif /* FLASH_H_ */

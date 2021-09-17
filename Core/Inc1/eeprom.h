//------------------------------------------------------------------------------
//
//
//  eeprom.h
//                                                    	              03.09.2021
//
//------------------------------------------------------------------------------

#ifndef EEPROM_H_
#define EEPROM_H_

void eeprom_config_write(void);

void read_config_from_eeprom(void);

void factory_config_write(void);
BOOL factory_config_read(void);

#endif /* EEPROM_H_ */

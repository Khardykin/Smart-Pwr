/*
 * Mipex_command.h
 *
 *
 */

#ifndef INC2_MIPEX_COMMAND_H_
#define INC2_MIPEX_COMMAND_H_

#define MIPEX_UART 			USART2
#define TIME_RX_MIPEX 		(5)
#define MIPEX_STATE_WARM 	(0x0001)
#define MIPEX_STATE_ERR 	(0x0A86)
#define MIPEX_DISC			(2)
typedef enum {
	COMMAND_OEM_XXXX = 0,
	COMMAND_ID,
	COMMAND_RT,
	COMMAND_SRAL,
	COMMAND_AZERO,
	COMMAND_AZERO_OFF,
	COMMAND_AZERO_ON,
	COMMAND_INIT,
	COMMAND_ZERO2,
	COMMAND_CALB_AAAA,
	COMMAND_DATAE2,
	REPEATER,
}CommandMipexType;

extern void Mipex_init_buff(void);
extern void Mipex_Init(void);
extern void Mipex_Restart(void);
extern void Mipex_transmit_commmand(CommandMipexType command);
extern void Mipex_transmit_commmand_IT(CommandMipexType command);
extern void Mipex_transmit_commmand_repeat(char* data, uint8_t len);
extern void Mipex_transmit_commmand_repeat_IT(char* data, uint8_t len);

extern void Mipex_UART_IRQHandler(void);

extern ErrorStatus 	Mipex_status_parcing;	// Ошибка разбора команды

extern uint16_t 	TimerRxMipex;
extern uint8_t 		Mipex_repeater;			// Байт повторителя, 1 вкл, 0 выкл

extern uint16_t 	Password;				// Пароль для доступа в MIPEX по умолчанию

#endif /* INC2_MIPEX_COMMAND_H_ */

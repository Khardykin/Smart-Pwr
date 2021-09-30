//------------------------------------------------------------------------------
//
//
// modbus_lpuart.h
//                                                					  20.08.2021
//
//------------------------------------------------------------------------------

#ifndef INC1_MODBUS_LPUART_H_
#define INC1_MODBUS_LPUART_H_

void modbus_init(void);
void Modbus_TO_timer(void);
void Modbus_LPUART_IRQHandler(void);

#define MBS_LPUART LPUART1

extern BOOL f_mbs_packet_rcv;
extern uint16_t mbs_rx_pkt_len;

extern uint16_t mbs_tx_len;
extern uint16_t mbs_tx_cnt;

extern uint8_t mbs_pkt_rx[];
extern uint8_t mbs_pkt_tx[];

#endif /* INC1_MODBUS_LPUART_H_ */

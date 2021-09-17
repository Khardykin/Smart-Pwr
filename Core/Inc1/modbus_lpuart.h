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
///000
#define MBS_RX_BUF_SIZE 256
#define MBS_TX_BUF_SIZE 256

extern BOOL f_mbs_packet_rcv;
extern uint16_t mbs_rx_pkt_len;

extern uint16_t mbs_tx_len;
extern uint16_t mbs_tx_cnt;
///000
extern uint8_t mbs_pkt_rx[256];
extern uint8_t mbs_pkt_tx[256];

#endif /* INC1_MODBUS_LPUART_H_ */

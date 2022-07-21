//===============================================================================
//
//
//
//
//	modbus_lpuart.c
//                                                   				  20.08.2021
//
//==============================================================================

#include "main.h"
#include "debug.h"
#include "defines.h"
#include "modbus_lpuart.h"
#include "string.h"

#define MBS_RX_BUF_SIZE 256
#define MBS_TX_BUF_SIZE 256

uint16_t mbs_rx_cnt;
uint16_t mbs_rx_pkt_len;
//uint8_t mbs_buff_rx[MBS_RX_BUF_SIZE];
uint8_t mbs_pkt_rx[MBS_RX_BUF_SIZE];

uint16_t mbs_tx_len;
uint16_t mbs_tx_cnt;

//uint8_t mbs_buff_tx[(1024+10)];
uint8_t mbs_pkt_tx[MBS_TX_BUF_SIZE];


BOOL f_mbs_packet_rcv = FALSE;

void modbus_init(void){

	mbs_rx_cnt = 0;

	LL_LPUART_EnableDirectionRx(MBS_LPUART);
	LL_LPUART_EnableIT_RXNE(MBS_LPUART);
	LL_LPUART_Enable(MBS_LPUART);

}

void Modbus_TO_timer(void){

	//memcpy(mbs_pkt_rx, mbs_buff_rx, mbs_rx_cnt);
	mbs_rx_pkt_len = mbs_rx_cnt;

	mbs_rx_cnt = 0;
	f_mbs_packet_rcv = TRUE;

}

void Modbus_RXNEmpty_Callback(void);
void Modbus_TXEmpty_Callback(void);

void Modbus_LPUART_IRQHandler(void){

	if(LL_LPUART_IsEnabledIT_RXNE(MBS_LPUART) && LL_LPUART_IsActiveFlag_RXNE(MBS_LPUART)){
		Modbus_RXNEmpty_Callback();
	}

	if(LL_LPUART_IsEnabledIT_TXE(MBS_LPUART) && LL_LPUART_IsActiveFlag_TXE(MBS_LPUART)){
		Modbus_TXEmpty_Callback();
	}



	if(LL_LPUART_IsEnabledIT_TC(MBS_LPUART) && LL_LPUART_IsActiveFlag_TC(MBS_LPUART)){
		LL_USART_ClearFlag_TC(MBS_LPUART);
		LL_USART_DisableIT_TC(MBS_LPUART);
		LL_USART_EnableDirectionRx(MBS_LPUART);
	}


	if(LL_LPUART_IsActiveFlag_ORE(MBS_LPUART)){
		LL_LPUART_ClearFlag_ORE(MBS_LPUART);
	}

}

void Modbus_RXNEmpty_Callback(void){

	uint8_t tmp = LL_USART_ReceiveData8(MBS_LPUART);

	mbs_pkt_rx[mbs_rx_cnt] = tmp;

	if(mbs_rx_cnt < (MBS_RX_BUF_SIZE-1)){
		mbs_rx_cnt++;
	}

	if(mbs_rx_cnt == 1){
		  LL_TIM_EnableCounter(TIM22);
	}

    TIM22->CNT = 0;

}

void Modbus_TXEmpty_Callback(void){

	if(mbs_tx_cnt < mbs_tx_len){

		LL_USART_TransmitData8(MBS_LPUART,mbs_pkt_tx[mbs_tx_cnt]);
		mbs_tx_cnt++;

		if(mbs_tx_cnt == mbs_tx_len){

			LL_USART_DisableIT_TXE(MBS_LPUART);

			LL_USART_EnableIT_TC(MBS_LPUART);

		}

	}

}

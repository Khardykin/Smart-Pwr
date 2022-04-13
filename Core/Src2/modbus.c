//===============================================================================
//
//
//
//
//	modbus.c
//                                                   				  03.09.2021
//
//==============================================================================

#include "main.h"
#include "debug.h"
#include "defines.h"
#include "string.h"
#include "device.h"
#include "modbus.h"
#include "modbus_lpuart.h"
#include "Mipex_command.h"
#include "eeprom.h"
#include "flash.h"
#include "calculations.h"

//==============================================================================

uint32_t cmdModbusRTU(uint8_t *mas, uint32_t len, uint8_t *out);
uint16_t mb_crc(uint8_t *buf, int len);

//==============================================================================

void mb_proc(void){

	if(!f_mbs_packet_rcv)
		return;
	f_mbs_packet_rcv = FALSE;

#ifdef MB_REC_DBG
	d_printf("\n\rMB Receive len %d: ", mbs_rx_pkt_len);

	for(int i = 0; i < mbs_rx_pkt_len; i++)
		d_printf(" %02X", mbs_pkt_rx[i]);
#endif

	if(mbs_rx_pkt_len < 4)
		return;
	uint16_t crc = mb_crc(mbs_pkt_rx,mbs_rx_pkt_len-2);

	if((mbs_pkt_rx[0] == 0x55) && \
			(mbs_pkt_rx[mbs_rx_pkt_len-2] == LOBYTE(crc)) && \
			(mbs_pkt_rx[mbs_rx_pkt_len-1] == HIBYTE(crc))){

		mbs_tx_len = cmdModbusRTU(mbs_pkt_rx, mbs_rx_pkt_len, mbs_pkt_tx);

		mbs_tx_cnt = 0;
		LL_USART_DisableDirectionRx(MBS_LPUART);
		LL_USART_EnableIT_TXE(MBS_LPUART);

	}
#ifdef CONFIG_MIPEX
	else{
		Mipex_transmit_commmand_repeat((char*)mbs_pkt_rx, mbs_rx_pkt_len);
	}
#endif

}

//==============================================================================

uint16_t mb_crc(uint8_t *buf, int len)
{
	uint16_t crc = 0xFFFF;

	for (int pos = 0; pos < len; pos++)
	{
		crc ^= (uint16_t)buf[pos];

		for (int i = 8; i != 0; i--) {
			if ((crc & 0x0001) != 0) {
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
				crc >>= 1;
		}
	}

	return crc;
}


//======================================================================================================================================

// Коды доступа (регистр wAccessCode 0x1001/0x1020)

enum
{
	// Код доступа к изменению настроек
	DEF_CODE_MODE_UNLOCK			= 0xFACD,	// 64205

	// Коды доступа к сенсору
	DEF_CODE_CALIB_GAS_ZERO			= 0x185D,	// 6237		// Калибровка нуля
	DEF_CODE_CALIB_GAS_CONC			= 0x64C4,	// 25796	// Калибровка концентрации
	DEF_CODE_CALIB_GAS_CONC_AC		= 0x64DE,	// 25822	// Калибровка концентрации


	DEF_CODE_MODE_INIT_MIPEX		= 0x93BE,	// 37822	// Рестарт сенсора MIPEX
	DEF_CODE_MODE_DIRECT_MIPEX		= 0xBC53,	// 48211	// Прямой доступ к сенсору MIPEX
	DEF_CODE_CALIB_FID				= 0x14AE,	// 5294		// Режим непрерывной работы для ФИД во время калибровки

	DEF_CODE_MOTOHOURS_RESET		= 0x1010,	// 4112		// Сброс счётчиков наработки

	DEF_CODE_ACCESS_PUT_TEMP_ZERO	= 0x8055,	// 32853	// Добавить точку температурной коррекции нуля ( код АЦП )
	DEF_CODE_ACCESS_PUT_TEMP_CONC	= 0x8065,	// 32869	// Добавить точку температурной коррекции калибровочной концентрации ( код АЦП )
	DEF_CODE_ACCESS_PUT_LINE_CONC	= 0x8075,	// 32885	// Добавить точку линейной коррекции концентрации
	DEF_CODE_ACCESS_CLR_TEMP_ZERO	= 0x8F55,	// 36693	// Сброс таблицы температурной коррекции нуля ( код АЦП )
	DEF_CODE_ACCESS_CLR_TEMP_CONC	= 0x8F65,	// 36709	// Сброс таблицы температурной коррекции калибровочной концентрации ( код АЦП )
	DEF_CODE_ACCESS_CLR_LINE_CONC	= 0x8F75,	// 36725	// Сброс таблицы линейной коррекции концентрации

	DEF_CODE_SENS_PROFILE_SAVE		= 0xABCE,	// 43982	// Сохранение текущих настроек сенсора в заводской профиль настроек
	DEF_CODE_SENS_PROFILE_RESTORE	= 0xCE4B,	// 52811	// Восстановление из профиля заводских настроек сенсора

	DEF_CODE_SET_HARWARE_VERSION	= 0xABFE,	// 44030	// Сохранение версии аппаратной реализации сенсора
	///000
	DEF_CODE_NULL 					= 0x0,					// Сброс
};

/*
	Перед записью регистров и калибровки:
		- необходимо войти в сервисный режим (в регист статуса записать 0x0004);
		- ввести код доступа 64205 (0xFACD)

 */

//==============================================================================

uint32_t CmdFunc3(uint8_t *mas, uint32_t len, uint8_t *out);
uint32_t CmdFunc4(uint8_t *mas, uint32_t len, uint8_t *out);
uint32_t CmdFunc6(uint8_t *mas, uint32_t len, uint8_t *out);
uint32_t CmdFunc8(uint8_t *mas, uint32_t len, uint8_t *out);
uint32_t CmdFunc12(uint8_t *mas, uint32_t len, uint8_t *out);
uint32_t CmdFunc16(uint8_t *mas, uint32_t len, uint8_t *out);

//==============================================================================

enum
{
	/* 0 */	ERROR_Exception = 0,
	/* 1 */	ERROR_Illegal_Function,
	/* 2 */	ERROR_Illegal_Data_Address,
	/* 3 */	ERROR_Illegal_Data_Value,
	/* 4 */	ERROR_Illegal_Slave_Device_Failure,
	/* 5 */	ERROR_Illegal_Anknowledge,
	/* 6 */	ERROR_Illegal_Slave_Device_busy
};

//==============================================================================

static WORD Address;			// Адрес сохраняемого регистра
static BYTE *Value;				// Значение регистра
static WORD Count;				// Количество регистров
static BYTE *CountByte;			// Счётчик
static BYTE AnswerLen;			// Длина ответа (в байтах)
static BOOL Save;
static uint16_t wModeCalib;
//static uint16_t tmp;
uint16_t Reserv;

//==============================================================================

uint16_t AccessCode;
uint16_t AccessData;

BOOL mbUnlock;
BOOL mbServiceMode;
uint16_t mbHoldDevStatus;

//======================================================================================================================================
//                АДРЕСНОЕ ПРОСТРАНСТВО РЕГИСТРОВ ГРУППЫ HOLD ( функция 0x03 )
//======================================================================================================================================

typedef struct
{
	uint16_t	*pVar;
	BOOL		bSave;
} TVAR;

#define DEF_REG_ADR_BASE_x03		0x1000
#define DEF_REG_CNT_x03				0x7A

//==============================================================================

const TVAR reg_x03[] =
{
		/* 0x1000 */	{ (uint16_t*)&mbHoldDevStatus,				FALSE },	/* Состояние 							*/
		/* 0x1001 */	{ (uint16_t*)&AccessCode,					FALSE },	/* Код доступа / Режим					*/
		/* 0x1002 */	{ (uint16_t*)&dev.Config.TypeSensor,		TRUE },		/* Тип сенсора (Код)					*/
		/* 0x1003 */	{ (uint16_t*)&dev.Config.Unit,				TRUE },		/* Единица измерения концентрации		*/
		/* 0x1004 */	{ (uint16_t*)&dev.Config.ValueLow,			TRUE },		/* Нижнее значение 						*/
		/* 0x1005 */	{ (uint16_t*)&dev.Config.ValueHigh,			TRUE },		/* Верхнее значение: отображаемое		*/
		/* 0x1006 */	{ (uint16_t*)&dev.Config.ValueCalib,		TRUE },		/* Концентрация калибровочного газа 	*/
		/* 0x1007 */	{ (uint16_t*)&dev.Config.ADCLow,			TRUE },		/* Нижнее значение с АЦП 				*/
		/* 0x1008 */	{ (uint16_t*)&dev.Config.ADCHigh,			TRUE },		/* Верхнее значение с АЦП 				*/
		/* 0x1009 */	{ (uint16_t*)&dev.Config.Text[0],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100A */	{ (uint16_t*)&dev.Config.Text[1],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100B */	{ (uint16_t*)&dev.Config.Text[2],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100C */	{ (uint16_t*)&dev.Config.Text[3],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100D */	{ (uint16_t*)&dev.Config.Text[4],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100E */	{ (uint16_t*)&dev.Config.Text[5],			TRUE },		/* Текстовый описатель газа ASCII		*/
		/* 0x100F */	{ (uint16_t*)&dev.Config.Text[6],			TRUE },		/* Текстовый описатель газа ASCII		*/
		//---------------------------------------------------------------
		/* 0x1010 */	{ (uint16_t*)&dev.Config.Text[7],			TRUE },		/* Текстовый описатель газа				*/
		/* 0x1011 */	{ (uint16_t*)&dev.Config.SNum[0],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1012 */	{ (uint16_t*)&dev.Config.SNum[1], 			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1013 */	{ (uint16_t*)&dev.Config.SNum[2],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1014 */	{ (uint16_t*)&dev.Config.SNum[3],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1015 */	{ (uint16_t*)&dev.Config.SNum[4],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1016 */	{ (uint16_t*)&dev.Config.SNum[5],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1017 */	{ (uint16_t*)&dev.Config.SNum[6],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1018 */	{ (uint16_t*)&dev.Config.SNum[7],			TRUE },		/* Заводской номер самого сенсора ASCII	*/
		/* 0x1019 */	{ (uint16_t*)&dev.Config.ScaleKoef,			TRUE },		/* Масштабный коэффициент концентрации	*/
		/* 0x101A */	{ (uint16_t*)&dev.Config.ScaleADC,			TRUE },		/* Коэффициент усиления для АЦП			*/
		/* 0x101B */	{ (uint16_t*)&dev.Config.FID,				TRUE },		/* Настройки управления ФИД'ом			*/
		/* 0x101C */	{ (uint16_t*)&dev.Config.LMP_Gain,			TRUE },		/* Настройки LMP ucRLoad & ucGain		*/
		/* 0x101D */	{ (uint16_t*)&dev.Config.LMP_BiasSign,		TRUE },		/* Настройки LMP ucBias & ucBiasSign 	*/
		/* 0x101E */	{ (uint16_t*)&dev.Config.LMP_Source,		TRUE },		/* Настройки LMP ucIntZ & ucSource		*/
		/* 0x101F */	{ (uint16_t*)&dev.Config.LMP_Mode,			TRUE },		/* Настройки LMP  ucMode & ucFET		*/
		//----------------------------------------------------------------
		/* 0x1020 */	{ (uint16_t*)&AccessCode,					FALSE },	/* Код действия							*/
		/* 0x1021 */	{ (uint16_t*)&AccessData,					FALSE },	/* Параметр								*/
		/* 0x1022 */	{ (uint16_t*)&(HIWORD(dev.Config.Serial)),	TRUE },		/* Заводской номер прибора Hi			*/
		/* 0x1023 */	{ (uint16_t*)&(LOWORD(dev.Config.Serial)),	TRUE },		/* Заводской номер прибора Lo			*/
		/* 0x1024 */	{ (uint16_t*)&dev.Config.MolarMass,			TRUE },		/* Молярная масса газа					*/
		/* 0x1025 */	{ (uint16_t*)&dev.Config.ValueHighMeausure,	TRUE },		/* Верхнее значение: измеряемое			*/
		/* 0x1026 */	{ (uint16_t*)&dev.Config.VerHW,				TRUE },		/* Версия аппаратной реализации			*/
		/* 0x1027 */	{ (uint16_t*)&dev.Config.CalibZeroTemper,	TRUE },		/*	Темпер. при калибровке нуля			*/
		/* 0x1028 */	{ (uint16_t*)&dev.Config.CalibZeroADC,		TRUE },		/*	АЦП при калибровке нуля				*/
		/* 0x1029 */	{ (uint16_t*)&dev.Config.CalibConcTemper,	TRUE },		/*	Темпер. при калибровке концентрации	*/
		/* 0x102A */	{ (uint16_t*)&dev.Config.CalibConcADC,		TRUE },		/*	АЦП при калибровке концентрации		*/
		/* 0x102B */	{ (uint16_t*)&Reserv,						FALSE },		/*		Р Е З Е Р В						*/
		/* 0x102C */	{ (uint16_t*)&Reserv,						FALSE },		/*		Р Е З Е Р В						*/
		/* 0x102D */	{ (uint16_t*)&Reserv,						FALSE },		/*		Р Е З Е Р В						*/
		/* 0x102E */	{ (uint16_t*)&Reserv,						FALSE },		/*		Р Е З Е Р В						*/
		/* 0x102F */	{ (uint16_t*)&Reserv,						FALSE },		/*		Р Е З Е Р В						*/
		//------------------------------------------------------------------------------------------------------------------
		// Температурные корректировочные коэффициенты нуля
		/* 0x1030 */	{ (uint16_t*)&dev.Config.temp_corr_zero[0].Temp,			TRUE },// [0] Темература
		/* 0x1031 */	{ (uint16_t*)&dev.Config.temp_corr_zero[0].Koef,			TRUE },// [0] Коэффициент коррекции
		/* 0x1032 */	{ (uint16_t*)&dev.Config.temp_corr_zero[1].Temp,			TRUE },// [1] Темература
		/* 0x1033 */	{ (uint16_t*)&dev.Config.temp_corr_zero[1].Koef,			TRUE },// [1] Коэффициент коррекции
		/* 0x1034 */	{ (uint16_t*)&dev.Config.temp_corr_zero[2].Temp,			TRUE },// [2] Темература
		/* 0x1035 */	{ (uint16_t*)&dev.Config.temp_corr_zero[2].Koef,			TRUE },// [2] Коэффициент коррекции
		/* 0x1036 */	{ (uint16_t*)&dev.Config.temp_corr_zero[3].Temp,			TRUE },// [3] Темература
		/* 0x1037 */	{ (uint16_t*)&dev.Config.temp_corr_zero[3].Koef,			TRUE },// [3] Коэффициент коррекции
		/* 0x1038 */	{ (uint16_t*)&dev.Config.temp_corr_zero[4].Temp,			TRUE },// [4] Темература
		/* 0x1039 */	{ (uint16_t*)&dev.Config.temp_corr_zero[4].Koef,			TRUE },// [4] Коэффициент коррекции
		/* 0x103A */	{ (uint16_t*)&dev.Config.temp_corr_zero[5].Temp,			TRUE },// [5] Темература
		/* 0x103B */	{ (uint16_t*)&dev.Config.temp_corr_zero[5].Koef,			TRUE },// [5] Коэффициент коррекции
		/* 0x103C */	{ (uint16_t*)&dev.Config.temp_corr_zero[6].Temp,			TRUE },// [6] Темература
		/* 0x103D */	{ (uint16_t*)&dev.Config.temp_corr_zero[6].Koef,			TRUE },// [6] Коэффициент коррекции
		/* 0x103E */	{ (uint16_t*)&dev.Config.temp_corr_zero[7].Temp,			TRUE },// [7] Темература
		/* 0x103F */	{ (uint16_t*)&dev.Config.temp_corr_zero[7].Koef,			TRUE },// [7] Коэффициент коррекции
		//------------------------------------------------------------------------------------------------------------------
		// Температурные корректировочные коэффициенты концентрации
		/* 0x1040 */	{ (uint16_t*)&dev.Config.temp_corr_conc[0].Temp,			TRUE },// [0] Темература
		/* 0x1041 */	{ (uint16_t*)&dev.Config.temp_corr_conc[0].Koef,			TRUE },// [0] Коэффициент коррекции
		/* 0x1042 */	{ (uint16_t*)&dev.Config.temp_corr_conc[1].Temp,			TRUE },// [1] Темература
		/* 0x1043 */	{ (uint16_t*)&dev.Config.temp_corr_conc[1].Koef,			TRUE },// [1] Коэффициент коррекции
		/* 0x1044 */	{ (uint16_t*)&dev.Config.temp_corr_conc[2].Temp,			TRUE },// [2] Темература
		/* 0x1045 */	{ (uint16_t*)&dev.Config.temp_corr_conc[2].Koef,			TRUE },// [2] Коэффициент коррекции
		/* 0x1046 */	{ (uint16_t*)&dev.Config.temp_corr_conc[3].Temp,			TRUE },// [3] Темература
		/* 0x1047 */	{ (uint16_t*)&dev.Config.temp_corr_conc[3].Koef,			TRUE },// [3] Коэффициент коррекции
		/* 0x1048 */	{ (uint16_t*)&dev.Config.temp_corr_conc[4].Temp,			TRUE },// [4] Темература
		/* 0x1049 */	{ (uint16_t*)&dev.Config.temp_corr_conc[4].Koef,			TRUE },// [4] Коэффициент коррекции
		/* 0x104A */	{ (uint16_t*)&dev.Config.temp_corr_conc[5].Temp,			TRUE },// [5] Темература
		/* 0x104B */	{ (uint16_t*)&dev.Config.temp_corr_conc[5].Koef,			TRUE },// [5] Коэффициент коррекции
		/* 0x104C */	{ (uint16_t*)&dev.Config.temp_corr_conc[6].Temp,			TRUE },// [6] Темература
		/* 0x104D */	{ (uint16_t*)&dev.Config.temp_corr_conc[6].Koef,			TRUE },// [6] Коэффициент коррекции
		/* 0x104E */	{ (uint16_t*)&dev.Config.temp_corr_conc[7].Temp,			TRUE },// [7] Темература
		/* 0x104F */	{ (uint16_t*)&dev.Config.temp_corr_conc[7].Koef,			TRUE },// [7] Коэффициент коррекции
		//------------------------------------------------------------------------------------------------------------------
		// Коэффициенты линеаризации концентрации
		/* 0x1050 */	{ (uint16_t*)&dev.Config.linear[0].Conc,			TRUE },// [0] Концентрация
		/* 0x1051 */	{ (uint16_t*)&dev.Config.linear[0].Koef,			TRUE },// [0] Коэффициент коррекции
		/* 0x1052 */	{ (uint16_t*)&dev.Config.linear[1].Conc,			TRUE },// [1] Концентрация
		/* 0x1053 */	{ (uint16_t*)&dev.Config.linear[1].Koef,			TRUE },// [1] Коэффициент
		/* 0x1054 */	{ (uint16_t*)&dev.Config.linear[2].Conc,			TRUE },// [2] Концентрация
		/* 0x1055 */	{ (uint16_t*)&dev.Config.linear[2].Koef,			TRUE },// [2] Коэффициент
		/* 0x1056 */	{ (uint16_t*)&dev.Config.linear[3].Conc,			TRUE },// [3] Концентрация
		/* 0x1057 */	{ (uint16_t*)&dev.Config.linear[3].Koef,			TRUE },// [3] Коэффициент
		/* 0x1058 */	{ (uint16_t*)&dev.Config.linear[4].Conc,			TRUE },// [4] Концентрация
		/* 0x1059 */	{ (uint16_t*)&dev.Config.linear[4].Koef,			TRUE },// [4] Коэффициент
		/* 0x105A */	{ (uint16_t*)&dev.Config.linear[5].Conc,			TRUE },// [5] Концентрация
		/* 0x105B */	{ (uint16_t*)&dev.Config.linear[5].Koef,			TRUE },// [5] Коэффициент
		/* 0x105C */	{ (uint16_t*)&dev.Config.linear[6].Conc,			TRUE },// [6] Концентрация
		/* 0x105D */	{ (uint16_t*)&dev.Config.linear[6].Koef,			TRUE },// [6] Коэффициент
		/* 0x105E */	{ (uint16_t*)&dev.Config.linear[7].Conc,			TRUE },// [7] Концентрация
		/* 0x105F */	{ (uint16_t*)&dev.Config.linear[7].Koef,			TRUE },// [7] Коэффициент
		/* 0x1060 */	{ (uint16_t*)&dev.Config.linear[8].Conc,			TRUE },// [8] Концентрация
		/* 0x1061 */	{ (uint16_t*)&dev.Config.linear[8].Koef,			TRUE },// [8] Коэффициент
		/* 0x1062 */	{ (uint16_t*)&dev.Config.linear[9].Conc,			TRUE },// [9] Концентрация
		/* 0x1063 */	{ (uint16_t*)&dev.Config.linear[9].Koef,			TRUE },// [9] Коэффициент
		/* 0x1064 */	{ (uint16_t*)&dev.Config.linear[10].Conc,			TRUE },// [10] Концентрация
		/* 0x1065 */	{ (uint16_t*)&dev.Config.linear[10].Koef,			TRUE },// [10] Коэффициент
		/* 0x1066 */	{ (uint16_t*)&dev.Config.linear[11].Conc,			TRUE },// [11] Концентрация
		/* 0x1067 */	{ (uint16_t*)&dev.Config.linear[11].Koef,			TRUE },// [11] Коэффициент
		/* 0x1068 */	{ (uint16_t*)&dev.Config.linear[12].Conc,			TRUE },// [12] Концентрация
		/* 0x1069 */	{ (uint16_t*)&dev.Config.linear[12].Koef,			TRUE },// [12] Коэффициент
		/* 0x106A */	{ (uint16_t*)&dev.Config.linear[13].Conc,			TRUE },// [13] Концентрация
		/* 0x106B */	{ (uint16_t*)&dev.Config.linear[13].Koef,			TRUE },// [13] Коэффициент
		/* 0x106C */	{ (uint16_t*)&dev.Config.linear[14].Conc,			TRUE },// [14] Концентрация
		/* 0x106D */	{ (uint16_t*)&dev.Config.linear[14].Koef,			TRUE },// [14] Коэффициент
		/* 0x106E */	{ (uint16_t*)&dev.Config.linear[15].Conc,			TRUE },// [15] Концентрация
		/* 0x106F */	{ (uint16_t*)&dev.Config.linear[15].Koef,			TRUE },// [15] Коэффициент
		//------------------------------------------------------------------------------------------------------------------
		//------------------------------------------------------------------------------------------------------------------
		/* 0x1070 */	{ (uint16_t*)&dev.Config.UnitKoef[0],	TRUE },		/* 		*/
		/* 0x1071 */	{ (uint16_t*)&dev.Config.UnitKoef[1],	TRUE },		/*		*/
		/* 0x1072 */	{ (uint16_t*)&dev.Config.UnitKoef[2],	TRUE },		/* 		*/
		/* 0x1073 */	{ (uint16_t*)&dev.Config.UnitKoef[3],	TRUE },		/*		*/
		/* 0x1074 */	{ (uint16_t*)&dev.Config.UnitKoef[4],	TRUE },		/* 		*/
		/* 0x1075 */	{ (uint16_t*)&dev.Config.UnitKoef[5],	TRUE },		/*		*/
		/* 0x1076 */	{ (uint16_t*)&dev.Config.diskret_1_2,	TRUE },		/* 		*/
		/* 0x1077 */	{ (uint16_t*)&dev.Config.diskret_3_4,	TRUE },		/*		*/
		/* 0x1078 */	{ (uint16_t*)&dev.Config.diskret_5_6,	TRUE },		/* 		*/
		/* 0x1079 */	{ (uint16_t*)&dev.Config.UnitView,		TRUE },		/*		*/
		/* 0x107A */	/*		Р Е З Е Р В						*/
		/* 0x107B */	/*		Р Е З Е Р В						*/
		/* 0x107C */	/*		Р Е З Е Р В						*/
		/* 0x107D */	/*		Р Е З Е Р В						*/
		/* 0x107E */	/*		Р Е З Е Р В						*/
		/* 0x107F */	/*		Р Е З Е Р В						*/
		//------------------------------------------------------------------------------------------------------------------
};

//======================================================================================================================================
//                АДРЕСНОЕ ПРОСТРАНСТВО РЕГИСТРОВ ГРУППЫ INPUT ( функция 0x04 )
//======================================================================================================================================
#define DEF_REG_ADR_BASE_x04		0x0000
#define DEF_REG_CNT_x04				0x32

typedef uint16_t*	tINREG;
const tINREG reg_x04[]=
{
		/* 0x0000 */	(uint16_t*)&dev.RegInput.cod_8225,					/* cod_8225											*/
		/* 0x0001 */	(uint16_t*)&dev.RegInput.VerSW,						/* Код версии ПО								*/
		/* 0x0002 */	(uint16_t*)&dev.RegInput.VerSW_Build,				/* Код версии ПО. Build							*/
		/* 0x0003 */	(uint16_t*)&(HIWORD(dev.Config.Serial)),			/* Заводской номер HI							*/
		/* 0x0004 */	(uint16_t*)&(LOWORD(dev.Config.Serial)),			/* Заводской номер LO							*/
		/* 0x0005 */	(uint16_t*)&dev.RegInput.Value,						/* Концентрация									*/
		/* 0x0006 */	(uint16_t*)&dev.Status,								/* Статус										*/
		/* 0x0007 */	(uint16_t*)&dev.RegInput.Volt_3V,					/* Напряжение в цепи 3V							*/
		/* 0x0008 */	(uint16_t*)&dev.RegInput.Volt_5V,					/* Напряжение в цени 5V							*/
		/* 0x0009 */	(uint16_t*)&dev.RegInput.TempSensor,				/* Температура *10								*/
		/* 0x000A */	(uint16_t*)&dev.RegInput.Volt_Sens,					/* Напряжение с сенсора *1000					*/
		/* 0x000B */	(uint16_t*)&dev.RegInput.Volt_Bridge,				/* PI. Напряжение моста *100					*/
		/* 0x000C */	(uint16_t*)&dev.RegInput.Volt_BridgeV2,				/* PI. Напряжение с середины моста *1000		*/
		/* 0x000D */	(uint16_t*)&dev.RegInput.Volt_Sens_PI,				/* PI. Напряжение на сенсоре					*/
		/* 0x000E */	(uint16_t*)&dev.RegInput.Volt_Pelistor,				/* Внутреннее опорное напряжение *10			*/
		/* 0x000F */	(uint16_t*)&dev.RegInput.Volt_Heat,					/* Напряжение подогрева *10 					*/
		/* 0x0010 */	(uint16_t*)&(HIWORD(dev.RegInput.dwHours)),			/* Счётчик времени наработки HI					*/
		/* 0x0011 */	(uint16_t*)&(LOWORD(dev.RegInput.dwHours)),			/* Счётчик времени наработки LO					*/
		/* 0x0012 */	(uint16_t*)&(HIWORD(dev.RegInput.dwPPM_Min)),		/* Счётчик наработки по концентрации HI			*/
		/* 0x0013 */	(uint16_t*)&(LOWORD(dev.RegInput.dwPPM_Min)),		/* Счётчик наработки по концентрации LO			*/
		/* 0x0014 */	(uint16_t*)&(LOWORD(dev.RegInput.dwValue_mg_m3)),	/* Концентрация в мг/м3	LO						*/
		/* 0x0015 */	(uint16_t*)&(HIWORD(dev.RegInput.dwValue_mg_m3)),	/* Концентрация в мг/м3	HI						*/
		/* 0x0016 */	(uint16_t*)&dev.RegInput.TimeToOffHeat,				/* Время до конца прогрева						*/
		/* 0x0017 */	(uint16_t*)&dev.RegInput.CntMaxMin,					/* счётчик времени замера предельных значений	*/
		/* 0x0018 */	(uint16_t*)&dev.RegInput.ValueMin5m,				/* Концентрация минимальная за 5 минут			*/
		/* 0x0019 */	(uint16_t*)&dev.RegInput.ValueMax5m,				/* Концентрация максимальная за 5 минут 		*/

		/* M I P E X */
#if defined( SMART_SENS_MX )
		/* 0x001A */	/* Температура преобразователя в отсчетах АЦП							*/
		/* 0x001B */	/* Фильтрованное отношение умноженное на температурный коэффициент		*/
		/* 0x001C */	/* Сигнал рабочего приемника в отсчетах АЦП								*/
		/* 0x001D */	/* Сигнал опорного приемника в отсчетах АЦП								*/
		/* 0x001E */	/* Отношение Us/Uref с учетом коэффициента ZERO1						*/
		/* 0x001F */	/* Отношение St` с учетом коэффициента ZERO2							*/
		/* 0x0020 */	/* Отношение Sz с учетом коэффициента температурной чувствительности	*/
		/* 0x0021 */	/* Текущая концентрация датчика											*/
		/* 0x0022 */	/* Масштабированная концентрация с учетом калибровочного коэффициента	*/
		/* 0x0023 */	/* Кодированное статус слово											*/
		/* 0x0024 */	/* Заводской номер датчика												*/
		/* 0x0025 */	/* Заводской номер датчика												*/
		/* 0x0026 */	/* Заводской номер датчика												*/
		/* 0x0027 */	/* Заводской номер датчика												*/
		/* 0x0028 */	/* Характеристика датчика												*/
		/* 0x0029 */	/* Тип преобразователя													*/
		/* 0x002A */	/* Тип преобразователя													*/
		/* 0x002B */	/* Тип преобразователя													*/
		/* 0x002C */	/* Версия ПО микроконтроллера											*/
		/* 0x002D */	/* Версия ПО микроконтроллера											*/
		/* 0x002E */	/* Версия ПО микроконтроллера											*/
		/* 0x002F */	/* Версия ПО микроконтроллера											*/
		/* 0x0030 */	/* Версия ПО микроконтроллера											*/
		/* 0x0031 */	/* Версия ПО микроконтроллера											*/
		/* 0x0032 */	/* Заводской номер датчика												*/
		/* 0x0033 */	/* Заводской номер датчика												*/
		/* 0x0034 */	/* 	Р Е З Е Р В															*/
		/* 0x0035 */	/* 	Р Е З Е Р В															*/
		/* 0x0036 */	/* 	Р Е З Е Р В															*/
		/* 0x0037 */	/* 	Р Е З Е Р В															*/
		/* 0x0038 */	/* 	Р Е З Е Р В															*/
		/* 0x0039 */	/* 	Р Е З Е Р В															*/
#else
		/* 0x001A */	(uint16_t*)&dev.RegInput.Value,		/* Текущая концентрация газа							*/
		/* 0x001B */	(uint16_t*)&Reserv,					/* Текущая концентрация газа до линеризации				*/
		/* 0x001C */	(uint16_t*)&dev.RegInput.Value_0,	/* Текущая концентрация газа до температурной коррекции */
		/* 0x001D */	(uint16_t*)&dev.Config.ADCLow,		/* Нижнее значение АЦП									*/
		/* 0x001E */	(uint16_t*)&dev.Config.ADCHigh,		/* Верхнее значение АЦП									*/
		/* 0x001F */	(uint16_t*)&dev.RegInput.ADC_0,		/* Значение АЦП до температурной коррекции				*/
		/* 0x0020 */	(uint16_t*)&dev.RegInput.ADC_TK,	/* Значение АЦП после температурной коррекции			*/
		/* 0x0021 */	(uint16_t*)&Reserv,					/* Коэфициент при температурной коррекции (множитель)	*/
		/* 0x0022 */	(uint16_t*)&Reserv,					/* Коэфициент при температурной коррекции (смещение)	*/
		/* 0x0023 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0024 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0025 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0026 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0027 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0028 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0029 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002A */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002B */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002C */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002D */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002E */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x002F */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0030 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
		/* 0x0031 */	(uint16_t*)&Reserv,					/* 	Р Е З Е Р В											*/
#endif
};

// Основной обработчик команд

uint32_t cmdModbusRTU(uint8_t *mas, uint32_t len, uint8_t *out)
{
	uint32_t AnswerLen = 0;	// Длина ответа (в байтах)
	uint32_t crc;

	//	ServiceTimeCnt = ServiceTO;

	out[0] = mas[0];
	out[1] = mas[1];

	switch (mas[1])
	{
	//--------------------------------------------------------------------
	case 0x03: // Чтение группы регистров HOLD
		AnswerLen = CmdFunc3(mas, len, out);
		break;
		//--------------------------------------------------------------------
	case 0x04: // Чтение группы регистров INPUT
		AnswerLen = CmdFunc4(mas, len, out);
		break;
		//--------------------------------------------------------------------
	case 0x06: // Запись одного регистра
		AnswerLen = CmdFunc6(mas, len, out);
		break;
		//--------------------------------------------------------------------
	case 0x08: // Эхо
		AnswerLen = CmdFunc8(mas, len, out);
		break;
		//--------------------------------------------------------------------
	case 0x0C: // Чтение 128 байт журнала
		AnswerLen = CmdFunc12(mas, len, out);
		break;
		//--------------------------------------------------------------------
	case 0x10: // Запись группы регистров
		AnswerLen = CmdFunc16(mas, len, out);
		break;
		//--------------------------------------------------------------------
		//--------------------------------------------------------------------
	default: //ucLenTx = 0;	return;
		//out[0] = SGMpoint[0].Config->NetAdress;
		d_printf("\n\r\n\r ERROR_Illegal_Function");

		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Function;
		AnswerLen = 3;
		break;
	} // switch (mas[1])

	if(AnswerLen)
	{
		crc = mb_crc(out, AnswerLen);
		out[AnswerLen]   = LOBYTE(crc);
		out[AnswerLen+1] = HIBYTE(crc);

		AnswerLen += 2;
	}

	//	if(out[0] == 0)
	//		AnswerLen = 0;

	return(AnswerLen);
}

//==============================================================================

void modbusSet(void){

	if(mbUnlock){
		AccessCode = 1;
	}
	else{
		AccessCode = 0;
	}


	if(mbServiceMode){
		dev.Status |= (1 << STATUS_BIT_MAIN_MODE);
	}else{
		dev.Status &=~ (1 << STATUS_BIT_MAIN_MODE);
	}

	mbHoldDevStatus = dev.Status;

}

//==============================================================================

void __CMD_PUT_WORD(uint16_t DATA)
{
	Value[0] = HIBYTE(DATA);
	Value[1] = LOBYTE(DATA);

	(*CountByte) += 2;		Count --;	Value +=2;	AnswerLen += 2;
	Address ++;
}


#define CMD_PUT_WORD(DATA)				__CMD_PUT_WORD(DATA);

#define CMD_LOAD_WORD(BUF,REG)								\
		HIBYTE( REG )	= (BUF)[0];	/* Регистр HI */			\
		LOBYTE( REG )	= (BUF)[1];	/* Регистр LO */


//--------------------------------------------------------------------------------------------------------------------------------------
// Чтение группы регистров HOLD

uint32_t CmdFunc3(uint8_t *mas, uint32_t len, uint8_t *out)
{
	//	tmp = 0;
	//	wModeCalib = st_dgs_state.ModeCalib;

	out[0] = mas[0];	// Адрес устройства
	out[1] = mas[1];	// Функция

	CMD_LOAD_WORD(mas+2, Address);	// Адрес начального регистра
	CMD_LOAD_WORD(mas+4, Count);	// Количество регистров

	Value		= (BYTE*)&(out[3]);	// Выходной поток
	CountByte	= &(out[2]);		// Счётчик
	AnswerLen	= 3;				// Длина ответа (в байтах)
	*CountByte = 0;
	//------------------------------------------------------------------------------------------------------------------
	if( !Count || (Count > 125) )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	do
	{
#if DEF_REG_ADR_BASE_x03 > 0
		if( Address < DEF_REG_ADR_BASE_x03 ) break;
#endif
		if( Address >= (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
		if( Count > DEF_REG_CNT_x03 ) break;
		if( (Address + Count) > (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
#if DEF_REG_ADR_BASE_x03 > 0
		Address -= DEF_REG_ADR_BASE_x03;
#endif

		//#define MB_READ_DBG

#ifdef MB_READ_DBG
		d_printf("\n\r%02x CMD Modbus 3 (%02d) : read adr %03x : data ", mas[0], Count, Address);
#endif
		while( Count )
		{
#ifdef MB_READ_DBG
			d_printf(" %04x", *reg_x03[ Address ].pVar);
#endif
			CMD_PUT_WORD( *reg_x03[ Address ].pVar );
		}

	} while(0);
	//------------------------------------------------------------------------------------------------------------------
	if( Count )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	return( AnswerLen );
}

//======================================================================================================================================
// Чтение группы регистров INPUT

uint32_t CmdFunc4(uint8_t *mas, uint32_t len, uint8_t *out)
{

	//	tmp = 0;
	out[0] = mas[0];	// Адрес устройства
	out[1] = mas[1];	// Функция

	CMD_LOAD_WORD(mas+2, Address);		// Адрес начального регистра
	CMD_LOAD_WORD(mas+4, Count);		// Количество регистров

	Value		= (BYTE*)&(out[3]);		// Выходной поток
	CountByte	= &(out[2]);			// Счётчик
	AnswerLen	= 3;					// Длина ответа (в байтах)
	*CountByte = 0;
	//------------------------------------------------------------------------------------------------------------------
	if( !Count || (Count > 125) )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------

	//tmp = DEF_DEVICE_ID;

	do
	{
		/*if( Count > DEF_REG_CNT_x04 ) break;
		if( (Address + Count) > DEF_REG_CNT_x04 ) break;*/

		if( Address < DEF_REG_ADR_BASE_x04 ) break;
		if( Address >= (DEF_REG_ADR_BASE_x04 + DEF_REG_CNT_x04) ) break;
		if( Count > DEF_REG_CNT_x04 ) break;
		if( (Address + Count) > (DEF_REG_ADR_BASE_x04 + DEF_REG_CNT_x04) ) break;

		//#define	MB_READ_DBG
		//		d_printf("\n\rCMD Modbus 4 Val - %05d", dev.RegInput.Value);
#ifdef MB_READ_DBG
		d_printf("\n\r%02x CMD Modbus 4 (%02d) : read adr %03x : data ", mas[0], Count, Address);
#endif
		Address -= DEF_REG_ADR_BASE_x04;

		while( Count )
		{
#ifdef MB_READ_DBG
			d_printf(" %04x", *reg_x04[ Address ]);
#endif
			CMD_PUT_WORD( *reg_x04[ Address ] );
		}
	} while(0);
	//-----------------------------------------------------------------------------------------------------------------
	if(Count)
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	return(AnswerLen);
}
//======================================================================================================================================
void __CMD_SAVE_WORD(TVAR *var)
{
#define DATA	*(var->pVar)
#define SAVE	var->bSave

	HIBYTE(DATA)	= Value[0];
	LOBYTE(DATA)	= Value[1];
	if(SAVE) Save	= SAVE;
	Address ++;
	Count --;
	Value += 2;

#undef DATA
#undef SAVE
}

#define CMD_SAVE_WORD(DATA)				__CMD_SAVE_WORD( (TVAR*)DATA );

//======================================================================================================================================
// Извлечение слова (сначала старший, потом младший байты)
#define TO_M_WORD(tw_data)		(((WORD)(*(tw_data)) << 8) + *((tw_data)+1))
//======================================================================================================================================


// Защита от случайной порчи настроек

BOOL check_block(void){

	uint16_t tmp;

	if(		( !mbServiceMode && (Address != 0x1000)) || \
			( !mbUnlock && (Address != 0x1001) && (Address != 0x1000)))
	{ // В рабочем режиме изменение регистров ЗАПРЕЩЕНО !!!

		return(FALSE);
	}

	// *}

	HIBYTE(tmp)	= Value[0];
	LOBYTE(tmp)	= Value[1];

	if(Address == 0x1001){

		if((tmp == 0xFACD)){
			mbUnlock = TRUE;
		}else if(tmp == 0){
			mbUnlock = FALSE;
		}else if(!mbUnlock){
			return(FALSE);
		}

	}

	if(Address == 0x1000){
		if( TESTBIT( tmp, STATUS_BIT_MAIN_MODE))
		{ // Возможно включение сервисного режима
			mbServiceMode = TRUE;
		}else{ // Отключение сервисного режима

			serviceTimerStop();
		}

	}

	if( mbServiceMode || mbUnlock)
	{ // Преобразователь в сервисном режиме, продлеваем время
		serviceTimerStart( SERVICE_TIME_MODBUS );
	}


	return(TRUE);
}

//==============================================================================

void modbusCMD(void){

	uint16_t cmd;

	if(Address != 0x1001)
		return;

	HIBYTE(cmd)	= Value[0];
	LOBYTE(cmd)	= Value[1];

	switch(cmd){

	case DEF_CODE_CALIB_GAS_ZERO:
		d_printf("\n\rCalib Zero");
#ifdef CONFIG_MIPEX
		Mipex_transmit_commmand(COMMAND_INIT);
		Mipex_transmit_commmand(COMMAND_ZERO2);
#else
		CalibGasZero();
		eeprom_config_write();
#endif
		break;

	case DEF_CODE_CALIB_GAS_CONC:
		d_printf("\n\rCalib Conc");
#ifdef CONFIG_MIPEX
		Mipex_transmit_commmand(COMMAND_CALB_AAAA);
#else
		CalibGasConc();
		eeprom_config_write();
#endif
		break;

	case DEF_CODE_SENS_PROFILE_SAVE:

		factory_config_write();

		break;

	case DEF_CODE_SENS_PROFILE_RESTORE:

		factory_config_read();

		break;
		///000
#ifdef CONFIG_FID
	case DEF_CODE_CALIB_FID:
		// Режим непрерывной работы для ФИД во время калибровки включить
		f_TimeCalibFid = TRUE;
		break;
		///000
	case DEF_CODE_NULL:
		// Режим непрерывной работы для ФИД во время калибровки выключить
		f_TimeCalibFid = FALSE;
		// Выключаем питание на сенсоре
		LL_GPIO_ResetOutputPin(TURN_ON_IR_GPIO_Port, TURN_ON_IR_Pin);
		dev.Status &=~ (1 << STATUS_BIT_FID_PWR);
		break;
#endif
		///000
#ifdef CONFIG_MIPEX
	case DEF_CODE_MODE_INIT_MIPEX:
		//Mipex_Restart();
		Mipex_repeater = 0;
		break;
	case DEF_CODE_MODE_DIRECT_MIPEX:
		Mipex_repeater = 1;
		break;
#endif
	default:
		break;
	}

}

//--------------------------------------------------------------------------------------------------------------------------------------
// Запись одного регистра
uint32_t CmdFunc6(uint8_t *mas, uint32_t len, uint8_t *out)
{
	Address 	= TO_M_WORD(mas+2);			// Адрес сохраняемого регистра
	Value		= (BYTE*)&(mas[4]);			// Значение регистра
	Count   	= 1;						// Количество регистров

	memcpy((char*)out, (char*)mas, 6);
	Save = FALSE;
	//TSGMPoint *sgm = &SGMpoint[0];
	//------------------------------------------------------------------------------------------------------------------

#define BLOCK_MODBUS

#ifdef BLOCK_MODBUS
	/*
	if( ( Address >= DEF_reg_x03_UnLock_LO ) && ( Address <= DEF_reg_x03_UnLock_HI ) )
	{
		__NOP();
	} else
	if( ( Address >= DEF_reg_x03_UnLock2_LO ) && ( Address <= DEF_reg_x03_UnLock2_HI ) )
	{
		__NOP();
	} else
	if( ( Address >= DEF_reg_x03_UnLock3_LO ) && ( Address <= DEF_reg_x03_UnLock3_HI ) )
	{
		__NOP();
	} else
	 */

	if(!check_block()){

		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Slave_Device_Failure;
		return(3);

	}

#endif

	modbusCMD();

	//------------------------------------------------------------------------------------------------------------------
#ifdef DEF_AT45_ENABLE
	archAddNote_Modbus( ARCH_EVENT_RS_WRITE, mas, len );
#endif
	//------------------------------------------------------------------------------------------------------------------
	do
	{
#if DEF_REG_ADR_BASE_x03 > 0
		if( Address < DEF_REG_ADR_BASE_x03 ) break;
#endif
		if( Address >= (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
		if( Count > DEF_REG_CNT_x03 ) break;
		if( (Address + Count) > (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
#if DEF_REG_ADR_BASE_x03 > 0
		Address -= DEF_REG_ADR_BASE_x03;
#endif

#define MB_WRITE_DBG

#ifdef MB_WRITE_DBG
		d_printf("\n\r%02x CMD Modbus 6 : write adr %03x : data ", mas[0], Address);
		d_printf(" %02x%02x", Value[0], Value[1]);
#endif
		//CMD_SAVE_WORD( *reg_x03[ Address ].pVar, reg_x03[ Address ].bSave );
		CMD_SAVE_WORD( &reg_x03[ Address ] );
	} while(0);
	//------------------------------------------------------------------------------------------------------------------
	if( Count )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------

	modbusSet();

	//------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------
	if(Save)
	{
		eeprom_config_write();

		//chSaveConfigAll();
#if defined( SMART_SENS_EL ) || defined( SMART_SENS_EL_O2 )
		stMain.bUpDate = TRUE;
#endif
	}
	//------------------------------------------------------------------------------------------------------------------
	return(6);
}
//======================================================================================================================================
// Эхо
uint32_t CmdFunc8(uint8_t *mas, uint32_t len, uint8_t *out)
{
	memcpy((char*)out, (char*)mas, 6);
	return(6);
}
//======================================================================================================================================
// Передача памяти архива
uint32_t CmdFunc12(uint8_t *mas, uint32_t len, uint8_t *out)
{

	int i;
	uint16_t num_p;
	uint32_t ptr;

	CMD_LOAD_WORD(mas+2, num_p);		// Номер пакета

	if( num_p >=  (ARHIV_NUM_ITEMS * ARHIV_SIZE_ITEM * 4) / 128)
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Value;
		return(3);
	}

	for(i=0; i < 32; i++){
		ptr = ARHIV_BASE_ADR + 128 * num_p + i * 4;
		((uint32_t *)out)[i] = *(__IO uint32_t *)((uint32_t)ptr);
	}
	return(3+128);
}

//======================================================================================================================================
// Запись группы регистров
uint32_t CmdFunc16(uint8_t *mas, uint32_t len, uint8_t *out)
{
	wModeCalib = 0;

	Address 	= TO_M_WORD(mas+2);			// Адрес первого сохраняемого регистра
	Count	 	= TO_M_WORD(mas+4);			// Количество регистров
	Value		= (BYTE*)&(mas[7]);			// Значение регистра

	memcpy((char*)out, (char*)mas, 6);
	Save = FALSE;
	//TSGMPoint *sgm = &SGMpoint[0];
	//------------------------------------------------------------------------------------------------------------------
#ifdef BLOCK_MODBUS

	if(!check_block()){

		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Slave_Device_Failure;
		return(3);

	}

#endif

	modbusCMD();

	//------------------------------------------------------------------------------------------------------------------
#ifdef DEF_AT45_ENABLE
	archAddNote_Modbus( ARCH_EVENT_RS_WRITE, mas, len );
#endif
	//------------------------------------------------------------------------------------------------------------------
	if( !Count || (Count > 125) )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	do
	{
#if DEF_REG_ADR_BASE_x03 > 0
		if( Address < DEF_REG_ADR_BASE_x03 ) break;
#endif
		if( Address >= (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
		if( Count > DEF_REG_CNT_x03 ) break;
		if( (Address + Count) > (DEF_REG_ADR_BASE_x03 + DEF_REG_CNT_x03) ) break;
#if DEF_REG_ADR_BASE_x03 > 0
		Address -= DEF_REG_ADR_BASE_x03;
#endif

#ifdef MB_WRITE_DBG
		d_printf("\n\r%02x CMD Modbus 16 (%02d) : write adr %03x : data ", mas[0], Count, Address, *Value);
#endif

		while( Count )
		{
#ifdef MB_WRITE_DBG
			d_printf(" %02x%02x", Value[0], Value[1]);
#endif
			//CMD_SAVE_WORD( *reg_x03 [Address ].pVar, reg_x03[ Address ].bSave );
			CMD_SAVE_WORD( &reg_x03[ Address ] );
		}

	} while(0);
	//------------------------------------------------------------------------------------------------------------------
	if( Count )
	{
		out[1] |= 0x80;
		out[2] = ERROR_Illegal_Data_Address;
		return(3);
	}
	//------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------
	modbusSet();
	//------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------
	if(Save)
	{
		eeprom_config_write();
		//chSaveConfigAll();
#if defined( SMART_SENS_EL ) || defined( SMART_SENS_EL_O2 )
		stMain.bUpDate = TRUE;
#endif
	}
	//------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------
	return(6);
}


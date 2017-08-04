/*
 * PN532.h
 *
 *  Created on: 16 Jul 2017
 *      Author: Shahriar Sayeed (3pinmicro)
 */

#ifndef NFC_PN532_H_
#define NFC_PN532_H_

#include <stdio.h>
#include <string.h>
#include "Serial/SPI.h"






//-----------------------------//
//PN532 device definitions.
//-----------------------------//



//Normal information frame:
//PREAMBLE  ST_CODE1  ST_CODE  LEN  LCS  TFI  PD0 PD1  ........ PDn DCS POSTAMBLE

//Extended information frame:
//PREAMBLE  ST_CODE1  ST_CODE  FF FF LENM  LENL  LCS  TFI  PD0 PD1  ........ PDn DCS POSTAMBLE




//BAUD definitions:
#define PN532_SER_BAUD_9600			0x00
#define PN532_SER_BAUD_19200		0x01
#define PN532_SER_BAUD_38400		0x02
#define PN532_SER_BAUD_57600		0x03
#define PN532_SER_BAUD_115200		0x04
#define PN532_SER_BAUD_230400		0x05
#define PN532_SER_BAUD_460800		0x06
#define PN532_SER_BAUD_921600		0X07
#define PN532_SER_BAUD_1288000		0x08

//Configuration definitions:
#define PN532_CONF_MODE_NORMAL		0x01		//Normal: The SAM is not used; this is the default mode.
#define PN532_CONF_MODE_VIR_CARD	0x02		//Virtual Card: The couple PN532+SAM is seen as only
												//one contactless SAM card from the external world.
#define PN532_CONF_MODE_WIRED_CARD	0x03		//Wired Card: The host controller can access to the SAM with
												//standard PCD commands (InListPassiveTarget, InDataExchange, ..).
#define PN532_CONF_MODE_DUAL_CARD	0x04		//Dual Card: Both the PN532 and the SAM are visible from the
												//external world as two separated targets.

#define PN532_CONF_TIMEOUT(s)		(s*1000/50)	//LSB in 50ms. So a value of s = 1sec, gives 20 value.
#define PN532_CONF_IRQ_UNUSED		0x00
#define PN532_CONF_IRQ_USED			0x01

//Power down wakeup definitions:
#define PN532_PD_WAKEUP_SRC_INT0	(1 << 0)	//Wakeup source is INT0.
#define PN532_PD_WAKEUP_SRC_INT1	(1 << 1)	//Wakeup source is INT1.
#define PN532_PD_WAKEUP_SRC_RFU		(0 << 2)	//RFO always set to 0.
#define PN532_PD_WAKEUP_SRC_RFLVL	(1 << 3)	//Wakeup source is RF level detector.
#define PN532_PD_WAKEUP_SRC_HSU		(1 << 4)	//Wakeup source is HSU.
#define PN532_PD_WAKEUP_SRC_SPI		(1 << 5)	//Wakeup source is SPI.
#define PN532_PD_WAKEUP_SRC_GPIO	(1 << 6)	//Wakeup source is GPIO.
#define PN532_PD_WAKEUP_SRC_I2C		(1 << 7)	//Wakeup source is I2C.




//Transfer definitions:
#define PN532_PREAMBLE				0x00
#define PN532_STARTCODE1			0x00
#define PN532_STARTCODE2			0xFF
#define PN532_POSTAMBLE				0x00
#define PN532_HOST_TO_PN532			0xD4		//TFI byte to indicate direction of transfer.
#define PN532_PN532_TO_HOST			0xD5

//SPI transfer sequence starts:
#define  PN532_SPI_STATREAD			0x02
#define  PN532_SPI_DATAWRITE		0x01
#define  PN532_SPI_DATAREAD			0x03
#define  PN532_SPI_READY			0x01


//Miscellaneous command definitions:
#define PN532_CMD_GET_FIRM_VER		0x02
#define PN532_CMD_GET_GEN_STATUS	0x04
#define PN532_CMD_RD_REGISTER		0x06
#define PN532_CMD_WR_REGISTER		0x08
#define PN532_CMD_RD_GPIO			0x0C
#define PN532_CMD_WR_GPIO			0x0E
#define PN532_CMD_SET_BAUD			0x10
#define PN532_CMD_SET_PARAMS		0x12
#define PN532_CMD_SET_CONFIG		0x14

//RF communication command definitions:
#define PN532_CMD_INLISTPASSIVETARGET	0x4A
#define PN532_CMD_INDATAEXCHANGE		0x40
#define PN532_CMD_MIFARE_READ			0x30
#define PN532_CMD_MIFARE_WRITE			0xA0
#define PN532_CMD_AUTH_WITH_KEYA		0x60
#define PN532_CMD_AUTH_WITH_KEYB		0x61


#define PN532_MIFARE_ISO14443A		0x00
#define PN532_HSU_WAKEUP_BYTE		0x55
#define PN532_KEY_A					1
#define PN532_KEY_B					2





//Data structure for managing PN532.
typedef struct PN532 PN532;
struct PN532
{

};







//Function definitions.
void PN532_Init(void);
unsigned char PN532_SPI_GetFirmwareVersion(void);
unsigned char PN532_SPI_GetGeneralStatus(void);
unsigned char PN532_SPI_ReadRegister(unsigned char *regArray, unsigned char *values, unsigned short items);
unsigned char PN532_SPI_WriteRegister(unsigned char *regArray, unsigned short items);
unsigned char PN532_SPI_Read_GPIO(unsigned char *p3, unsigned char *p7, unsigned char *i0i1);
unsigned char PN532_SPI_Write_GPIO(unsigned char p3, unsigned char p7);
unsigned char PN532_SPI_SetSerialBaud(unsigned char setting);
unsigned char PN532_SPI_SetParameters(unsigned char flags);
unsigned char PN532_SPI_SetConfiguration(unsigned char mode, unsigned char timeout, unsigned char irq);
unsigned char PN532_SPI_PowerDown(unsigned char wakeupEnable, unsigned char genIRQ);







#endif /* NFC_PN532_H_ */

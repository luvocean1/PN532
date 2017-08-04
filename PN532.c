/*
 * PN532.c
 *
 *  Created on: 16 Jul 2017
 *      Author: Shahriar Sayeed (3pinmicro)
 */


#include "PN532.h"
#include "System/Task.h"
#include "Serial/DBG_UART.h"




//Private data structure variable.
PN532 pn532;



//Transaction specific data.
const char PN532_ACK_Data[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};






//Forward declarations:
static void PN532_SPI_WakeUp(void);
static unsigned char PN532_WaitStatusReady(void);
static unsigned char PN532_SPI_Recv_ACK(void);
static unsigned char PN532_SPI_Send_ACK(void);

static void PN532_SPI_WriteCmd(unsigned char cmd, unsigned char *data, unsigned char len);
static unsigned char PN532_SPI_WriteCmdCheck_ACK(unsigned char cmd, unsigned char *data, unsigned char len);
static unsigned char PN532_SPI_GetRespData(unsigned char respType, unsigned char *data, unsigned short *len);






//Initialize the PN532 NFC controller.
void PN532_Init(void)
{
	//Initialize the SPI interface.
	SPI_PortInit(&spi_MEM, SPI_MEM_INT, SPI_SCL_FREQ_4MHz, 0xFF);
	PN532_SPI_WakeUp();

	printf("PN532: Initialised\r\n");
}


//Wake the chip up.
void PN532_SPI_WakeUp(void)
{
	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	TaskDelay_us(2000);
	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.
}


//Get current status of the chip.
static unsigned char PN532_GetStatus(void)
{
	unsigned char status;

	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	SPI_WriteByte(&spi_MEM, PN532_SPI_STATREAD);
	SPI_ReadByte(&spi_MEM, &status);
	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.

	return status;
}


//Wait for chip to respond ready using SPI.
unsigned char PN532_WaitStatusReady(void)
{
	unsigned long timeout = 10000;
	unsigned char status = PN532_GetStatus();

	while (!(status & PN532_SPI_READY))
	{
		if (timeout == 0)
		{
			return KO;
		}
		else
		{
			timeout--;
			status = PN532_GetStatus();
		}
	}
	return OK;
}


//Simply read an array of bytes.
static void PN532_SPI_ReadBytes(unsigned char *data, unsigned char len)
{
	unsigned char i;

	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	TaskDelay_us(2000);
	SPI_WriteByte(&spi_MEM, PN532_SPI_DATAREAD);

	for (i = 0; i < len; i++)
	{
		SPI_ReadByte(&spi_MEM, &data[i]);
	}

	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.
}


//Simply send an array of bytes.
static void PN532_SPI_WriteBytes(unsigned char *data, unsigned char len)
{
	unsigned char i;

	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	TaskDelay_us(2000);
	SPI_WriteByte(&spi_MEM, PN532_SPI_DATAWRITE);

	for (i = 0; i < len; i++)
	{
		SPI_WriteByte(&spi_MEM, data[i]);
	}

	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.
}


//Read ACK data using SPI.
unsigned char PN532_SPI_Recv_ACK(void)
{
	unsigned char data[6];

	PN532_SPI_ReadBytes(data, 6);
	DBG_PrintHex("ACK:", data, 6);
	if (memcmp(data, PN532_ACK_Data, 6) == 0)
	{
		return OK;
	}

	return KO;
}


//Send an ACK using SPI.
unsigned char PN532_SPI_Send_ACK(void)
{
	PN532_SPI_WriteBytes(PN532_ACK_Data, 6);

	return KO;
}


//Write command via SPI interface.
//Here len does not include cmd byte, but only data length.
void PN532_SPI_WriteCmd(unsigned char cmd, unsigned char *data, unsigned char len)
{
	unsigned char i;
	unsigned char checkSum;

	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	TaskDelay_us(2000);

	//Length to include the TFI and CMD byte as well before DATA starts.
	len += 2;
	SPI_WriteByte(&spi_MEM, PN532_SPI_DATAWRITE);
	SPI_WriteByte(&spi_MEM, PN532_PREAMBLE);
	SPI_WriteByte(&spi_MEM, PN532_STARTCODE1);
	SPI_WriteByte(&spi_MEM, PN532_STARTCODE2);

	SPI_WriteByte(&spi_MEM, len);
	SPI_WriteByte(&spi_MEM, ~len + 1);			//Complement of len.
	SPI_WriteByte(&spi_MEM, PN532_HOST_TO_PN532);

	//Send the command byte.
	SPI_WriteByte(&spi_MEM, cmd);
	checkSum = PN532_HOST_TO_PN532 + cmd;

	//Send the command data.
	if (data != NULL)
	{
		for (i = 0; i < (len - 2); i++)
		{
			SPI_WriteByte(&spi_MEM, data[i]);
			checkSum += data[i];
		}
	}

	checkSum = ~checkSum + 1;
	SPI_WriteByte(&spi_MEM, checkSum);
	SPI_WriteByte(&spi_MEM, PN532_POSTAMBLE);

	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.
}


//Write a command followed by checking of ACK status.
//Here cmd is the actual cmd ID, and data is the command data to be sent.
unsigned char PN532_SPI_WriteCmdCheck_ACK(unsigned char cmd, unsigned char *data, unsigned char len)
{
	//First write the command.
	PN532_SPI_WriteCmd(cmd, data, len);

	//Wait till chip is ready.
	if (PN532_WaitStatusReady() != OK)
	{
		return KO;
	}

	//Read in the ACK.
	if (PN532_SPI_Recv_ACK() != OK)
	{
		return KO;
	}

	//Wait till we have a status ready again.
	if (PN532_WaitStatusReady() != OK)
	{
		return KO;
	}

	return OK;
}



//Read data from PN532 using SPI interface.
unsigned char PN532_SPI_GetRespData(unsigned char respType, unsigned char *data, unsigned short *len)
{
	unsigned char dummy, origSum;
	unsigned char calcSum = 0;
	unsigned char buffer[8];
	unsigned char status = OK;
	unsigned short i, dataLen = 0;
	unsigned long timeout = 1000;

	spi_MEM.ChipSelect(SPI_MEM_CS_PIN);			//Set CS low.
	TaskDelay_us(2000);
	SPI_WriteByte(&spi_MEM, PN532_SPI_DATAREAD);

	//Try to read in data, but look for 0x00 followed by 0xFF
	//as start of sync bytes.
	SPI_ReadByte(&spi_MEM, buffer);
	while (timeout)
	{
		if (*buffer == PN532_STARTCODE1)
		{
			SPI_ReadByte(&spi_MEM, buffer);
			if (*buffer == PN532_STARTCODE2)
			{
				//We have got sync bytes. Next byte is length.
				SPI_ReadByte(&spi_MEM, &buffer[0]);
				SPI_ReadByte(&spi_MEM, &buffer[1]);
				//Is this extended frame?
				if (buffer[0] == 0xFF && buffer[1] == 0xFF)
				{
					SPI_ReadByte(&spi_MEM, &buffer[0]);	//LEN MSB.
					SPI_ReadByte(&spi_MEM, &buffer[1]);	//LEN LSB.
					SPI_ReadByte(&spi_MEM, &buffer[2]);	//LEN MSB + LEN LSB + LCS = 0x00.
					if (((buffer[0] + buffer[1] + buffer[2]) & 0xFF) != 0x00)
					{
						*len = 0;
						status = KO;		//We have corrupted data.
						break;
					}
					dataLen = ((unsigned short)buffer[0] << 8) | buffer[1];
				}
				else if (((buffer[0] + buffer[1]) & 0xFF) == 0x00)	//Normal frame.
				{
					dataLen = buffer[0];
				}
				else
				{
					*len = 0;
					status = KO;			//We have corrupted data.
					break;
				}

				//Read the TFI and respType bytes.
				SPI_ReadByte(&spi_MEM, &buffer[0]);
				SPI_ReadByte(&spi_MEM, &buffer[1]);
				calcSum += buffer[0] + buffer[1];
				if (buffer[0] != PN532_PN532_TO_HOST	||
					buffer[1] != respType				||
					dataLen < 2)
				{
					*len = 0;
					status = KO;			//We have corrupted data.
					break;
				}

				//Read the data bytes.
				dataLen -= 2;
				for (i = 0; i < dataLen; i++)
				{
					//Limit our reading by our data buffer length.
					if (data != NULL && i < *len)
					{
						SPI_ReadByte(&spi_MEM, &data[i]);
						calcSum += data[i];
					}
					else
					{
						SPI_ReadByte(&spi_MEM, &dummy);
						calcSum += dummy;
					}
				}
				SPI_ReadByte(&spi_MEM, &origSum);
				SPI_ReadByte(&spi_MEM, &dummy);		//Postamble.
				if (((origSum + calcSum) & 0xFF) != 0x00)
				{
					*len = 0;
					status = KO;			//We have corrupted data.
					printf("PN532: Bad checksum: 0x%02X, exp: 0x%02X\r\n", origSum, ~calcSum + 1);
					break;
				}
				else if(dataLen < *len)
				{
					*len = dataLen;			//Mark end of data.
				}
				break;
			}
			else if (*buffer != PN532_STARTCODE1)
			{
				SPI_ReadByte(&spi_MEM, buffer);
			}
		}
		else
		{
			SPI_ReadByte(&spi_MEM, buffer);
		}
		if (--timeout == 0)
		{
			*len = 0;
			status = KO;			//We have corrupted data.
			break;
		}
	}
	spi_MEM.ChipUnselect(SPI_MEM_CS_PIN);		//Set CS high.
	return status;
}




//Get the firmware version.
unsigned char PN532_SPI_GetFirmwareVersion(void)
{
	unsigned char buffer[4];
	unsigned short len = 4;

	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_GET_FIRM_VER, NULL, 0) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_GET_FIRM_VER + 1, buffer, &len) == OK && len == 4)
	{
		printf("IC     : 0x%02X\r\n", buffer[0]);
		printf("Ver    : 0x%02X\r\n", buffer[1]);
		printf("Rev    : 0x%02X\r\n", buffer[2]);
		printf("Support: 0x%02X\r\n", buffer[3]);
		return OK;
	}
	else
	{
		printf("PN532: Failed to read firmware version.\r\n");
		return KO;
	}
}


//Get general status: allows the host controller to know
//at a given moment the complete situation of the PN532.
unsigned char PN532_SPI_GetGeneralStatus(void)
{
	unsigned char buffer[12];
	unsigned short len = 12;
	unsigned char i;

	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_GET_GEN_STATUS, NULL, 0) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_GET_GEN_STATUS + 1, buffer, &len) == OK)
	{
		printf("Err   : 0x%02X\r\n", buffer[0]);
		printf("Field : 0x%02X\r\n", buffer[1]);
		printf("NbTg  : 0x%02X\r\n", buffer[2]);

		for (i = 0; i < buffer[2]; i++)
		{
			printf("\tTg%d  : 0x%02X\r\n", i, buffer[3+4*i]);
			printf("\tBrRx%d: 0x%02X\r\n", i, buffer[3+4*i + 1]);
			printf("\tBrTx%d: 0x%02X\r\n", i, buffer[3+4*i + 2]);
			printf("\tType%d: 0x%02X\r\n", i, buffer[3+4*i + 3]);
		}
		if (buffer[2] > 0)
		{
			printf("Status: 0x%02X\r\n", buffer[3+4*i + 4]);
		}
		else
		{
			printf("Status: 0x%02X\r\n", buffer[3]);
		}
		return OK;
	}
	else
	{
		printf("PN532: Failed to get general status.\r\n");
		return KO;
	}
}


//Read register: used to read the content of one or several
//internal registers of the PN532 (located either in the SFR
//area or in the XRAM memory space).
unsigned char PN532_SPI_ReadRegister(unsigned char *regArray, unsigned char *values, unsigned short items)
{
	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_RD_REGISTER, regArray, items*2) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_RD_REGISTER + 1, values, &items) != OK)
	{
		printf("PN532: Failed to read registers.\r\n");
		return KO;
	}

	return OK;
}


//Write register: overwrite the content of one or several
//internal registers of the PN532 (located either in the SFR
//area or in the XRAM memory space).
//Structure of the input array is such that:
//[ADR1L ADR1L VAL1, ... ADRnL ADRnL VALn]
//Where n is the number of items
unsigned char PN532_SPI_WriteRegister(unsigned char *regArray, unsigned short items)
{
	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_WR_REGISTER, regArray, items*3) != OK)
	{
		return KO;
	}

	//Now read the response data. Not expecting any items.
	items = 0;
	if (PN532_SPI_GetRespData(PN532_CMD_WR_REGISTER + 1, NULL, &items) != OK)
	{
		printf("PN532: Failed to write registers.\r\n");
		return KO;
	}

	return OK;
}


//Read GPIO function:
//PN532 output: D5 0D P3 P7 I0I1
//Where:
//	  P3 =   0   0 P35 P34 P33 P32 P31 P30 bit positions.
//	  P7 =   0   0   0   0   0 P72 P71   0 bit positions.
//	I0I1 =   0   0   0   0   0   0  I1  I0 bit positions.
unsigned char PN532_SPI_Read_GPIO(unsigned char *p3, unsigned char *p7, unsigned char *i0i1)
{
	unsigned char buffer[3];
	unsigned short len = 3;

	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_RD_GPIO, NULL, 0) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_RD_GPIO + 1, buffer, &len) == OK && len == 3)
	{
		*p3 = buffer[0];
		*p7 = buffer[1];
		*i0i1 = buffer[2];
		return OK;
	}
	else
	{
		printf("PN532: Failed to read GPIO.\r\n");
		return KO;
	}
}


//Write GPIO function:
//PN532 input: D4 0E P3 P7
//Where:
//	P3 =  Val nu P35 P34 P33 P32 P31 P30 bit positions.
//	P7 =  Val nu  nu  nu  nu P72 P71   0 bit positions.
//Error condition is indicated by setting all bytes to 0xFFFFFFFF (-1).
//Otherwise success is indicated by 0.
unsigned char PN532_SPI_Write_GPIO(unsigned char p3, unsigned char p7)
{
	unsigned char buffer[2];
	unsigned short len = 0;

	//Setup the command to be sent.
	buffer[0] = p3;
	buffer[1] = p7;
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_WR_GPIO, buffer, 2) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_WR_GPIO + 1, NULL, &len) != OK)
	{
		printf("PN532: Failed to write GPIO.\r\n");
		return KO;
	}

	return OK;
}


//Set serial buad:
unsigned char PN532_SPI_SetSerialBaud(unsigned char setting)
{
	unsigned short len = 0;

	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_SET_BAUD, &setting, 1) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_SET_BAUD + 1, NULL, &len) != OK)
	{
		printf("PN532: Failed to set baud.\r\n");
		return KO;
	}

	//Must send ACK after setting serial baud and wait 200us.
	PN532_SPI_Send_ACK();
	TaskDelay_us(200);

	return OK;
}



//Set parameters: used to set internal parameters of the PN532,
//and then to configure its behavior regarding different cases.
unsigned char PN532_SPI_SetParameters(unsigned char flags)
{
	unsigned short len = 0;

	//Setup the command to be sent.
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_SET_PARAMS, &flags, 1) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_SET_PARAMS + 1, NULL, &len) != OK)
	{
		printf("PN532: Failed to set parameters.\r\n");
		return KO;
	}

	return OK;
}


//Set configurations: used to select the data flow path by
//configuring the internal serial data switch.
unsigned char PN532_SPI_SetConfiguration(unsigned char mode, unsigned char timeout, unsigned char irq)
{
	unsigned char buffer[3];
	unsigned short len = 0;

	//Setup the command to be sent.
	buffer[0] = mode;
	buffer[1] = timeout;
	buffer[2] = irq;
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_SET_CONFIG, buffer, 3) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_SET_CONFIG + 1, NULL, &len) != OK)
	{
		printf("PN532: Failed to set configuration.\r\n");
		return KO;
	}

	return OK;
}

//PowerDown: used to put the PN532 (including the contactless analog front end)
//into Power Down mode in order to save power consumption.
unsigned char PN532_SPI_PowerDown(unsigned char wakeupEnable, unsigned char genIRQ)
{
	unsigned char buffer[2];
	unsigned char status;
	unsigned short len = 1;

	//Setup the command to be sent.
	buffer[0] = wakeupEnable;
	buffer[1] = genIRQ;
	if (PN532_SPI_WriteCmdCheck_ACK(PN532_CMD_SET_CONFIG, buffer, 2) != OK)
	{
		return KO;
	}

	//Now read the response data.
	if (PN532_SPI_GetRespData(PN532_CMD_SET_CONFIG + 1, &status, &len) == OK && len == 1)
	{
		printf("PN532: Powerdown status 0x%02X.\r\n", status);
		return OK;
	}
	else
	{
		printf("PN532: Failed to powerdown.\r\n");
		return KO;
	}
}












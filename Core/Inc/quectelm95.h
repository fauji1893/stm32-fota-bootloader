/*
 * m95.h
 *
 *  Created on: Apr 11, 2020
 *      Author: fauzi
 */

#ifndef INC_QUECTELM95_H_
#define INC_QUECTELM95_H_

#include "usart.h"
#include <stdbool.h>
#include "gpio.h"

#define PWR_GPIO	GPIOB
#define PWR_PIN		GPIO_PIN_3

#define AT_CMD_CR	'\r'
#define AT_CMD_LF	'\n'



typedef enum
{
	Module_Idle,
	Module_Busy,
	Module_Not_Responding,

}ModuleStatus_t;

typedef enum
{
	COMMAND_IDLE,
	COMMAND_OK,
	COMMAND_FAIL,
	COMMAND_ERROR,
	COMMAND_CME_ERROR,
	COMMAND_CMS_ERROR
}AtCommandResponse_t;


//to handle what response from the module
typedef enum
{
	Write_Command,
	Execute_Command,
	URC,
	URC_InCommand,

}CommandType_t;

typedef enum
{
	Idle,
//	IpStart,
//	IpConfig,
//	IpInd,
//	IpGprsAct,
//	IpStatus,
//	TcpConnecting,
//	IpClose,
	ConnectOk,
	AlreadyConnect,
	ConnectFail,
	PdpDeact
}ConnectionStatus_t;

//typedef enum
//{
//	SendData_Idle=0,
//	SendData_SendOK,
//	SendData_SendFail,
//
//}GPRSSendData_t;
typedef void (*UrcCB)(uint8_t *pUrcStr, uint16_t len);

typedef struct
{
    uint8_t* p_urcStr;
    UrcCB p_urcCallBack;

}UrcEntity;

typedef struct
{
    uint8_t* 			p_ResponseStr;
    AtCommandResponse_t commandResponse;

}ResponseEntity;

typedef struct
{
	bool				newMessageIndication;
	uint8_t				messageIndex;
	uint8_t				phoneNumber[18];
	uint8_t				ready;
}SMS_t;

typedef struct
{
	uint8_t				FtpPacket[4096];
	uint8_t				PacketIndex;
	uint8_t				PacketReady;
	uint32_t			FileSize;
	bool				DataMode;
	uint16_t			DataIndex;
	uint8_t				TotalPacket;
}FTP_t;

typedef struct
{
	ModuleStatus_t		status;
	CommandType_t		type;
	AtCommandResponse_t	response;
	uint8_t				newLine;
	uint8_t				expectedNewLine;

}M95AtCommand_t;

typedef struct
{
	bool				ready;
	uint16_t        	UsartRxIndex;
	uint16_t 			timeout;
	uint8_t				UsartRxTemp;
	uint8_t				RxBufferQueue;
	uint8_t				UsartRxBuffer[4][256];
	FTP_t				FTP;
	char                IMEI[16];
	M95AtCommand_t   AtCommand;
	bool				GSMSignal;
	bool				GPRSSignal;
	bool				GPRSStatus;
	SMS_t				SMS;
	ConnectionStatus_t	connectStatus;

}M95_t;

extern M95_t m95;




/*funtion prototype*/

uint32_t StrToInt(uint8_t* input);
bool M95_FTPConfig(uint8_t* user, uint8_t* pass, uint8_t* dir);
bool M95_FTPOpen(uint8_t* ip, uint8_t* port);
uint32_t M95_FTPFileSize(uint8_t* fileName);
bool M95_FTPGetFile(uint8_t* fileName);
bool M95_ActivateGPRS(void);
bool M95_DeactivateGPRS(void);
uint8_t M95_StartTCP(void);
bool M95_CloseTCP(void);
bool M95_SendData(char *data);
uint8_t M95_ConnectionStatus(void);
void M95_SyncRTCMCU(void);
bool M95_Init(void);
void M95_InitParameter(void);
void M95_RxCallBack(void);
bool M95_SMSSend(uint8_t* phone, uint8_t* message);
void M95_SMSRead(void);
bool M95_FSWrite(char *fileName, char *data);
bool M95_FSRead(char *fileName);
bool M95_FSDelete(char *fileName);
bool M95_FSCreate(char *fileName);
uint8_t M95_SendATCommand(char *AtCommand, uint32_t  maxWaiting_ms);

#endif /* INC_SIM800_H_ */

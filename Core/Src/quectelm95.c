/*
 * m95.c
 *
 *  Created on: Apr 12, 2020
 *      Author: fauzi
 */
#include "quectelm95.h"
#include <string.h>

M95_t m95;



uint32_t StrToInt(uint8_t *input);


uint32_t StrToInt(uint8_t *input){
	uint32_t result = 0;
	uint8_t buffer[15]={0};
	uint8_t i = 0;

	while(1){
		if(*input>47 && *input<58){
			buffer[i++]=*input;
		}
		if(*input==0) break;
		input++;
	}


	return result = atoi(buffer);
}

bool M95_FTPConfig(uint8_t* user, uint8_t* pass, uint8_t* dir){

	char buffer[100] = {0};
	sprintf(buffer, "AT+QFTPUSER=\"%s\"\r",user);
	M95_SendATCommand(buffer, 1000);

	sprintf(buffer, "AT+QFTPPASS=\"%s\"\r",pass);
	M95_SendATCommand(buffer, 1000);

	sprintf(buffer, "AT+QFTPPATH=\"Firmware Release/%s/\"\r",dir);
	M95_SendATCommand(buffer, 1000);
//	M95_SendATCommand("AT+QFTPPATH=\"Firmware Release/\"\r", 1000);
}

bool M95_FTPOpen(uint8_t* ip, uint8_t* port){

	bool status = false;
	uint32_t timeout = 0;

	timeout = HAL_GetTick();

	char buffer[40] = {0};
	sprintf(buffer, "AT+QFTPOPEN=\"%s\",%s\r",ip,port);
	if(M95_SendATCommand(buffer, 20000)== Module_Idle){

		/* wait untill ftp connected/not connected */
		while(HAL_GetTick() - timeout < 10000){
			if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue], "+QFTPOPEN:0")){
				status = true;
				break;
			}
		}

	}

	return status;
}

uint32_t M95_FTPFileSize(uint8_t* fileName){

	char buffer[100] = {0};
	uint32_t timeout = 0;
	uint32_t size = 0;

	timeout = HAL_GetTick();

	sprintf(buffer, "AT+QFTPSIZE=\"%s\"\r",fileName);
	if(M95_SendATCommand(buffer, 20000)== Module_Idle){
		/* wait server response */
		while(HAL_GetTick() - timeout < 10000){
			if(m95.AtCommand.status == Module_Idle){
				if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue-1], "+QFTPSIZE:")){
					if(strchr(m95.UsartRxBuffer[m95.RxBufferQueue-1],'-')==NULL){
						size = StrToInt(m95.UsartRxBuffer[m95.RxBufferQueue-1]);
						break;
					}
					else{
						break;
					}

				}
			}
		}
	}
	return size;
}

bool M95_FTPGetFile(uint8_t* fileName){
	bool status = false;
	uint32_t timeout = 0;

	timeout = HAL_GetTick();

	char buffer[40] = {0};
	sprintf(buffer, "AT+QFTPGET=\"%s\"\r",fileName);
	if(M95_SendATCommand(buffer, 20000)== Module_Idle){

		/* wait server response */
		while(HAL_GetTick() - timeout < 30000){
			if (m95.FTP.DataMode==true)break;
		}


	}
	return m95.FTP.DataMode;
}


bool M95_SMSSend(uint8_t* phone, uint8_t* message){
	bool status = false;

	char buffer[40] = {0};
	sprintf(buffer, "AT+CMGS=\"%s\"\r",phone);
	if(M95_SendATCommand(buffer, 2000)== Module_Idle){
		if(M95_SendATCommand(message, 5000)== Module_Idle){
			status = true;
		}
	}
	return status;
}

bool M95_Init(void) {

	uint32_t timeout = HAL_GetTick();

	m95.FTP.DataMode=false;
	m95.GPRSSignal = 0;
	m95.ready = 0;
	m95.AtCommand.status = Module_Idle;

	HAL_Delay(750);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	HAL_Delay(1500);
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	//HAL_Delay(10000);


	while (m95.ready == 0)
	{
		if (HAL_GetTick() - timeout > 20000){

			break;
		}

	}

	return m95.ready;
	//M95_SendATCommand("AT\r", 10000);

}


uint8_t M95_SendATCommand(char *AtCommand, uint32_t maxWaiting_ms) {

	uint32_t sendCommandTime;

	sendCommandTime = HAL_GetTick();

	/* wait until end of URC message */
	while (1) {
		//if ((HAL_GetTick() - sendCommandTime) > maxWaiting_ms) return 0;
		if (m95.timeout > 250 && m95.AtCommand.status == Module_Idle)
			break;
	}
	//HAL_UART_Transmit(&huart2, (uint8_t*) "\0x13", 1, 0xFFFF);
//	for(int i = 0; i<4;i++){
		memset(m95.UsartRxBuffer, 0, sizeof(m95.UsartRxBuffer));
//	}
	m95.RxBufferQueue = 0;
	m95.AtCommand.status = Module_Busy;
	m95.UsartRxIndex = 0;
	m95.AtCommand.response = COMMAND_IDLE;

	//sendCommandTime = HAL_GetTick();
	printf("%s\n", AtCommand);
	USART_Puts(UART5, AtCommand);
	//HAL_UART_Transmit(&huart2, (uint8_t*) AtCommand, strlen(AtCommand), 0xFFFF);
	if (AtCommand[0] != 'A')
		USART_Puts(UART5, "\x1A");
	//HAL_UART_Transmit(&huart2, (uint8_t*) "\x1A", 1, 0xFFFF);
	//HAL_UART_Transmit(&huart2, (uint8_t*) "\0x11", 1, 0xFFFF);
	while (m95.AtCommand.status == Module_Busy) {

		if ((HAL_GetTick() - sendCommandTime) > maxWaiting_ms) {
			printf("Modem Not Responding\r\n");
			m95.AtCommand.status = Module_Not_Responding;
//			M95_SendATCommand(AtCommand, maxWaiting_ms);
			M95_Init();
			break;
		}
	}
	return m95.AtCommand.status;
}


void M95_RxCallBack(void) {
	static uint8_t parseResponse = 0;
	uint8_t rxTemp;
	static uint8_t rxTemp2;


	m95.timeout = 0;
	rxTemp = LL_USART_ReceiveData8(UART5);

	if(m95.FTP.DataMode){
		m95.AtCommand.status = Module_Busy;
		m95.FTP.FtpPacket[m95.FTP.DataIndex++] = rxTemp;
		if(m95.FTP.PacketIndex<m95.FTP.TotalPacket){
			if(m95.FTP.DataIndex == 4096){
				m95.FTP.DataMode = false;
				m95.FTP.PacketReady = 1;
				m95.FTP.DataIndex = 0;
			}
		}
		else{
			if(m95.FTP.DataIndex == m95.FTP.FileSize%4096){
				m95.FTP.DataMode = false;
				m95.FTP.PacketReady = 1;
				m95.FTP.DataIndex = 0;
			}
		}
	}
	else{
		if(parseResponse){
			if(rxTemp == '\r'){


			}
			if(rxTemp == '\n'){

				if(m95.ready){
					if(strcmp(m95.UsartRxBuffer[m95.RxBufferQueue], "OK")==0){
						m95.AtCommand.response = COMMAND_OK;
					}
					else if (strchr(m95.UsartRxBuffer[m95.RxBufferQueue], '.') != NULL){
						m95.AtCommand.response = COMMAND_OK;
					}
					else if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue], "ERROR")!=NULL){
						m95.AtCommand.response = COMMAND_ERROR;
					}
					else if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue], "+CGREG: 1")!=NULL){
						m95.GPRSSignal = 1;
					}
					else if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue], "CONNECT") !=NULL){
						m95.FTP.DataMode = true;
					}
				}
				else{
					if(strstr(m95.UsartRxBuffer[m95.RxBufferQueue], "RDY")!=NULL){
						m95.ready = 1;
					}
				}

				m95.AtCommand.status = Module_Idle;

	//			printf("response: %s\r\n", m95.UsartRxBuffer[m95.RxBufferQueue]);
				parseResponse = 0;
				m95.RxBufferQueue++;
			}
			else {
				m95.AtCommand.status = Module_Busy;
				m95.UsartRxBuffer[m95.RxBufferQueue][m95.UsartRxIndex++]=rxTemp;
			}
		}
		else{
			if(rxTemp2 == '\n' && rxTemp != '\r' ){
				if(rxTemp=='>'){
					m95.AtCommand.response = COMMAND_OK;
					m95.AtCommand.status = Module_Idle;
				}
				else{
					if(m95.RxBufferQueue>3)m95.RxBufferQueue=0;
					parseResponse = 1;
					m95.UsartRxIndex = 0;
					m95.UsartRxBuffer[m95.RxBufferQueue][m95.UsartRxIndex++]=rxTemp;
				}
			}
		}

		rxTemp2 = rxTemp;

			LL_USART_ClearFlag_TC(USART1);
			LL_USART_TransmitData8(USART1,rxTemp);


	}


}

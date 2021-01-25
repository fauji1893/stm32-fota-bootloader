/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "quectelm95.h"
#include "flash_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UPDATE_INDICATOR	((uint32_t)0x08005800)
#define FTP_IP				((uint32_t)0x08005808)
#define FTP_PORT			((uint32_t)0x08005828)
#define FTP_FILENAME		((uint32_t)0x08005848)
#define PHONE_NUMBER		((uint32_t)0x08005868)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef void (*pFunction)(void);

pFunction JumpToApplication;
uint32_t JumpAddress;

uint8_t ftpIp[20] = "175.158.47.234";
uint8_t ftpPort[12] = "54218";
uint8_t phoneNum[20] = "089670947492";
uint8_t fileName[32] = "app.bin";
uint8_t totalFile;
uint8_t ftpUser[12] = "dev";
uint8_t ftpPass[12] = "sewul2912";
uint32_t indicatorValue = 0x000000;
uint8_t status = 1;
uint8_t smsMessage[50] = "Firmware update success";
uint8_t updateMode = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t DownloadFile(uint8_t *fileName, uint32_t fileSize);
void Read_FTPConfig(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_UART5_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	LL_USART_EnableIT_RXNE(UART5);
//	LL_USART_EnableIT_RXNE(USART1);

	/* check for update */
	if(*(__IO uint32_t*) UPDATE_INDICATOR == 0xABABABAB){
		updateMode = 1;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);


		uint32_t timeout = 0;
		uint8_t modem_initCounter = 0;

		printf("\r\nFirmware update mode\r\n");
		/* Read FTP configuration from EEPROM */
		Read_FTPConfig();

		while (modem_initCounter++ < 3) {
			if (M95_Init())
				break;
		}

		M95_SendATCommand("AT+CGREG=1\r", 10000);
		M95_SendATCommand("AT+CMGF=1\r", 10000);
		M95_SendATCommand("AT+CSCS=\"GSM\"\r", 10000);

		FLASH_If_Init();
		FLASH_If_Write(UPDATE_INDICATOR, (uint32_t*) indicatorValue, 1);

		/* Wait untill internet connection is ready */
		timeout = HAL_GetTick();
		while (HAL_GetTick() - timeout < 60000 * 5) {
			if (m95.GPRSSignal)
				break;
		}

		/* Set FTP username and password */
		M95_FTPConfig(ftpUser, ftpPass, fileName);

		if (m95.GPRSSignal) {

			/* Open connection with FTP server */
			if (M95_FTPOpen(ftpIp, ftpPort)) {
				char buffer[100]={0};

				/* Check the new firmware file size
				 * Skip firmware update if file size exceeds the predetermined memory allocation */
				sprintf(buffer, "%s.%d",fileName,totalFile);
				m95.FTP.FileSize = M95_FTPFileSize(buffer);
				if (m95.FTP.FileSize != 0 && (m95.FTP.FileSize + 4096*totalFile) < USER_FLASH_SIZE) {

					printf("Create backup point\r\n");
					/* Free up APPLICATION_2 space */
					FLASH_If_Erase(APPLICATION_2, USER_FLASH_END_ADDRESS);

					/* Copy old firmware (APPLICATION_1) to APPLICATION_2 */
					FLASH_If_Write(APPLICATION_2,(uint32_t*) APPLICATION_1, USER_FLASH_SIZE / 4);

					/* Free up APPLICATION_1 space */
					FLASH_If_Erase(APPLICATION_1, APPLICATION_2);

					/* Download new firmware and write to APPLICATION_1 */
					if (!DownloadFile(fileName, m95.FTP.FileSize + 4096*totalFile)) {
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
						m95.FTP.DataMode = 0;
						m95.AtCommand.status = Module_Idle;
						FLASH_If_Erase(APPLICATION_1, APPLICATION_2);

						/* Copy old firmware (APPLICATION_1) to APPLICATION_2 */
						FLASH_If_Write(APPLICATION_1, (uint32_t*) APPLICATION_2,
								USER_FLASH_SIZE / 4);
						strcpy(smsMessage, "Firmware update failed\nERROR:1");

						status = 0;

					}

//					HAL_NVIC_SystemReset();
				} else {
					m95.FTP.DataMode = 0;
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
					strcpy(smsMessage, "Firmware update failed\nERROR:2");
					status = 0;
				}

			}
			else{
				m95.FTP.DataMode = 0;
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
				status = 0;
				strcpy(smsMessage, "Firmware update failed\nERROR:3");
			}
		} else {
			m95.FTP.DataMode = 0;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
			status = 0;
			strcpy(smsMessage, "Firmware update failed\nERROR:4");

		}

		printf(smsMessage);
		printf("\r\n");
		M95_SMSSend(phoneNum, smsMessage);
		HAL_Delay(500);
	}

	/* Run application */
	JumpAddress = *(__IO uint32_t*) (APPLICATION_1 + 4);
	JumpToApplication = (pFunction) JumpAddress;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) APPLICATION_1);
	JumpToApplication();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void Read_FTPConfig(void) {
	uint32_t address = 0;
	uint8_t buffer[32] = { 0 };
	uint8_t x = 0;

	/* Read FTP IP address */
	for (address = FTP_IP, x = 0; address < FTP_IP + 20; address++, x++) {
		if (*(__IO uint8_t*) address == 0||*(__IO uint8_t*) address == 0xFF)
			break;
		buffer[x] = *(__IO uint8_t*) address;
	}
	if (strlen(buffer) > 7) {
		strcpy(ftpIp, buffer);
	}

	printf("FTP IP Address: %s\r\n", ftpIp);
	memset(buffer, 0, sizeof(buffer));

	/* Read FTP port */
	for (address = FTP_PORT, x = 0; address < FTP_PORT + 20; address++, x++) {
		if (*(__IO uint8_t*) address == 0||*(__IO uint8_t*) address == 0xFF)
			break;
		buffer[x] = *(__IO uint8_t*) address;
	}

	if (strlen(buffer) > 1) {
		strcpy(ftpPort, buffer);
	}
	printf("FTP Port: %s\r\n", ftpPort);
	memset(buffer, 0, sizeof(buffer));

	/* Read firmware file name */
	for (address = FTP_FILENAME, x = 0; address < FTP_FILENAME + 32;
			address++, x++) {
		if (*(__IO uint8_t*) address == 0||*(__IO uint8_t*) address == 0xFF||*(__IO uint8_t*) address == 0x0D)
			break;
		buffer[x] = *(__IO uint8_t*) address;
	}

	if (strlen(buffer) > 3) {
		uint8_t parseFileName = 1;
		uint x = 0;
		uint8_t buffer2[5] = {0};
		for(int i = 0; i<strlen(buffer);i++){

			if(parseFileName){
				fileName[i] = buffer[i];
			}
			else{
				if(buffer[i]=='.')continue;
				buffer2[i - x] = buffer[i];
			}
			if(buffer[i]=='n'&&buffer[i+1]=='.'){
				parseFileName = 0;
				x = i + 2;
			}
		}
		totalFile = atoi(buffer2);
		//strcpy(fileName, buffer);
	}
	memset(buffer, 0, sizeof(buffer));

	/* Read sender phone number */
	for (address = PHONE_NUMBER, x = 0; address < PHONE_NUMBER + 32;
			address++, x++) {
		if (*(__IO uint8_t*) address == 0||*(__IO uint8_t*) address == 0xFF||*(__IO uint8_t*) address == 0x0D)
			break;
		buffer[x] = *(__IO uint8_t*) address;
	}
	if (strlen(buffer) > 7) {
		strcpy(phoneNum, buffer);
	}
	printf("Sender Phone Number: %s\r\n", phoneNum);



	printf("File name: %s\r\n", fileName);
	printf("Total file: %d\r\n", totalFile);

}

uint8_t DownloadFile(uint8_t *fileName, uint32_t fileSize) {

	uint16_t totalPacket = 0;
	uint16_t packetDataIndex = 0;
	uint32_t flashDestination = 0;
	uint32_t CRCFile = 0;
	uint32_t CRCCalculated = 0;
	uint16_t packetLength = 4096;
	uint8_t status = 0;
	uint32_t ramsource = 0;
	uint32_t timeout = 0;
	uint32_t timeout2 = 0;
	uint8_t buffer[100] = {0};

	/* data dari ftp dibagi menjadi kibibytes packet */
	m95.FTP.TotalPacket = fileSize / 4096;

	/* Set APPLICATION_1 as starting point to be flash */
	flashDestination = APPLICATION_1;

	/* Request to get file from FTP server */


	printf("\nDownload new firmware\r\n");

	timeout = HAL_GetTick();
	timeout2 = timeout;
	while (m95.FTP.PacketIndex <= m95.FTP.TotalPacket) {
		if(HAL_GetTick() - timeout2> 30000)break;


		sprintf(buffer, "%s.%d",fileName,m95.FTP.PacketIndex);
		if (M95_FTPGetFile(buffer)==true) {

			timeout2 = HAL_GetTick();
			while(HAL_GetTick() - timeout< 90000){
				if (m95.FTP.PacketReady) {

					m95.FTP.PacketReady = 0;
					if (m95.FTP.PacketIndex == m95.FTP.TotalPacket) {
						packetLength = (fileSize % 4096) - 4;

						CRCFile =
								((uint32_t) m95.FTP.FtpPacket[packetLength])
										| ((uint32_t) m95.FTP.FtpPacket[packetLength
												+ 1]) << 8
										| ((uint32_t) m95.FTP.FtpPacket[packetLength
												+ 2]) << 16
										| ((uint32_t) m95.FTP.FtpPacket[packetLength
												+ 3]) << 24;
					}
					ramsource =
							(uint32_t) & m95.FTP.FtpPacket;
					if (FLASH_If_Write(flashDestination, (uint32_t*) ramsource,
							packetLength / 4) == FLASHIF_OK) {
		//					printf("write ok\r\n");
						timeout = HAL_GetTick();
						memset(m95.FTP.FtpPacket, 0,sizeof(m95.FTP.FtpPacket));
						flashDestination += packetLength;
						m95.FTP.PacketIndex++;
						printf("Update process %lu of %lu\r\n",m95.FTP.PacketIndex,  m95.FTP.TotalPacket+1);
						break;
					} else
						break;

				}
			}
		}
		else break;

	}
	/* Perform CRC checksum */
	CRCCalculated = HAL_CRC_Calculate(&hcrc, (uint32_t*) APPLICATION_1,
			(fileSize - 4) / 4);
	printf("CRC File: %lu\r\n", CRCFile);
	printf("CRC Calculated: %lu\r\n", CRCCalculated);
	if (CRCCalculated == CRCFile) {
		status = 1;
	}
	return status;

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

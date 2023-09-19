/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
//#include "portable.h"
#include "fatfs.h"
#include <stdio.h>

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* Definitions for defaultTask */

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static const int rate_1 = 500;

void toggleLED_1(void *parameter) {
  while(1) {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    vTaskDelay(rate_1 / portTICK_PERIOD_MS);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    vTaskDelay(rate_1 / portTICK_PERIOD_MS);
  }
}

#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

static const int8_t * const pcWelcomeMessage =
  "FreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n";

BaseType_t xCommandTest(char *pcWriteBuffer,
		size_t xWriteBufferLen,
		const char *pcCommandString)
{
//	printf("I am a dummy command\r\n");

	//assumbe buffer is big enough
	memset(pcWriteBuffer,'\0',xWriteBufferLen);
	//pcWriteBuffer = "I am a dummy\r\n";
	strcpy(pcWriteBuffer,"I am a dummy\r\n");
	return pdFALSE;
}

static const CLI_Command_Definition_t xTestCmd =
{
		"test",
		"\r\ntest:\r\n Prints stuff\r\n\r\n",
		xCommandTest,
		0
};

void vCommandConsoleTask( void *pvParameters )
{
////	Peripheral_Descriptor_t xConsole;
	int8_t cRxedChar, cInputIndex = 0;
	BaseType_t xMoreDataToFollow;
//	/* The input and output buffers are declared static to keep them off the stack. */
	static int8_t pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];
//
//	/* This code assumes the peripheral being used as the console has already
//    been opened and configured, and is passed into the task as the task
//    parameter.  Cast the task parameter to the correct type. */
//	//xConsole = ( Peripheral_Descriptor_t ) pvParameters;

//	/* Send a welcome message to the user knows they are connected. */
//	FreeRTOS_write( xConsole, pcWelcomeMessage, strlen( pcWelcomeMessage ) );
	printf(pcWelcomeMessage);
	for( ;; )
	{
//		/* This implementation reads a single character at a time.  Wait in the
//        Blocked state until a character is received. */
//		FreeRTOS_read( xConsole, &cRxedChar, sizeof( cRxedChar ) );
		cRxedChar = getchar();
		if( cRxedChar == '\r' )
		{
//			printf("aah\r\n");
			/* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
			//FreeRTOS_write( xConsole, "\r\n", strlen( "\r\n" );
			putchar('\r');
			putchar('\n');

			/* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            exaplanation of why this is. */
			do
			{
				/* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
				//printf("The command str:\r\n");
				//printf(pcInputString);
				//printf("\r\n---\r\n");
				xMoreDataToFollow = FreeRTOS_CLIProcessCommand
						(
								pcInputString,   /* The command string.*/
								pcOutputString,  /* The output buffer. */
								MAX_OUTPUT_LENGTH/* The size of the output buffer. */
						);

				/* Write the output generated by the command interpreter to the
                console. */
				printf(pcOutputString);
				//FreeRTOS_write( xConsole, pcOutputString, strlen( pcOutputString ) );

			} while( xMoreDataToFollow != pdFALSE );


			/* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
			cInputIndex = 0;
			memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
		}
		else
		{
			/* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

			if( cRxedChar == '\n' )
			{
				/* Ignore carriage returns. */
				//printf("oh?\r\n");
			}
			else if( cRxedChar == '\b' )
			{
				/* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
				if( cInputIndex > 0 )
				{
					cInputIndex--;
					pcInputString[ cInputIndex ] = '\0';
				}
			}
			else
			{
				/* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
				if( cInputIndex < MAX_INPUT_LENGTH )
				{
					pcInputString[ cInputIndex ] = cRxedChar;
					cInputIndex++;
				}
			}
		}
	}
}

prvSetupHardware();
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	setvbuf(stdin, NULL, _IONBF, 0);
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
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  FATFS fs;           /* Filesystem object */
  FIL fil;            /* File object */
  FRESULT res;        /* API result code */
  UINT bw;            /* Bytes written */
  BYTE work[1024]; /* Work area (larger is better for processing time) */
  ///how big should work be?

//  FRESULT f_mkfs (const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len);	/* Create a FAT volume */


  /* Format the default drive with default parameters */
//  printf("about to make fs...\r\n");
//  res = f_mkfs("0",FM_ANY,0, work, sizeof work);
//  printf("mkfs res:%u\r\n",res);

  /* Register work area */
  res = f_mount(&fs, "", 0);
  printf("mnt res:%u\r\n",res);

  /* Create a file as new */
  res = f_open(&fil, "hello.txt", FA_CREATE_NEW | FA_WRITE);
  printf("open res:%u\r\n",res);

  /* Write a message */
  res = f_write(&fil, "Hello, World!\r\n", 15, &bw);
  printf("write res:%u\r\n",res);

  /* Close the file */
  f_close(&fil);

  FRESULT fr;
  FILINFO fno;
  const char *fname = "hello.txt";


  printf("Test for \"%s\"...\r\n", fname);

  fr = f_stat(fname, &fno);
  switch (fr) {

  case FR_OK:
      printf("Size: %lu\r\n", fno.fsize);
      printf("Timestamp: %u-%02u-%02u, %02u:%02u\r\n",
             (fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
             fno.ftime >> 11, fno.ftime >> 5 & 63);
      printf("Attributes: %c%c%c%c%c\r\n",
             (fno.fattrib & AM_DIR) ? 'D' : '-',
             (fno.fattrib & AM_RDO) ? 'R' : '-',
             (fno.fattrib & AM_HID) ? 'H' : '-',
             (fno.fattrib & AM_SYS) ? 'S' : '-',
             (fno.fattrib & AM_ARC) ? 'A' : '-');
      break;

  case FR_NO_FILE:
      printf("\"%s\" is not exist.\r\n", fname);
      break;

  default:
      printf("An error occured. (%d)\n", fr);
  }


  /* Unregister work area */
  f_mount(0, "", 0);



  //DSTATUS stat = disk_initialize(0);
  //printf("init:%u\r\n",stat);

  //DSTATUS stat = USER_initialize(0);
  //printf("init:%u\r\n",stat);

  //stat = USER_status(0);
  //printf("init:%u\r\n",stat);

  //vTaskStartScheduler();

  /* USER CODE END 2 */

  /* Init scheduler */
  //osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  //defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  //osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//  xTaskCreate(  // Use xTaskCreate() in vanilla FreeRTOS
//              toggleLED_1,  // Function to be called
//              "Toggle 1",   // Name of task
//              1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
//              NULL,         // Parameter to pass to function
//              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
//              NULL);         // Task handle
  xTaskCreate(  // Use xTaskCreate() in vanilla FreeRTOS
		  	  vCommandConsoleTask,  // Function to be called
              "Toggle 1",   // Name of task
              1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
              NULL,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              NULL);         // Task handle

  FreeRTOS_CLIRegisterCommand( (const) &xTestCmd);

  vTaskStartScheduler();
  printf("If we're here, theres a problem\r\n");
  char strBuf[50] = {0};
  char c = 0;
  printf("getchar\r\n");
  while (1)
  {

//	scanf("%s",strBuf);
//	printf("Heres your command:%s\r\n",strBuf);

	  c = getchar();
	  printf("Heres your char:%c\r\n",c);


	  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	  HAL_Delay(100);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

GETCHAR_PROTOTYPE
{
	//__HAL_UART_CLEAR_OREFLAG(&huart2);

	uint8_t _char;

	HAL_UART_Receive(&huart2, (uint8_t *)&_char, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t *)&_char, 1, HAL_MAX_DELAY);

	//if (_char == '\r') {
	//	unsigned char repl = '\n';
	//	HAL_UART_Transmit(&huart2, &repl, 1, HAL_MAX_DELAY);
	//}

	return _char;

}

uint8_t fixedPointLog2(uint8_t val){
	uint8_t r = 0;
	while (val >>= 1) // unroll for more speed...
	{
	  r++;
	}
	return r;
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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


/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
CAN_HandleTypeDef hcan;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CAN_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
CAN_RxHeaderTypeDef receiver_structure;
CAN_TxHeaderTypeDef TxHeader;

uint8_t Transmitted_Data[8];
uint32_t TxMailbox;

uint8_t RxData[8];
uint32_t sender_info[5];

uint8_t receivedData[259]; // RECEIVE DATA FROM BMS
uint8_t serialData[8] = { 129, 3, 0, 0, 0, 127, 27, 234 };
uint8_t serialData2[8] = { 129, 3, 0, 0, 0, 127, 27, 234 };

uint8_t arduinoTransmit[53];

uint32_t count;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	receivedData[0] = 0;
	receivedData[1] = 0;
	receivedData[2] = 0;
	receivedData[3] = 0;

	serialData[0] = serialData[0];
	receivedData[0] = receivedData[0];
	arduinoTransmit[0] = arduinoTransmit[0];
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
  MX_DMA_Init();
  MX_CAN_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_CAN_Start(&hcan);
   HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

   TxHeader.DLC = 8;
   TxHeader.IDE = CAN_ID_STD;
   TxHeader.RTR = CAN_RTR_DATA;
   TxHeader.StdId = 0X321;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  		  // SEND TO BMS
	  		  HAL_UART_Transmit(&huart2, serialData2, sizeof(serialData2), 1000);

	  		  // GET THE ANSWER FROM BMS
	  		  HAL_UART_Receive(&huart2, receivedData, sizeof(receivedData), 1000);

	  		  HAL_Delay(100);

	  		  // SUM VOTAGE
	  		  arduinoTransmit[0] = receivedData[115];
	  		  arduinoTransmit[1] = receivedData[116];



	  		  // SOC
	  		  arduinoTransmit[2] = receivedData[119];
	  		  arduinoTransmit[3] = receivedData[120];

	  		  //POWER WATT
	  		  arduinoTransmit[4] = receivedData[179];
	  		  arduinoTransmit[5] = receivedData[180];

	  		  // CELLS VOLTAGE
	  		  for(int i = 0; i < 40; i++) {
	  			  arduinoTransmit[i + 6] = receivedData[i + 3];
	  		  }


	  		  // TEMPS (SICAKLIKLAR)
	  		  arduinoTransmit[46] = receivedData[100];
	  		  arduinoTransmit[47] = receivedData[102];
	  		  arduinoTransmit[48] = receivedData[104];
	  		  arduinoTransmit[49] = receivedData[106];

	  		  // MAX TEMP
	  		  arduinoTransmit[50] = receivedData[108];

	  		  // CURRENT
	  		  arduinoTransmit[51] = receivedData[179];
	  		  arduinoTransmit[52] = receivedData[180];

	  		  // SEND TO ARDUINO
//	  		  HAL_UART_Transmit(&huart1, arduinoTransmit, sizeof(arduinoTransmit), 1000);





	  		  // Sicaklik 2 tane
	  		  HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &receiver_structure, RxData);
	  		  HAL_Delay(90);
	  		  if(receiver_structure.StdId == 123 && RxData[0] == 1) {
	  			  Transmitted_Data[0] = 101;
	  			  // Temps
	  			  Transmitted_Data[1] = arduinoTransmit[46];
	  			  Transmitted_Data[2] = arduinoTransmit[47];
	  			  Transmitted_Data[3] = arduinoTransmit[48];
	  			  Transmitted_Data[4] = arduinoTransmit[49];
	  			  // Max Temp
	  			  Transmitted_Data[5] = arduinoTransmit[50];
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }


	  		  if(receiver_structure.StdId == 123 && RxData[0] == 2) {
	  			  Transmitted_Data[0] = 102;
	  			  // SUM VOTAGE
	  			  Transmitted_Data[1] = arduinoTransmit[0];
	  			  Transmitted_Data[2] = arduinoTransmit[1];

	  			  // SOC
	  			  Transmitted_Data[3] = arduinoTransmit[2];
	  			  Transmitted_Data[4] = arduinoTransmit[3];

	  			  // POWER WATT
	  			  Transmitted_Data[5] = arduinoTransmit[4];
	  			  Transmitted_Data[6] = arduinoTransmit[5];
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 3) {
	  			  Transmitted_Data[0] = 103;
	  			  // CELLS VOLTAGE
	  			  for(int i = 0; i < 6; i++){
	  				  Transmitted_Data[i+1]=arduinoTransmit[i+6];
	  			  }
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 4) {
	  			  Transmitted_Data[0] = 104;

	  			  for(int i = 0; i < 6; i++){
	  				  Transmitted_Data[i+1]=arduinoTransmit[i+12];
	  			  }

	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }


	  		  if(receiver_structure.StdId == 123 && RxData[0] == 5) {
	  			  Transmitted_Data[0]=105;
	  			 for(int i=0; i<6; i++) {
	  				 Transmitted_Data[i+1]=arduinoTransmit[i+18];

	  			 }
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 6) {
	  			  Transmitted_Data[0]=106;
	  			 for(int i=0; i<6; i++) {
	  				 Transmitted_Data[i+1]=arduinoTransmit[i+24];
	  			 }
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 7) {
	  			  Transmitted_Data[0]=107;
	  			 for(int i=0; i<6; i++) {
	  				 Transmitted_Data[i+1]=arduinoTransmit[i+30];
	  			 }
	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 8) {
	  			  Transmitted_Data[0]=108;
	  			 for(int i=0; i<6; i++) {
	  				 Transmitted_Data[i+1]=arduinoTransmit[i+36];
	  			 }

	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }

	  		  if(receiver_structure.StdId == 123 && RxData[0] == 9) {
	  			  Transmitted_Data[0]=109;
	  			 for(int i=0; i<4; i++) {
	  				 Transmitted_Data[i+1]=arduinoTransmit[i+42];
	  			 }

	  			  HAL_CAN_AddTxMessage(&hcan, &TxHeader, Transmitted_Data, &TxMailbox);
	  		  }
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 9;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_3TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */
  CAN_FilterTypeDef can_filter_structure;

  can_filter_structure.FilterActivation = CAN_FILTER_ENABLE;
  can_filter_structure.FilterBank = 13;
  can_filter_structure.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  can_filter_structure.FilterIdHigh = 0;
  can_filter_structure.FilterIdLow = 0;
  can_filter_structure.FilterMaskIdHigh = 0x0000;
  can_filter_structure.FilterMaskIdLow = 0x0000;
  can_filter_structure.FilterMode = CAN_FILTERMODE_IDMASK;
  can_filter_structure.FilterScale = CAN_FILTERSCALE_32BIT;
  can_filter_structure.SlaveStartFilterBank = 0;

  HAL_CAN_ConfigFilter(&hcan, &can_filter_structure);
  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 9600;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

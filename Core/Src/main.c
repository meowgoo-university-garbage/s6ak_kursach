/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"

#include "driver_display.h"
#include "driver_display_characters.h"

#include "driver_sensor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	NAV_MENU,

	NAV_TYPERACER_SETTINGS,
	NAV_TYPERACER_PLAYING,
	NAV_TYPERACER_RESULTS,
} NavState;

typedef struct {
	bool started;
	bool displayedSelection;
	int gameSelected;
} MenuState;

typedef struct {
	char *name;
	NavState initState;
} MenuGame;

typedef struct {
	bool moving;
	bool overwrite;
	uint8_t text;
} TyperacerSettings;

typedef struct {
	char *text;
	size_t textLength;
	size_t textConsumed;

	int currentShift;
	bool shiftActivated;
	int charactersTyped;
	int hiddenStart;

	bool displayedNewLine;

	int mistakeCount;
} TyperacerState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern void initialise_monitor_handles();
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

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
  //initialise_monitor_handles();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM4_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);

  Display display = {
		.config = {
		  	.twoLinesInsteadOfOne = true,
		  	.tallFont = false,

		  	.displayOn = true,
		  	.cursorVisible = true,
		  	.cursorBlinking = true,

		  	.incrementInsteadOfDecrement = true,
		  	.shiftOnEntry = false
		 },
		.handle = &hi2c1,
    	.backlight = true,
  };

  display_init(&display);


  Sensor sensor = {
		.gpiox = GPIOC,
		.pin = GPIO_PIN_7,
		.timer = &htim4
  };
  HAL_TIM_Base_Start(sensor.timer);


  NavState navState = NAV_MENU;

  TyperacerSettings settings_tr = {
		  .moving = false,
		  .overwrite = true,
		  .text = 0,
  };
  TyperacerState state_tr = {0};

  MenuState state_menu = {0};

  MenuGame games[] = {
		  {
				.name = "Typeracer",
				.initState = NAV_TYPERACER_PLAYING,
		  },
  };
  int gameCount = sizeof(games) / sizeof(MenuGame);


  extern uint8_t CDC_buffer[1024];
  extern uint32_t CDC_length;
  extern uint8_t CDC_ready;

  bool firstState = true;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  bool justChangedState = firstState;
	  firstState = false;

#define CHANGE_STATE(state) do { \
		  navState = state; \
		  justChangedState = true; \
		  goto changeState; \
  } while(0)

	  changeState:
	  switch(navState) {
	  case NAV_MENU:
		  if(justChangedState) {
			  display_instruction_displayOnOffControl(&display, true, false, false);
			  state_menu.displayedSelection = false;
		  }

		  if(!state_menu.displayedSelection) {
			  display_instruction_clearDisplay(&display);

			  if(!state_menu.started) {
				  char buffer[256];
				  int len;
				  len = sprintf(buffer, "AbobaGameStation");
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
	  			  display_writeString(&display, buffer, len);
				  len = sprintf(buffer, "<SPACE> TO START");
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
	  			  display_writeString(&display, buffer, len);
			  }
			  else {
				  char buffer[256];
				  int len;
				  len = sprintf(buffer, "%s", games[state_menu.gameSelected]);
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
	  			  display_writeString(&display, buffer, len);
				  len = sprintf(buffer, "  <    %d/%d   >  ", state_menu.gameSelected + 1, gameCount);
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
	  			  display_writeString(&display, buffer, len);
			  }

			  state_menu.displayedSelection = true;
		  }

		  if(!CDC_ready) {
			  continue;
		  }
		  CDC_ready = false;

		  if(!state_menu.started) {
			  if(CDC_buffer[0] == ' ') {
				  state_menu.started = true;

				  // TODO: animation

				  state_menu.displayedSelection = false;
			  }

			  continue;
		  }

		  if(0){}
		  else if(CDC_buffer[0] == '<') {
			  state_menu.gameSelected -= 1;
			  state_menu.displayedSelection = false;
		  }
		  else if(CDC_buffer[0] == '>') {
			  state_menu.gameSelected += 1;
			  state_menu.displayedSelection = false;
		  }
		  else if(CDC_buffer[0] == ' ') {
			  CHANGE_STATE(games[state_menu.gameSelected].initState);
		  }

		  if(state_menu.gameSelected < 0) {
			  state_menu.gameSelected += gameCount;
		  }
		  else if(state_menu.gameSelected >= gameCount) {
			  state_menu.gameSelected -= gameCount;
		  }
		  break;
	  case NAV_TYPERACER_PLAYING:
  		  if(justChangedState) {
  			  if(settings_tr.moving && settings_tr.overwrite) {
  				  settings_tr.overwrite = false;
  			  }

			  display_instruction_displayOnOffControl(&display, true, true, true);
			  display_instruction_entryModeSet(&display, true, false);

  			  state_tr = (TyperacerState){0};
  			  state_tr.text =
  					  "Once upon a time, a LEGEND was whispered among shadows. "
  					  "It was a LEGEND of HOPE. "
  					  "It was a LEGEND of DREAMS. "
  					  "It was a LEGEND of LIGHT. "
  					  "It was a LEGEND of DARK. "
  					  "This is the legend of DELTA RUNE.";
  			  //state_tr.text = "amogus";
  			  state_tr.textLength = strlen(state_tr.text);

  			  if(settings_tr.moving) {
  				  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  				  display_writeString(&display, state_tr.text, min(state_tr.textLength, DISPLAY_LINE_LEN));
  				  state_tr.textConsumed += min(state_tr.textLength, DISPLAY_LINE_LEN);

  				  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  }
  		  }

  		  if(state_tr.charactersTyped >= state_tr.textLength) {
  			  CHANGE_STATE(NAV_TYPERACER_RESULTS);
  		  }

  		  if(!settings_tr.moving && !state_tr.displayedNewLine) {
  			  state_tr.displayedNewLine = true;

			  display_instruction_clearDisplay(&display);

			  char buffer[256];
			  int len;
			  len = sprintf(buffer, "[%-.*s]", DISPLAY_VISIBLE_LINE_LEN - 2, &state_tr.text[state_tr.textConsumed]);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  			  display_writeString(&display, buffer, len);

			  state_tr.textConsumed += (DISPLAY_VISIBLE_LINE_LEN - 2);
			  if(state_tr.textConsumed > state_tr.textLength) {
				  state_tr.textConsumed = state_tr.textLength;
			  }

			  len = sprintf(buffer, "[%-*s]", DISPLAY_VISIBLE_LINE_LEN - 2, "");
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  display_writeString(&display, buffer, len);

  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + 1);
  		  }

		  if(!CDC_ready) {
			  continue;
		  }
		  CDC_ready = 0;

		  for(int i = 0; i < CDC_length; i++) {
		  		  if(state_tr.charactersTyped == state_tr.textLength) {
		  			  break;
		  		  }

		  		  if(CDC_buffer[i] != state_tr.text[state_tr.charactersTyped]) {
		  			  state_tr.mistakeCount += 1;
		  			  continue;
		  		  }

		  		  display_writeChar(&display, CDC_buffer[i]);
		  		  state_tr.charactersTyped += 1;

		  		  if(settings_tr.moving) {
		  			  if(state_tr.charactersTyped % DISPLAY_LINE_LEN == 0) {
						  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
					  }

					  if(state_tr.shiftActivated) {
						  state_tr.currentShift += 1;
						  state_tr.currentShift %= DISPLAY_LINE_LEN;
					  }

					  if((state_tr.currentShift + DISPLAY_VISIBLE_LINE_LEN) % DISPLAY_LINE_LEN == state_tr.hiddenStart) {
						  int len = DISPLAY_LINE_LEN - DISPLAY_VISIBLE_LINE_LEN;

						  display_instruction_entryModeSet(&display, true, false);

						  {
							  int textRemaining = state_tr.textLength - state_tr.textConsumed;
							  int textLength = min(len, textRemaining);
							  int spaceLength = len - textLength;
							  display_writeStringOnLine(&display, 0, state_tr.hiddenStart, &state_tr.text[state_tr.textConsumed], textLength);
							  state_tr.textConsumed += textLength;
							  int pos = (state_tr.hiddenStart + textLength) % DISPLAY_LINE_LEN;
							  for(int i = 0; i < spaceLength; i++)  {
								  pos = display_writeCharOnLine(&display, 0, pos, ' ', true);
							  }
						  }

						  int pos = state_tr.hiddenStart;
						  display_instruction_setDisplayRamAddress(&display, pos + DISPLAY_LINE_1_MIN);
						  for(int i = 0; i < len; i++)  {
							  pos = display_writeCharOnLine(&display, 1, pos, ' ', true);
						  }

						  display_instruction_setDisplayRamAddress(&display, (state_tr.charactersTyped % DISPLAY_LINE_LEN) + DISPLAY_LINE_1_MIN);

						  display_instruction_entryModeSet(&display, true, state_tr.shiftActivated);
						  state_tr.hiddenStart = state_tr.currentShift;
						  state_tr.hiddenStart %= DISPLAY_LINE_LEN;
					  }

					  if(!state_tr.shiftActivated && state_tr.charactersTyped > 4) {
						  state_tr.shiftActivated = true;

						  display_instruction_entryModeSet(&display, true, state_tr.shiftActivated);
					  }
		  		  }
		  		  else {
#define OVERWRITE_DELAY 2
		  			  if(settings_tr.overwrite && state_tr.textConsumed < state_tr.textLength && state_tr.charactersTyped > OVERWRITE_DELAY) {
		  				  int index = (state_tr.currentShift - OVERWRITE_DELAY + (DISPLAY_VISIBLE_LINE_LEN - 2)) % (DISPLAY_VISIBLE_LINE_LEN - 2);

	  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN + 1 + index);
	  		  			  display_writeChar(&display, state_tr.text[state_tr.textConsumed]);
	  		  			  state_tr.textConsumed += 1;

	  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + 2 + state_tr.currentShift);
		  			  }

		  			  state_tr.currentShift += 1;

		  			  if(state_tr.currentShift >= (DISPLAY_VISIBLE_LINE_LEN - 2)) {
		  				  state_tr.currentShift = 0;

		  				  if(!settings_tr.overwrite) {
			  				  state_tr.displayedNewLine = false;
		  				  }
		  				  else {
		  					  char buffer[256];
		  					  int len;
		  					  len = sprintf(buffer, "[%-*s]", DISPLAY_VISIBLE_LINE_LEN - 2, "");
		  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
		  		  			  display_writeString(&display, buffer, len);

		  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + 1);
		  				  }
		  			  }
		  		  }

		  	  }
		  break;
	  case NAV_TYPERACER_RESULTS:
		  if(justChangedState) {
			  display_instruction_clearDisplay(&display);
			  display_instruction_entryModeSet(&display, true, false);
			  display_instruction_displayOnOffControl(&display, true, false, false);

			  char buffer[256];
			  size_t len;
			  len = sprintf(buffer, "GG! Time: %ds", 5);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  			  display_writeString(&display, buffer, len);
			  len = sprintf(buffer, "Mistakes: %d", state_tr.mistakeCount);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  display_writeString(&display, buffer, len);
		  }


		  if(!CDC_ready) {
			  continue;
		  }
		  CDC_ready = 0;


		  if(CDC_buffer[0] == ' ') {
			  CHANGE_STATE(NAV_MENU);
		  }

		  break;
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
#ifdef USE_FULL_ASSERT
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

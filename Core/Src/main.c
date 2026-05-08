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

	NAV_DINO_PLAYING,
	NAV_DINO_RESULTS,

	NAV_MATH_PLAYING,
	NAV_MATH_RESULTS,
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

typedef struct {
	float speed;

	size_t time_passed;
	size_t time_next;
	size_t time_step;

	size_t time_lastCrouch;
	size_t time_lastJump;
	uint8_t lastState;

	uint16_t pteros;
	uint16_t cacti;

	int pos;
	int frame;
} DinoState;

typedef struct {
	uint8_t task_total;
	uint8_t task_done;
	uint8_t task_correct;

	uint8_t correctAnswer;
	int8_t selection;
	bool displayedTask;
} MathState;

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

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */


extern uint8_t CDC_buffer[1024];
extern size_t CDC_index_lo;
extern size_t CDC_index_hi;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

size_t getInputAmount() {
	if(CDC_index_lo <= CDC_index_hi) {
		return (CDC_index_hi - CDC_index_lo);
	}
	else {
		return (sizeof(CDC_buffer) - CDC_index_lo) + (CDC_index_hi);
	}
}

bool tryReadInput(uint8_t *dst, int amount, bool consume) {
	if(getInputAmount() < amount) return false;
	size_t preserve = CDC_index_lo;

	for(int i = 0; i < amount; i++) {
		dst[i] = CDC_buffer[CDC_index_lo];
		CDC_index_lo += 1;

		if(CDC_index_lo >= sizeof(CDC_buffer)) {
			CDC_index_lo = 0;
		}
	}

	if(!consume) {
		CDC_index_lo = preserve;
	}

	return true;
}


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
  MX_TIM2_Init();
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
  HAL_TIM_Base_Start(&htim2);


  NavState navState = NAV_MENU;

  TyperacerSettings settings_tr = {
		  .moving = false,
		  .overwrite = true,
		  .text = 0xff,
  };
  TyperacerState state_tr = {0};
  DinoState state_dino = {0};
  MathState state_math = {0};
  MenuState state_menu = {0};

  char *texts[] = {
		  "Once upon a time, a LEGEND was whispered among shadows. "
		  "It was a LEGEND of HOPE. "
		  "It was a LEGEND of DREAMS. "
		  "It was a LEGEND of LIGHT. "
		  "It was a LEGEND of DARK. "
		  "This is the legend of DELTA RUNE.",

		  "Born into a world without anime, "
		  "Where the sun doesn't shine, there's no sanity. "
		  "We are left alone with these killing machines. "
		  "We have no home, we have no dreams. "
		  "Crushed under the weight of the mechanical demon, "
		  "We are victims of fate from the power of semen.",

		  "I think Skyblock at its core is the ultimate challenge in resource management. "
		  "You spawn on a tiny island in an empty universe. "
		  "All you have is a tree, some supplies and some dirt to stand on. "
		  "You have to treasure EVERY dirt block, because if one falls into the void, "
		  "there's no way to replace it and as you carefully navigate your absurd circumstance, "
		  "you gain a new appreciation for the few things you have as you meticulously use them to their fullest effect. "
		  "With nothing but some ice, lava and saplings you slowly transform this empty expanse into a world of your very own. "
		  "Skyblock teaches us that no matter how ridiculous the odds may seem, "
		  "within us resides the power to overcome these challenges and achieve something beautiful. "
		  "That one day, we'll look back at where we started and be amazed "
		  	  "by how far we've come.",

		  "amogus sus sus amogus amogus sus sus amogus",

		  "I've seen your kind, time and time again. "
		  "Every fleeing man must be caught. Every secret must be unearthed. "
		  "Such is the conceit of the self-proclaimed seeker of truth. "
		  "But in the end, you lack the stomach. "
		  "For the agony you'll bring upon yourself...",
  };


  MenuGame games[] = {
		  {
				.name = "Typeracer",
				.initState = NAV_TYPERACER_PLAYING,
		  },
		  {
				.name = "Dinosaur",
				.initState = NAV_DINO_PLAYING,
		  },
		  {
				.name = "Math quiz",
				.initState = NAV_MATH_PLAYING,
		  },
  };
  int gameCount = sizeof(games) / sizeof(MenuGame);



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

	  char input;

	  char buffer[256];
	  int len;

	  if(tryReadInput(&input, 1, false) && input == '\e') {
		  CHANGE_STATE(NAV_MENU);
	  }

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
				  len = sprintf(buffer, "AbobaGameStation");
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
	  			  display_writeString(&display, buffer, len);
				  len = sprintf(buffer, "<SPACE> TO START");
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
	  			  display_writeString(&display, buffer, len);
			  }
			  else {
				  len = sprintf(buffer, "%s", games[state_menu.gameSelected]);
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
	  			  display_writeString(&display, buffer, len);
				  len = sprintf(buffer, "  <    %d/%d   >  ", state_menu.gameSelected + 1, gameCount);
	  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
	  			  display_writeString(&display, buffer, len);
			  }

			  state_menu.displayedSelection = true;
		  }


		  if(!tryReadInput((uint8_t *)&input, 1, true)) continue;


		  if(!state_menu.started) {
			  if(input == ' ') {
				  state_menu.started = true;

				  // TODO: animation

				  state_menu.displayedSelection = false;
			  }

			  continue;
		  }

		  if(0){}
		  else if(input == '<') {
			  state_menu.gameSelected -= 1;
			  state_menu.displayedSelection = false;
		  }
		  else if(input == '>') {
			  state_menu.gameSelected += 1;
			  state_menu.displayedSelection = false;
		  }
		  else if(input == ' ') {
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
			  srand(__HAL_TIM_GET_COUNTER(&htim2));

  			  if(settings_tr.moving && settings_tr.overwrite) {
  				  settings_tr.overwrite = false;
  			  }

  			 __HAL_TIM_SET_COUNTER(&htim2, 0);

			  display_instruction_displayOnOffControl(&display, true, true, true);
			  display_instruction_entryModeSet(&display, true, false);

  			  state_tr = (TyperacerState){0};

  			  if(settings_tr.text == 0xff) {
  				  state_tr.text = texts[rand() % (sizeof(texts) / sizeof(char *))];
  			  }
  			  else {
  				  state_tr.text = texts[settings_tr.text];
  			  }

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


		  while(tryReadInput(&input, 1, true)) {
		  		  if(state_tr.charactersTyped == state_tr.textLength) {
		  			  break;
		  		  }

		  		  if(input != state_tr.text[state_tr.charactersTyped]) {
		  			  state_tr.mistakeCount += 1;
		  			  continue;
		  		  }

		  		  display_writeChar(&display, input);
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

			  int time = __HAL_TIM_GET_COUNTER(&htim2);

			  int sec = time / 1000000;
			  int sec_decimal = (time - (sec * 1000000)) / 100000;

			  len = sprintf(buffer, "GG! Time: %d.%ds", sec, sec_decimal);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  			  display_writeString(&display, buffer, len);
			  len = sprintf(buffer, "Mistakes: %d", state_tr.mistakeCount);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  display_writeString(&display, buffer, len);
		  }


		  if(!tryReadInput(&input, 1, true)) continue;


		  if(input == ' ') {
			  CHANGE_STATE(NAV_MENU);
		  }

		  break;








#define DINO_RUNNING_1 0b10110101
#define DINO_RUNNING_2 0b11001000

#define DINO_JUMPING   0b10110110

#define DINO_CROUCHING 0b11011101

#define DINO_CACTUS    0b10110111
#define DINO_PTERO     0b11101010

	  case NAV_DINO_PLAYING:
		  if(justChangedState) {
			  srand(__HAL_TIM_GET_COUNTER(&htim2));

	  		  __HAL_TIM_SET_COUNTER(&htim2, 0);
			  state_dino = (DinoState){
				  .speed = 1,

				  .time_passed = 0,
				  .time_next = 0,
				  .time_step = 1000,

				  .lastState = DINO_RUNNING_1,

				  .pos = 0,
				  .frame = 0,
			  };

			  display_instruction_clearDisplay(&display);
			  display_instruction_entryModeSet(&display, true, false);
			  display_instruction_displayOnOffControl(&display, true, false, false);
		  }


		  size_t time = __HAL_TIM_GET_COUNTER(&htim2) / 1000;


		  bool hasInput = tryReadInput(&input, 1, true);
		  if(hasInput) {
			  if(input == ' ' && (time - state_dino.time_lastJump > 3500) && (time - state_dino.time_lastCrouch > 500)) {
				  state_dino.time_lastJump = time;
			  }
			  else if(input == 'c' &&  (time - state_dino.time_lastJump > 3500)) {
				  state_dino.time_lastCrouch = time;
			  }
		  }


		  if(state_dino.time_next > time) continue;

		  state_dino.frame += 1;

		  state_dino.time_next += state_dino.time_step;



		  display_writeCharOnLine(&display, 0, state_dino.pos, ' ', false);
		  display_writeCharOnLine(&display, 1, state_dino.pos, ' ', false);



		  state_dino.pteros >>= 1;
		  state_dino.cacti >>= 1;

		  if((state_dino.cacti & 0b1000) && state_dino.lastState != DINO_JUMPING) {
			  CHANGE_STATE(NAV_DINO_RESULTS);
		  }

		  if((state_dino.pteros & 0b1000) && state_dino.lastState != DINO_CROUCHING) {
			  CHANGE_STATE(NAV_DINO_RESULTS);
		  }



		  if(time - state_dino.time_lastJump < 3000) {
			  if(state_dino.lastState == DINO_RUNNING_1) {
				  display_writeCharOnLine(&display, 1, state_dino.pos + 3, ' ', false);
			  }

			  state_dino.lastState = DINO_JUMPING;
			  display_writeCharOnLine(&display, 0, state_dino.pos + 3, ' ', false);
			  state_dino.pos = (state_dino.pos + 1) % DISPLAY_LINE_LEN;
			  display_instruction_cursorOrDisplayShift(&display, true, false);
			  display_writeCharOnLine(&display, 0, state_dino.pos + 3, DINO_JUMPING, false);
		  }
		  else if(time - state_dino.time_lastCrouch < 200) {
			  if(state_dino.lastState == DINO_JUMPING) {
				  display_writeCharOnLine(&display, 0, state_dino.pos + 3, ' ', false);
			  }

			  state_dino.lastState = DINO_CROUCHING;
			  display_writeCharOnLine(&display, 1, state_dino.pos + 3, ' ', false);
			  state_dino.pos = (state_dino.pos + 1) % DISPLAY_LINE_LEN;
			  display_instruction_cursorOrDisplayShift(&display, true, false);
			  display_writeCharOnLine(&display, 1, state_dino.pos + 3, DINO_CROUCHING, false);
		  }
		  else {
			  if(state_dino.lastState == DINO_JUMPING) {
				  display_writeCharOnLine(&display, 0, state_dino.pos + 3, ' ', false);
			  }

			  state_dino.lastState = DINO_RUNNING_1;
			  display_writeCharOnLine(&display, 1, state_dino.pos + 3, ' ', false);
			  state_dino.pos = (state_dino.pos + 1) % DISPLAY_LINE_LEN;
			  display_instruction_cursorOrDisplayShift(&display, true, false);
			  display_writeCharOnLine(&display, 1, state_dino.pos + 3, state_dino.frame % 2 == 0 ? DINO_RUNNING_1 : DINO_RUNNING_2, false);
		  }


		  // TODO: this often results in unwinnable situations
		  {
			  bool cactus = (rand() % 100) < 7;
			  if(cactus) {
				  display_writeCharOnLine(&display, 1, state_dino.pos + 15, DINO_CACTUS, false);
				  state_dino.cacti  |= 0b1000000000000000;
			  }

			  bool ptero = (rand() % 100) < 7;
			  if(ptero && !cactus) {
				  display_writeCharOnLine(&display, 0, state_dino.pos + 15, DINO_PTERO, false);
				  state_dino.pteros |= 0b1000000000000000;
			  }
		  }

		  break;

	  case NAV_DINO_RESULTS:
		  if(justChangedState) {
			  display_writeStringOnLine(&display, 0, state_dino.pos + 15 - 4, " GAME", 5);
			  display_writeStringOnLine(&display, 1, state_dino.pos + 15 - 4, " OVER", 5);
		  }

		  if(!tryReadInput(&input, 1, true)) continue;

		  if(input == ' ') {
			  CHANGE_STATE(NAV_MENU);
		  }

		  break;







	  case NAV_MATH_PLAYING:
		  if(justChangedState) {
			  srand(__HAL_TIM_GET_COUNTER(&htim2));

	  		  __HAL_TIM_SET_COUNTER(&htim2, 0);

			  display_instruction_entryModeSet(&display, true, false);
			  display_instruction_displayOnOffControl(&display, true, true, true);

			  state_math = (MathState){
				  .task_total = 5,
			  };
		  }

		  if(state_math.task_done >= state_math.task_total) {
			  CHANGE_STATE(NAV_MATH_RESULTS);
		  }

#define MATH_SUM 0
#define MATH_SUB 1
#define MATH_MUL 2
#define MATH_DIV 3

		  char opChars[4] = "+-*/";

		  if(!state_math.displayedTask) {
			  state_math.displayedTask = true;

			  int op1, op2, a, b, c, x, y;

			  while(true) {
				  op1 = rand() % 4;
				  op2 = rand() % 4;

				  a = rand() % 100;
				  b = rand() % 100;
				  c = rand() % 100;

				  if((op2 / 2) > (op1 / 2)) {
					  switch(op2) {
					  case MATH_SUM: y = b + c; break;
					  case MATH_SUB: y = b - c; break;
					  case MATH_MUL: y = b * c; break;
					  case MATH_DIV: if(c == 0) continue; y = b / c; if(c * y != b) continue; break;
					  }

					  switch(op1) {
					  case MATH_SUM: x = a + y; break;
					  case MATH_SUB: x = a - y; break;
					  case MATH_MUL: x = a * y; break;
					  case MATH_DIV: if(y == 0) continue; x = a / y; if(y * x != a) continue; break;
					  }
				  }
				  else {
					  switch(op1) {
					  case MATH_SUM: y = a + b; break;
					  case MATH_SUB: y = a - b; break;
					  case MATH_MUL: y = a * b; break;
					  case MATH_DIV: if(b == 0) continue; y = a / b; if(b * y != a) continue; break;
					  }

					  switch(op2) {
					  case MATH_SUM: x = y + c; break;
					  case MATH_SUB: x = y - c; break;
					  case MATH_MUL: x = y * c; break;
					  case MATH_DIV: if(c == 0) continue; x = y / c; if(c * x != y) continue; break;
					  }
				  }

				  if(x < 0) continue;
				  if(x < 1000) break;
			  }

			  display_instruction_clearDisplay(&display);


			  len = sprintf(buffer, "%d. %d %c %d %c %d", state_math.task_done + 1, a, opChars[op1], b, opChars[op2], c);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  			  display_writeString(&display, buffer, len);

  			  state_math.correctAnswer = rand() % 4;
  			  int answers[4] = {0};
  			  answers[state_math.correctAnswer] = x;


  			  for(int i = 0; i < 4; i++) {
  				  if(i == state_math.correctAnswer) {
  					  len = sprintf(buffer, "%d", x);
  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + i * 4);
  		  			  display_writeString(&display, buffer, len);
  		  			  continue;
  				  }

  				  while(true) {
  					  loopmath:
  					  y = x + ((rand() % 150) - 75);
  					  if(y == x) continue;
  					  if(y >= 1000) continue;
  					  if(y < 0) continue;
  					  for(int j = 0; j < i; j++) {
  						  if(answers[j] == y) goto loopmath;
  					  }

  					  answers[i] = y;

  					  len = sprintf(buffer, "%d", y);
  		  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + i * 4);
  		  			  display_writeString(&display, buffer, len);

  		  			  break;
  				  }
  			  }


  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  state_math.selection = 0;
		  }


		  if(!tryReadInput(&input, 1, true)) continue;

		  if(input == ' ') {
			  state_math.displayedTask = false;

			  state_math.task_done += 1;
			  state_math.task_correct += (state_math.correctAnswer == state_math.selection);
		  }
		  else if(input == '<') {
			  state_math.selection -= 1;
		  }
		  else if(input == '>') {
			  state_math.selection += 1;
		  }

		  if(state_math.selection < 0) state_math.selection += 4;
		  if(state_math.selection >= 4) state_math.selection -= 4;


		  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN + state_math.selection * 4);


		  break;

	  case NAV_MATH_RESULTS:
		  if(justChangedState) {
			  display_instruction_clearDisplay(&display);
			  display_instruction_entryModeSet(&display, true, false);
			  display_instruction_displayOnOffControl(&display, true, false, false);

			  int time = __HAL_TIM_GET_COUNTER(&htim2);

			  int sec = time / 1000000;
			  int sec_decimal = (time - (sec * 1000000)) / 100000;

			  len = sprintf(buffer, "GG! Time: %d.%ds", sec, sec_decimal);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_0_MIN);
  			  display_writeString(&display, buffer, len);
			  len = sprintf(buffer, "Correct: %d/%d", state_math.task_correct, state_math.task_total);
  			  display_instruction_setDisplayRamAddress(&display, DISPLAY_LINE_1_MIN);
  			  display_writeString(&display, buffer, len);
		  }


		  if(!tryReadInput(&input, 1, true)) continue;


		  if(input == ' ') {
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);

  /*Configure GPIO pin : PD12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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

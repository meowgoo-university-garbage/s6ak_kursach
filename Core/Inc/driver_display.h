/*
 * driver_display.c
 *
 *  Created on: Apr 29, 2026
 *      Author: vanya
 */


#ifndef __DRIVER_DISPLAY
#define __DRIVER_DISPLAY

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define DISPLAY_PIN_RS (1 << 0)
#define DISPLAY_PIN_RW (1 << 1)
#define DISPLAY_PIN_EN (1 << 2)
#define DISPLAY_PIN_BL (1 << 3)
#define DISPLAY_PIN_D4 (1 << 4)
#define DISPLAY_PIN_D5 (1 << 5)
#define DISPLAY_PIN_D6 (1 << 6)
#define DISPLAY_PIN_D7 (1 << 7)

#define DISPLAY_ADDRESS (0x27 << 1)

#define PINS_FROM_NIBBLE(nibble) ( \
	(DISPLAY_PIN_D7 * (((nibble) & 0b1000) != 0)) | \
	(DISPLAY_PIN_D6 * (((nibble) & 0b0100) != 0)) | \
	(DISPLAY_PIN_D5 * (((nibble) & 0b0010) != 0)) | \
	(DISPLAY_PIN_D4 * (((nibble) & 0b0001) != 0)) )

#define NIBBLE_LO(byte) ((byte) & 0x0f)
#define NIBBLE_HI(byte) (((byte) & 0xf0) >> 4)

#define DISPLAY_MODE_WRITE 0
#define DISPLAY_MODE_READ  1

#define DISPLAY_REG_INSTRUCTION 0
#define DISPLAY_REG_DATA        1

#define DISPLAY_UNPACK_STATE(d) (((d).mode * DISPLAY_PIN_RW) | ((d).reg * DISPLAY_PIN_RS) | ((d).backlight * DISPLAY_PIN_BL))

#define DISPLAY_LINE_SINGLE_MIN 0x00
#define DISPLAY_LINE_SINGLE_MAX 0x4f

#define DISPLAY_LINE_0_MIN 0x00
#define DISPLAY_LINE_0_MAX 0x27
#define DISPLAY_LINE_1_MIN 0x40
#define DISPLAY_LINE_1_MAX 0x67

#define DISPLAY_VISIBLE_LINE_LEN 16

#define DISPLAY_LINE_SINGLE_LEN (DISPLAY_LINE_SINGLE_MAX - DISPLAY_LINE_SINGLE_MIN + 1)
#define DISPLAY_LINE_LEN        (DISPLAY_LINE_0_MAX      - DISPLAY_LINE_0_MIN      + 1)

typedef struct {
	bool twoLinesInsteadOfOne;
	bool tallFont;

	bool displayOn;
	bool cursorVisible;
	bool cursorBlinking;

	bool incrementInsteadOfDecrement;
	bool shiftOnEntry;
} DisplayConfig;

typedef struct {
	DisplayConfig config;

	I2C_HandleTypeDef *handle;

	uint8_t mode;
	uint8_t reg;
	bool backlight;
} Display;

void display_sendRaw(I2C_HandleTypeDef *handle, uint8_t *data, size_t len);

void display_sendNibble(Display *display, uint8_t byte);

void display_sync(Display *display);

void display_send(Display *display, uint8_t byte);

void display_writeInstruction(Display *display, uint8_t instruction);

void display_writeData(Display *display, uint8_t data);

void display_init(Display *display);




void display_instruction_clearDisplay(Display *display);

void display_instruction_returnHome(Display *display);

void display_instruction_entryModeSet(Display *display, bool incrementInsteadOfDecrement, bool shiftOnCharacters);

void display_instruction_displayOnOffControl(Display *display, bool on, bool cursorVisible, bool blinking);

void display_instruction_cursorOrDisplayShift(Display *display, bool displayInsteadOfCursor, bool rightInsteadOfLeft);

void display_instruction_functionSet(Display *display, bool use8bitInsteadOf4bit, bool twoLinesInsteadOfOneLine, bool tallFont);

void display_instruction_setCharacterRamAddress(Display *display, uint8_t address);

void display_instruction_setDisplayRamAddress(Display *display, uint8_t address);

#endif // __DRIVER_DISPLAY

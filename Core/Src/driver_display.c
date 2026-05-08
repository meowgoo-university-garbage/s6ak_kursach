/*
 * driver_display.c
 *
 *  Created on: Apr 29, 2026
 *      Author: vanya
 */

#include <string.h>
#include "driver_display.h"

void display_sendRaw(I2C_HandleTypeDef *handle, uint8_t *data, size_t len) {
	HAL_I2C_Master_Transmit(handle, DISPLAY_ADDRESS, data, len, 100);
}

void display_sendNibble(Display *display, uint8_t byte) {
	uint8_t data[2] = {0};
	data[0] = byte | ( DISPLAY_PIN_EN);
	data[1] = byte & (~DISPLAY_PIN_EN);

	display_sendRaw(display->handle, data, sizeof(data));
	HAL_Delay(1);
}

void display_sync(Display *display) {
	uint8_t data = DISPLAY_UNPACK_STATE(*display);
	display_sendRaw(display->handle, &data, 1);
}

void display_send(Display *display, uint8_t byte) {
	uint8_t data[4] = {0};
	data[0] = PINS_FROM_NIBBLE(NIBBLE_HI(byte)) | DISPLAY_UNPACK_STATE(*display) | DISPLAY_PIN_EN;
	data[1] = data[0] & (~DISPLAY_PIN_EN);
	data[2] = PINS_FROM_NIBBLE(NIBBLE_LO(byte)) | DISPLAY_UNPACK_STATE(*display) | DISPLAY_PIN_EN;
	data[3] = data[2] & (~DISPLAY_PIN_EN);

	display_sendRaw(display->handle, &data[0], 2);
	HAL_Delay(1);
	display_sendRaw(display->handle, &data[2], 2);
	HAL_Delay(1);
}

void display_writeInstruction(Display *display, uint8_t instruction) {
	Display copy = *display;
	display->mode = DISPLAY_MODE_WRITE;
	display->reg = DISPLAY_REG_INSTRUCTION;
	display_send(display, instruction);
	*display = copy;
}

void display_writeData(Display *display, uint8_t data) {
	Display copy = *display;
	display->mode = DISPLAY_MODE_WRITE;
	display->reg = DISPLAY_REG_DATA;
	display_send(display, data);
	*display = copy;
}


void display_init(Display *display) {
	uint32_t epsilon = 2;

	HAL_Delay(15 + epsilon);
	display_sendNibble(display, (DISPLAY_PIN_D5 | DISPLAY_PIN_D4));

	HAL_Delay(4 + epsilon);
	display_sendNibble(display, (DISPLAY_PIN_D5 | DISPLAY_PIN_D4));

	HAL_Delay(1 + epsilon);
	display_sendNibble(display, (DISPLAY_PIN_D5 | DISPLAY_PIN_D4));

	display_sendNibble(display, (DISPLAY_PIN_D5));


	display_instruction_functionSet(display, false, display->config.twoLinesInsteadOfOne, display->config.tallFont);

	display_instruction_displayOnOffControl(display, display->config.displayOn, display->config.cursorVisible, display->config.cursorBlinking);

	display_instruction_clearDisplay(display);

	display_instruction_entryModeSet(display, display->config.incrementInsteadOfDecrement, display->config.shiftOnEntry);
}




void display_instruction_clearDisplay(Display *display) {
	display_writeInstruction(display, 0b00000001);
}

void display_instruction_returnHome(Display *display) {
	display_writeInstruction(display, 0b00000010);
}

void display_instruction_entryModeSet(Display *display, bool incrementInsteadOfDecrement, bool shiftOnCharacters) {
	display_writeInstruction(display,
		   0b00000100
		| (0b00000010 * incrementInsteadOfDecrement)
		| (0b00000001 * shiftOnCharacters));
}

void display_instruction_displayOnOffControl(Display *display, bool on, bool cursorVisible, bool blinking) {
	display_writeInstruction(display,
		   0b00001000
		| (0b00000100 * on)
		| (0b00000010 * cursorVisible)
		| (0b00000001 * blinking));
}

void display_instruction_cursorOrDisplayShift(Display *display, bool displayInsteadOfCursor, bool rightInsteadOfLeft) {
	display_writeInstruction(display,
		   0b00010000
		| (0b00001000 * displayInsteadOfCursor)
		| (0b00000100 * rightInsteadOfLeft));
}

void display_instruction_functionSet(Display *display, bool use8bitInsteadOf4bit, bool twoLinesInsteadOfOneLine, bool tallFont) {
	if(twoLinesInsteadOfOneLine) { tallFont = false; }
	display_writeInstruction(display,
		   0b00100000
		| (0b00010000 * use8bitInsteadOf4bit)
		| (0b00001000 * twoLinesInsteadOfOneLine)
		| (0b00000100 * tallFont));
}

void display_instruction_setCharacterRamAddress(Display *display, uint8_t address) {
	address &= 0b00111111;
	display_writeInstruction(display, 0b01000000 | address);
}

void display_instruction_setDisplayRamAddress(Display *display, uint8_t address) {
	address &= 0b01111111;
	display_writeInstruction(display, 0b10000000 | address);
}





void display_writeString(Display *display, char *s, int length) {
	size_t l = length == -1 ? strlen(s) : (size_t)length;
	for(int i = 0; i < l; i++) {
		display_writeData(display, s[i]);
	}
}

void display_writeChar(Display *display, char c) {
	display_writeData(display, c);
}

int display_writeCharOnLine(Display *display, int line, int pos, char c, bool looping) {
	uint8_t min = line == 0 ? DISPLAY_LINE_0_MIN : DISPLAY_LINE_1_MIN;
	uint8_t max = line == 0 ? DISPLAY_LINE_0_MAX : DISPLAY_LINE_1_MAX;

	if(!looping) {
		display_instruction_setDisplayRamAddress(display, min + pos);
	}

	if((min + pos) > max) {
		pos %= (max - min + 1);
		display_instruction_setDisplayRamAddress(display, min + pos);
	}

	display_writeData(display, c);
	pos += 1;
	return pos;
}

void display_writeStringOnLine(Display *display, int line, int pos, char *s, int length) {
	if(display->config.twoLinesInsteadOfOne) {
		uint8_t min = line == 0 ? DISPLAY_LINE_0_MIN : DISPLAY_LINE_1_MIN;
		display_instruction_setDisplayRamAddress(display, min + pos);
		for(int i = 0; i < length; i++) {
			pos = display_writeCharOnLine(display, line, pos, s[i], true);
		}
	}
}


//void display_command_writeChar(Display *display, char c) {
//	display_writeData(display, c);
//}



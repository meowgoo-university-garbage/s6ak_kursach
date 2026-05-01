/*
 * driver_sensor.c
 *
 *  Created on: Apr 30, 2026
 *      Author: vanya
 */
#include "driver_sensor.h"

void sensor_delay(Sensor *sensor, size_t delayUs) {
	 __HAL_TIM_SET_COUNTER(sensor->timer, 0);
	 while (__HAL_TIM_GET_COUNTER(sensor->timer) < delayUs) {}
	 return;
}

bool sensor_readBit(Sensor *sensor) {
	GPIO_PinState state = GPIO_PIN_RESET;

	while(state != GPIO_PIN_SET) {
		sensor_delay(sensor, 1);
		state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
	}

	sensor_delay(sensor, 33);
	state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
	if(state == GPIO_PIN_RESET) {
		return false;
	}
	else {
		while(state != GPIO_PIN_RESET) {
			sensor_delay(sensor, 1);
			state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
		}
		return true;
	}
}

uint8_t sensor_readByte(Sensor *sensor) {
	uint8_t result = 0;

	for(int mask = 0b10000000; mask != 0; mask >>= 1) {
		bool bit = sensor_readBit(sensor);
		result |= (bit * mask);
	}

	return result;
}

bool sensor_read(Sensor *sensor, SensorData *result) {
	HAL_GPIO_WritePin(sensor->gpiox, sensor->pin, GPIO_PIN_RESET);
	sensor_delay(sensor, 20 * 1000);
	HAL_GPIO_WritePin(sensor->gpiox, sensor->pin, GPIO_PIN_SET);
	sensor_delay(sensor, 50);

	GPIO_PinState state;

	state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
	if(state != GPIO_PIN_RESET) return false;

	while(state != GPIO_PIN_SET) {
		sensor_delay(sensor, 1);
		state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
	}

	while(state != GPIO_PIN_RESET) {
		sensor_delay(sensor, 1);
		state = HAL_GPIO_ReadPin(sensor->gpiox, sensor->pin);
	}

	for(int i = 0; i < 5; i++) {
		result->data[i] = sensor_readByte(sensor);
	}

	return true;
}

bool sensor_validate(SensorData data) {
	uint16_t sum = data.data[0] + data.data[1] + data.data[2] + data.data[3];
	uint8_t capSum = (uint8_t)(sum & 0xff);
	return capSum == data.checksum;
}

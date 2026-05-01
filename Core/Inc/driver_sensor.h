/*
 * driver_sensor.h
 *
 *  Created on: Apr 30, 2026
 *      Author: vanya
 */

#ifndef INC_DRIVER_SENSOR_H_
#define INC_DRIVER_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

typedef struct {
	TIM_HandleTypeDef *timer;
	GPIO_TypeDef *gpiox;
	uint16_t pin;
} Sensor;

typedef union {
	uint8_t data[5];
	struct {
		uint8_t rh_integral;
		uint8_t rh_decimal;
		uint8_t t_integral;
		uint8_t t_decimal;
		uint8_t checksum;
	};
} SensorData;

void sensor_delay(Sensor *sensor, size_t delayUs);

bool sensor_readBit(Sensor *sensor);

uint8_t sensor_readByte(Sensor *sensor);

bool sensor_read(Sensor *sensor, SensorData *result);

bool sensor_validate(SensorData data);

#endif /* INC_DRIVER_SENSOR_H_ */

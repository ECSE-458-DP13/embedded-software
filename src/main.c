/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <metal/cpu.h>
#include <metal/i2c.h>
#include <metal/io.h>
#include <metal/machine.h>
#include <metal/uart.h>
#include <stdio.h>
#include <time.h>

// TODO: this is copied over from the example; figure out what address to use
/* PmodAD2, PmodTmp2 sensor modules are connected to I2C0 bus */
#define IMU_I2C_ADDR 0x6A
#define MAGNETO_I2C_ADDR 0x1C
#define TEMP_SENSOR_ID 0xCB
#define ADC_I2C_ADDR 0x28
#define I2C_BAUDRATE 100000

/* Return values */
#define RET_OK 0
#define RET_NOK 1
/* Buffer length macros */
#define LEN0 0
#define LEN1 1
#define LEN2 2
/* 1s delay macro */
#define WAIT_1S(timeout)                                                       \
  timeout = time(NULL) + 1;                                                    \
  while (timeout > time(NULL))                                                 \
    ;

int main() {
	unsigned int temp, volt;
	unsigned char buf[LEN2];
	time_t timeout;
	struct metal_i2c *i2c;

	printf("%s %s \n", __DATE__, __TIME__);
	printf("I2C demo test..\n");

	i2c = metal_i2c_get_device(0);

	if (i2c == NULL) {
		printf("I2C not available \n");
		return RET_NOK;
	}

	metal_i2c_init(i2c, I2C_BAUDRATE, METAL_I2C_MASTER);

	// TODO: this is copied from the example; find how it would work for our sensor
	/* Attempt to read ADT7420 Chip ID */
	buf[0] = 0x0B;
	metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_DISABLE);
	metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_ENABLE);

	/* Verify Chip ID */
	if (buf[0] == TEMP_SENSOR_ID) {
		printf("PmodTmp2 module detected \n");
	} else {
		printf("Failed to detect PmodTmp2 module \n");
		return RET_NOK;
	}

	/* Attempt to access AD7991, configure to convert on Vin0. */
	buf[0] = 0x10;
	if (metal_i2c_write(i2c, ADC_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_ENABLE) != RET_OK) {
		printf("Failed to detect PmodAD2 module \n");
		return RET_NOK;
	} else {
		printf("PmodAD2 module detected \n");
	}

	return 0;
}


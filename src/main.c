/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <metal/cpu.h>
#include <metal/i2c.h>
#include <metal/io.h>
#include <metal/machine.h>
#include <metal/uart.h>
#include <stdio.h>
#include <time.h>

// addresses and subaddresses for IMU
// IMU = accelero + gyro
// each sensor value is 2 bytes
// want yaw for gyro, x-y for accelero
#define IMU_I2C_ADDR 0x6A
#define IMU_SENSOR_ID 0x69
#define IMU_WHO_AM_I_SUBADDR 0x0F
#define IMU_OUTZ_L_G_SUBADDR 0x26
#define IMU_OUTZ_H_G_SUBADDR 0x27
#define IMU_OUTX_L_XL_SUBADDR 0x28
#define IMU_OUTX_H_XL_SUBADDR 0x29
#define IMU_OUTY_L_XL_SUBADDR 0x2A
#define IMU_OUTY_H_XL_SUBADDR 0x2B

// addresses and subaddresses for magneto
#define MAGNETO_I2C_ADDR 0x1C
// TODO: subaddresses

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
	unsigned char buf[LEN1];
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
	/* Attempt to read LSM6DS33 (IMU) chip id */
	buf[0] = IMU_WHO_AM_I_SUBADDR;

	while(1) {
		buf[0] = IMU_WHO_AM_I_SUBADDR;
		metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_DISABLE);
		metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_ENABLE);
		printf("Data read = %x\n", buf[0]);
		//metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_DISABLE);
		//metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, buf, METAL_I2C_STOP_ENABLE);

		/* Verify Chip ID */
		if (buf[0] == IMU_SENSOR_ID) {
			printf("Accelero/Gyro module detected \n");
		} else {
			printf("Failed to detect Accelero/Gyro module \n");
			//return RET_NOK;
		}
	}


	return 0;
}


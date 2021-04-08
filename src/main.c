/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <metal/cpu.h>
#include <metal/i2c.h>
#include <metal/io.h>
#include <metal/machine.h>
#include <metal/uart.h>
#include <metal/timer.h>
#include <stdio.h>
#include <time.h>

/*
 * Addresses and subaddresses for IMU
 * IMU = accelero + gyro
 * Each raw sensor value is 2 bytes, must be scaled appropriately
 * We want yaw for gyro, x-y for accelero
 */
#define IMU_I2C_ADDR 0x6A
#define IMU_SENSOR_ID 0x69
#define IMU_WHO_AM_I_SUBADDR 0x0F
#define IMU_STATUS_REG_SUBADDR 0x1E
#define IMU_OUTZ_L_G_SUBADDR 0x26
#define IMU_OUTZ_H_G_SUBADDR 0x27
#define IMU_OUTX_L_XL_SUBADDR 0x28
#define IMU_OUTX_H_XL_SUBADDR 0x29
#define IMU_OUTY_L_XL_SUBADDR 0x2A
#define IMU_OUTY_H_XL_SUBADDR 0x2B
#define IMU_CTRL1_XL_SUBADDR 0x10
#define IMU_CTRL2_G_SUBADDR 0x11

/* Constants for scaling raw IMU data */
#define ACCELERO_SCALE 0.061
#define GYRO_SCALE 4.375
#define GRAVITY 9.80665
#define DPS_TO_RADS 0.017453293

/* Addresses and subaddresses for magneto */
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
#define LEN4 4
#define LEN8 8
#define LEN10 10

/* 1s delay macro */
#define WAIT_1S(timeout)                                                       \
  timeout = time(NULL) + 1;                                                    \
  while (timeout > time(NULL))                                                 \
    ;

/*
 * This is here because of some weird issue where printf here doesn't support floats
 * See: https://forums.sifive.com/t/couldnt-store-floating-point-result/1660/14
 */
asm (".global _printf_float");

int main() {
	/*
	 * imu_buf is to buffer the following:
	 * [0] device ID
	 * [1] device status
	 * [2] gyro z-axis MSB
	 * [3] gyro z-axis LSB
	 * [4] accelero x-axis MSB
	 * [5] accelero x-axis LSB
	 * [6] accelero y-axis MSB
	 * [7] accelero y-axis LSB
	 * */
	unsigned char imu_buf[LEN8];
	unsigned char imu_addr_buf[LEN8] = { IMU_WHO_AM_I_SUBADDR, IMU_STATUS_REG_SUBADDR,
										IMU_OUTZ_H_G_SUBADDR, IMU_OUTZ_L_G_SUBADDR,
										IMU_OUTX_H_XL_SUBADDR, IMU_OUTX_L_XL_SUBADDR,
										IMU_OUTY_H_XL_SUBADDR, IMU_OUTY_L_XL_SUBADDR };
	time_t timeout;
	struct metal_i2c *i2c;
	struct metal_gpio *gpio;
	struct metal_cpu *cpu;

	i2c = metal_i2c_get_device(0);
	gpio = metal_gpio_get_device(0);
	cpu = metal_cpu_get(metal_cpu_get_current_hartid()); // only one processor, so should be just one cpu


	if (i2c == NULL) {
		printf("I2C not available \n");
		return RET_NOK;
	}
	if (gpio == NULL) {
		printf("GPIO not available \n");
		return RET_NOK;
	}

	metal_i2c_init(i2c, I2C_BAUDRATE, METAL_I2C_MASTER);

	/* Read IMU Chip ID */
	if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf, METAL_I2C_STOP_DISABLE) == RET_OK) {
		if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf, METAL_I2C_STOP_ENABLE) == RET_OK) {
			printf("Data read = %X\n", imu_buf[0]);
		}
	}

	/* Verify IMU Chip ID */
	if (imu_buf[0] == IMU_SENSOR_ID) {
		printf("IMU module detected!\n");
	} else {
		printf("Failed to detect IMU module!\n");
		//return RET_NOK;
	}

	/* Enable IMU */
	unsigned char imu_ctrl_buf[LEN4] = { IMU_CTRL1_XL_SUBADDR, 0xA0,
										IMU_CTRL2_G_SUBADDR, 0x80 };
	if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN2, imu_ctrl_buf, METAL_I2C_STOP_ENABLE) == RET_OK) {
		printf("IMU accelero enabled!\n");
	}
	if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN2, imu_ctrl_buf + 2, METAL_I2C_STOP_ENABLE) == RET_OK) {
		printf("IMU gyro enabled!\n");
	}

	/* Initialize variables for raw sensor values */
	short rawGyroZ = 0;
	short rawAcceleroX = 0;
	short rawAcceleroY = 0;

	float gyroZ = 0;
	float acceleroX = 0;
	float acceleroY = 0;

	/* Declare variables for timing of ultrasonic ranging */
	unsigned long long pulseStart;
	unsigned long long pulseEnd;
	unsigned long long timebase = metal_cpu_get_timebase(cpu);
	float duration;

	while(1) {

		/* Read IMU Status Register */
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 1, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 1, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Status Reg = %X\n", imu_buf[1]);
			}
		}

		/* Read IMU Gyro (z-axis) */
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 2, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 2, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Gyro Z MSB = %X\n", imu_buf[2]);
			}
		}
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 3, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 3, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Gyro Z LSB = %X\n", imu_buf[3]);
			}
		}
		rawGyroZ = (short) ((imu_buf[2] << 8) | imu_buf[3]);
		gyroZ = rawGyroZ * GYRO_SCALE * DPS_TO_RADS / 1000.0;
		printf("Gyro Z = %.6f\n", gyroZ);

		/* Read IMU Accelero (x-y plane) */
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 4, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 4, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Accelero X MSB = %X\n", imu_buf[4]);
			}
		}
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 5, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 5, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Accelero X LSB = %X\n", imu_buf[5]);
			}
		}
		rawAcceleroX = (short) ((imu_buf[4] << 8) | imu_buf[5]);
		acceleroX = rawAcceleroX * ACCELERO_SCALE * GRAVITY / 1000.0;
		printf("Accelero X = %.6f\n", acceleroX);

		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 6, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 6, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Accelero Y MSB = %X\n", imu_buf[6]);
			}
		}
		if (metal_i2c_write(i2c, IMU_I2C_ADDR, LEN1, imu_addr_buf + 7, METAL_I2C_STOP_DISABLE) == RET_OK) {
			if (metal_i2c_read(i2c, IMU_I2C_ADDR, LEN1, imu_buf + 7, METAL_I2C_STOP_ENABLE) == RET_OK) {
				printf("Accelero Y LSB = %X\n", imu_buf[7]);
			}
		}
		rawAcceleroY = (short) ((imu_buf[6] << 8) | imu_buf[7]);
		acceleroY = rawAcceleroY * ACCELERO_SCALE * GRAVITY / 1000.0;
		printf("Accelero Y = %.6f\n", acceleroY);


		/* ULTRASONIC SENSOR */
		/* Pre-set TRIG to LOW (for clean signal) */
		metal_gpio_disable_output(gpio, 16);
		// TODO: delay for 5 microseconds

		/* Hold TRIG at HIGH for 10 microseconds (to generate pulse) */
		metal_gpio_enable_output(gpio, 16);
		// TODO: delay for 10 microseconds
		metal_gpio_disable_output(gpio, 16);

		/* Listen for ECHO to be HIGH */
		while(!metal_gpio_get_input_pin(gpio, 17));
		pulseStart = metal_cpu_get_timer(cpu);
		while(metal_gpio_get_input_pin(gpio, 17));
		pulseEnd = metal_cpu_get_timer(cpu);
		duration = (float) (pulseEnd - pulseStart) / timebase;

		// TODO: test what we get out of the cpu timer functions
		printf("");


	}


	return 0;
}


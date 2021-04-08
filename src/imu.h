/*
 * imu.h
 *
 *  Created on: Apr. 7, 2021
 *      Author: Garrett Kinman
 */

#ifndef IMU_H_
#define IMU_H_

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

/*
 * Buffer the following:
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

/* Buffer subaddresses in corresponding indexes */
unsigned char imu_addr_buf[LEN8] = { IMU_WHO_AM_I_SUBADDR, IMU_STATUS_REG_SUBADDR,
									IMU_OUTZ_H_G_SUBADDR, IMU_OUTZ_L_G_SUBADDR,
									IMU_OUTX_H_XL_SUBADDR, IMU_OUTX_L_XL_SUBADDR,
									IMU_OUTY_H_XL_SUBADDR, IMU_OUTY_L_XL_SUBADDR };

int imu_init();

int getAcceleroX(float *acceleroX);

int getAcceleroY(float *acceleroY);

int getGyroZ(float *gyroZ);

#endif /* IMU_H_ */

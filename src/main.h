/*
 * main.h
 *
 *  Created on: Apr. 7, 2021
 *      Author: Garrett Kinman
 */

#ifndef MAIN_H_
#define MAIN_H_

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

#endif /* MAIN_H_ */

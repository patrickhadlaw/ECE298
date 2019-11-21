/*
 * buzzer.h
 *
 *  Created on: Nov 20, 2019
 *      Author: erikm
 */
#ifndef BUZZER_H
#define BUZZER_H

#include "driverlib/driverlib.h"

#define PWM_PORT        GPIO_PORT_P1
#define PWM_PIN         GPIO_PIN7

extern struct Timer_A_outputPWMParam param;

#define TIMER_A_PERIOD  1000 //T = 1/f = (TIMER_A_PERIOD * 1 us)
#define HIGH_COUNT      500  //Number of cycles signal is high (Duty Cycle = HIGH_COUNT / TIMER_A_PERIOD)

void PWM_init();
void Buzzer_on();
void Buzzer_off();

#endif /* BUZZER_H_ */

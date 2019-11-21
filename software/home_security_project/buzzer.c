/*
 * buzzer.c
 *
 *  Created on: Nov 20, 2019
 *      Author: erikm
 */

#include "buzzer.h"

struct Timer_A_outputPWMParam param;

void PWM_init() {

    param.clockSource           = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider    = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod           = TIMER_A_PERIOD; //Defined in main.h
    param.compareRegister       = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    param.compareOutputMode     = TIMER_A_OUTPUTMODE_RESET_SET;
    param.dutyCycle             = HIGH_COUNT; //Defined in main.h

    GPIO_setAsPeripheralModuleFunctionOutputPin(PWM_PORT, PWM_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
}

void Buzzer_on() {

    Timer_A_outputPWM(TIMER_A0_BASE, &param);   //Turn on PWM

}

void Buzzer_off() {

    Timer_A_stop(TIMER_A0_BASE);    //Shut off PWM signal


}


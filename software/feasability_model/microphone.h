#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "driverlib/driverlib.h"

#define ADC_IN_PORT     GPIO_PORT_P8
#define ADC_IN_PIN      GPIO_PIN1
#define ADC_IN_CHANNEL  ADC_INPUT_A9

extern volatile char ADCState;
extern volatile int16_t ADCResult;

void Microphone_init();
void Microphone_close();

void ADC_ISR(void);

#endif // MICROPHONE_H

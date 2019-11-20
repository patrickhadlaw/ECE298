#include "microphone.h"


volatile char ADCState = 0;
volatile int16_t ADCResult = 0;

void Microphone_init() {
    GPIO_setAsPeripheralModuleFunctionInputPin(ADC_IN_PORT, ADC_IN_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
    ADC_init(ADC_BASE, ADC_SAMPLEHOLDSOURCE_SC, ADC_CLOCKSOURCE_ADCOSC, ADC_CLOCKDIVIDER_1);

    ADC_enable(ADC_BASE);

    ADC_setupSamplingTimer(ADC_BASE, ADC_CYCLEHOLD_16_CYCLES, ADC_MULTIPLESAMPLESDISABLE);
    ADC_configureMemory(ADC_BASE, ADC_IN_CHANNEL, ADC_VREFPOS_AVCC, ADC_VREFNEG_AVSS);
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
    ADC_enableInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
    __enable_interrupt();
}

void Microphone_close() {
    ADC_clearInterrupt(ADC_BASE, ADC_COMPLETED_INTERRUPT);
    ADC_disable(ADC_BASE);
    __enable_interrupt();
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void) {
    uint8_t ADCStatus = ADC_getInterruptStatus(ADC_BASE, ADC_COMPLETED_INTERRUPT_FLAG);

    ADC_clearInterrupt(ADC_BASE, ADCStatus);

    if (ADCStatus)
    {
        ADCState = 0; //Not busy anymore
        ADCResult = ADC_getResults(ADC_BASE);
    }
}

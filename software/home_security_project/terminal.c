#include "terminal.h"


bool terminal_enabled = false;

void Clock_init() {
    //Set P4.1 and P4.2 as Primary Module Function Input, XT_LF
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768);
    // Set ACLK = XT1
    CS_initClockSignal(CS_ACLK, CS_XT1CLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Initializes the XT1 crystal oscillator
    CS_turnOnXT1LF(CS_XT1_DRIVE_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
}

int Terminal_init() {
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

    struct EUSCI_A_UART_initParam param;
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 6;
    param.firstModReg = 8;
    param.secondModReg = 17;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param)) {
        return -1;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    __enable_interrupt();
    return 0;
}

void Terminal_enable() {
    EUSCI_A_UART_resetDormant(EUSCI_A0_BASE);
    EUSCI_A_UART_enable(EUSCI_A0_BASE);
    terminal_enabled = true;
}

void Terminal_disable() {
    EUSCI_A_UART_setDormant(EUSCI_A0_BASE);
    EUSCI_A_UART_disable(EUSCI_A0_BASE);
    terminal_enabled = false;
}

void Terminal_close() {
    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    EUSCI_A_UART_disable(EUSCI_A0_BASE);
    __enable_interrupt();
}

int Terminal_printf(const char* format, ...) {
   va_list arglist;
   va_start(arglist, format);
   char * result = malloc(100);
   memset(result, 0, 100);
   if (vsnprintf(result, 100, format, arglist) < 0) {
       va_end( arglist );
       free(result);
       return -1;
   }
   va_end(arglist);
   int i;
   for (i = 0; i < strlen(result); i++) {
       while (EUSCI_A_UART_queryStatusFlags(EUSCI_A0_BASE, EUSCI_A_UART_BUSY) & 0x01);
       EUSCI_A_UART_transmitAddress(EUSCI_A0_BASE, result[i]);
   }
   free(result);
   return 0;
}

int Terminal_clear() {
    char clrstr[TERMINAL_CLEAR_LINES * 3 + 1];
    int i;
    for (i = 0; i < TERMINAL_CLEAR_LINES; i++) {
        if (i < TERMINAL_CLEAR_LINES * 2) {
            if (i % 2) {
                clrstr[i] = '\r';
            }
            else {
                clrstr[i] = '\n';
            }
        }
        else {
            clrstr[i] = '\x18';
        }
    }
    clrstr[TERMINAL_CLEAR_LINES * 2] = '\0';
    Terminal_printf(clrstr);
    return Terminal_printf("\033[H\033[2J\x4");
}

#pragma vector=USCI_A0_VECTOR
__interrupt void EUSCIA0_ISR(void) {
    uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
    if (terminal_enabled) {
        if (RxStatus)
        {
           //EUSCI_A_UART_transmitData(EUSCI_A0_BASE, EUSCI_A_UART_receiveData(EUSCI_A0_BASE));
        }
    }
    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus);
}

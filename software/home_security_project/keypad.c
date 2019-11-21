#include "keypad.h"


long long rtc = 0;

enum KeyPad current_key = NONE;
bool key_wait = false;
long long key_time = 0;

void Keypad_init() {
    // Multiplexor
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN4);

    // Keypad
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN7);

    // RTC
    RTC_init(RTC_BASE, 3, RTC_CLOCKPREDIVIDER_1);
    RTC_enableInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
    __enable_interrupt();
    RTC_start(RTC_BASE, RTC_CLOCKSOURCE_XT1CLK);
}

void set_mux_control(uint8_t ctl) {
    if (ctl & 0x1) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
    }
    else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);
    }
    if (ctl & 0x2) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
    }
    else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
    }
    if (ctl & 0x4) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
}

void Keypad_enable() {
    Terminal_disable();
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN1);
}

void Keypad_disable() {
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);
    Terminal_enable();
}

bool Keypad_read(enum KeyPad key) {
    switch (key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
        set_mux_control(4);
        break;
    case KEY_4:
    case KEY_5:
    case KEY_6:
        set_mux_control(5);
        break;
    case KEY_7:
    case KEY_8:
    case KEY_9:
        set_mux_control(6);
        break;
    case KEY_0:
    case KEY_STAR:
    case KEY_NUM:
        set_mux_control(7);
    }

    uint8_t result = false;
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);

    switch (key) {
    case KEY_1:
    case KEY_4:
    case KEY_7:
    case KEY_STAR:
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
        break;
    case KEY_2:
    case KEY_5:
    case KEY_8:
    case KEY_0:
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        break;
    case KEY_3:
    case KEY_6:
    case KEY_9:
    case KEY_NUM:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
        break;
    }

    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);

    return result == 0x1;
}

void Keypad_poll() {
    Keypad_enable();
    if (!key_wait) {
        enum KeyPad key;
        for (key = KEY_0; key <= KEY_NUM; key++) {
            if (Keypad_read(key)) {
                current_key = key;
                key_wait = true;
                key_time = rtc;
                // Fix for P1.0 usage on Keypad interfering with UART
                if (current_key == KEY_2 || current_key == KEY_5 || current_key == KEY_8 || current_key == KEY_0) {
                    Terminal_printf("\x08");    // Transmit backspace to remove glitch
                }
                break;
            }
        }
    }
    else {
        __delay_cycles(10);
        if (!Keypad_read(current_key)) {
            if (rtc - key_time >= KEYPAD_STABALIZE_TIME) {
                key_wait = false;
            }
        }
        else {
            key_time = rtc;
        }
        __delay_cycles(10);
    }
    Keypad_disable();
}

enum KeyPad Keypad_getkey() {
    if (key_wait) {
        return NONE;
    }
    else {
        enum KeyPad key = current_key;
        current_key = NONE;
        return key;
    }
}

#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void) {
    switch(__even_in_range(RTCIV, RTCIV_RTCIF))
    {
    case RTCIV_NONE : break;            // No interrupt pending
    case RTCIV_RTCIF:                   // RTC Overflow
        rtc++;
        break;
    default:
        break;
    }
}

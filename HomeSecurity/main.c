/*
 * HomeSecurity
 *
 *  Created on: Sep 23, 2019
 *      Author: patsh
 */

#include "terminal.h"
#include "microphone.h"

#define SPEED_OF_SOUND 343.0f

volatile unsigned long long rtc = 0;
volatile int ECHO = 0;

int get_lcd_position(int pos) {
    switch (pos) {
    case 0:
        return pos1;
    case 1:
        return pos2;
    case 2:
        return pos3;
    case 3:
        return pos4;
    case 4:
        return pos5;
    case 5:
        return pos6;
    }
    return 0;
}

void display_number(int num) {
    int i = 5;
    while (num > 0 && i >= 0) {
        showChar((num % 10) + '0', get_lcd_position(i));
        num = num / 10;
        i--;
    }
}

void display_text(const char* text, int offset) {
    int len = strlen(text);
    int i;
    for (i = 0; i < len && i < 6; i++) {
        showChar(text[i], get_lcd_position((i + offset) % 6));
    }
}

void display_text_scroll(const char* text, bool delay) {
    int len = strlen(text);
    display_text(text, 0);
    if (delay) {
        __delay_cycles(500000);
    }
    int i;
    for (i = 0; i < len - 5; i++) {
        int offset = 0;
        if (len - i < 6) {
            offset = len - i;
        }
        display_text(text + i, offset);
        __delay_cycles(100000);
    }
    if (delay) {
        __delay_cycles(1000000);
    }
}

void main() {
    WDTCTL = WDTPW | WDTHOLD;   // Disable WDT
    PM5CTL0 &= ~LOCKLPM5;

    P1REN |= 0x04;              // Enable pull up resistor on P1.2 (push button)
    P1OUT = 0x04;
    P1DIR = 0x01 | 0x08;        // Set P1.1, P1.2, P1.4..7 as inputs, P1.0, P1.3 as outputs
    P1DIR &= ~0x10;             // P1.4 is the echo pin of Ultrasonic sensor (Input)
    P1DIR |= 0x20;              // P1.5 is the trigger pin of the Ultrasonic sensor (Output)
    P1DIR |= 0x40;              // P1.6 is the buzzer gpio

    P2REN |= 0x40;              // Enable pull up resistor on P2.6 (push button)
    P2OUT = 0x40;
    P2DIR = 0x00;               // Set P2.0..7 as inputs

    P4DIR = 0x01;               // Set P4.0 as output


    Init_LCD();
    Clock_init();
    RTC_init(RTC_BASE, 3, RTC_CLOCKPREDIVIDER_1);

    // Test program

    display_text_scroll("LCD PB LED", true);

    int pos = 0;

    clearLCD();
    showChar('A', get_lcd_position(pos));

    // LCD, Push button and LED test
    while (P2IN & 0x40) {

        if (!(P1IN & 0x04)) {
            P4OUT ^= 0x01;
            P1OUT ^= 0x08;
            pos = (pos + 1) % 6;
            clearLCD();
            showChar('A', get_lcd_position(pos));
            while (!(P1IN & 0x04));
            __delay_cycles(1000);
        }
    }

    clearLCD();
    P4OUT &= ~0x01;
    while (!(P2IN & 0x40));
    __delay_cycles(1000);

    display_text_scroll("RTC", true);

    RTC_enableInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
    __enable_interrupt();
    RTC_start(RTC_BASE, RTC_CLOCKSOURCE_XT1CLK);

    while (P2IN & 0x40) {
        clearLCD();
        display_number(rtc / 10000);
        __delay_cycles(100000);
    }

    clearLCD();
    while (!(P2IN & 0x40));
    __delay_cycles(1000);

    display_text_scroll("UART", false);

    Terminal_init();

    Terminal_printf("Hello world!");

    while (P2IN & 0x40);

    Terminal_close();

    clearLCD();
    while (!(P2IN & 0x40));
    __delay_cycles(1000);

    display_text_scroll("ULTRASONIC", true);
    clearLCD();

    P1IE |= 0x10; // P1.4 interrupt enabled
    P1IES |= 0x10; // P1.4 Hi/lo edge
    P1IFG &= ~0x10;
    __enable_interrupt();

    unsigned long long previous_time = 0;
    while (P2IN & 0x40) {
        if (!(P1IN & 0x04)) {
            ECHO = 0;
            previous_time = rtc;
            P4OUT |= 0x01;
            P1OUT |= 0x20;
            __delay_cycles(100);
            P1OUT &= ~0x20;
            previous_time = rtc;
            while (!ECHO && rtc - previous_time < 20000ul);
            if ((rtc - previous_time) >= 20000ul) {
                display_text_scroll("BAD READ", true);
                clearLCD();
            } else {
                unsigned long long current_time = rtc;
                unsigned long long delta = current_time - previous_time;
                float distance = (SPEED_OF_SOUND * ((float) delta / 10000.0f)) * 100.0f;
                if (((int) distance) == 0) {
                    display_text_scroll("PRECISION ERROR", true);
                    clearLCD();
                } else {
                    previous_time = current_time;
                    clearLCD();
                    display_number((int) distance);
                }
            }
        }
    }

    clearLCD();
    while (!(P2IN & 0x40));
    __delay_cycles(1000);

    display_text_scroll("BUZZER", true);
    clearLCD();

    while (P2IN & 0x40) {
        if (!(P1IN & 0x04)) {
            int i;
            for (i = 0; i < 5000; i++) {
                P1OUT |= 0x40;
                __delay_cycles(100);
                P1OUT &= ~0x40;
                __delay_cycles(100);
            }
            while (!(P1IN & 0x04));
        }
    }

    clearLCD();
    while (!(P2IN & 0x40));
    __delay_cycles(1000);

    display_text_scroll("ADC MICROPHONE", true);
    clearLCD();

    Microphone_init();

    P4OUT &= ~0x01;
    int i = 0;
    while (P2IN & 0x40) {
        if (ADCState == 0) {
            ADCState = 1; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        }
        int result = ADCResult;
        if (i > 1000) {
            clearLCD();
            display_number(result);
            i = 0;
        }
        if (result > 800) {
            P4OUT |= 0x01;
            __delay_cycles(1000000);
            P4OUT &= ~0x01;
        }
        i++;
    }

    Microphone_close();

    clearLCD();
    display_text_scroll("DONE", true);
}

#pragma vector=RTC_VECTOR
__interrupt void RTC_VECTOR_ISR(void) {
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

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    ECHO = 0x1;
    P1IFG &= ~0x10; // P1.4 IFG cleared
}

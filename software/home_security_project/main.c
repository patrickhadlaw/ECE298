#include "terminal.h"

void init() {
    Init_LCD();
    Clock_init();
    Terminal_init();

    // LEDs
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN3);

    // Multiplexor
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN4);

    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN7);
}

void keypad_enable() {
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN1);
}

void keypad_disable() {
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);
}

enum ZoneState {
    STANDBY,
    ALARM
};

enum KeyPad {
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_STAR,
    KEY_NUM,
};

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

bool read_keypad(enum KeyPad key) {
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
        __delay_cycles(2);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
        break;
    case KEY_2:
    case KEY_5:
    case KEY_8:
    case KEY_0:
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        __delay_cycles(2);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        break;
    case KEY_3:
    case KEY_6:
    case KEY_9:
    case KEY_NUM:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
        __delay_cycles(2);
        result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
        break;
    }
    return result == 0x1;
}

bool read_reed_sw(uint8_t sw) {
    if (sw < 4) {
        set_mux_control(sw);
        __delay_cycles(100);
        uint8_t result = GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4);
        __delay_cycles(1);
        return result == 0x1;
    }
    else {
        return false;
    }
}

void set_zone_led(uint8_t zone, uint8_t value) {
    if (value == 0x1) {
        if (zone == 0) {
            GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN1);
        } else if (zone == 1) {
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
        } else if (zone == 2) {
            GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);
        } else if (zone == 3) {
            GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN3);
        }
    }
    else {
        if (zone == 0) {
            GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN1);
        } else if (zone == 1) {
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
        } else if (zone == 2) {
            GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
        } else if (zone == 3) {
            GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);
        }
    }
}

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

void print_terminal_header() {
    Terminal_printf("----------------------------------------------------------\n");
    Terminal_printf("\tHome Security System\n");
}

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;
	
	init();

    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN3);

    print_terminal_header();

    display_text_scroll("HOME SECURITY SYSTEM", true);

//    set_mux_control(0);
////    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);
////    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
////    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
//
//    if (GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN4)) {
//        GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);
//    }

    clearLCD();

    enum ZoneState global_status = STANDBY;
    enum ZoneState zone_status[4];
    int reed_trigger_count[4];
    memset(zone_status, 0, 4 * sizeof(enum ZoneState));
    memset(reed_trigger_count, 0, 4 * sizeof(int));

    int check_count = 0;
    int light_count = 0;
    bool blink_on = false;

    while(1) {
        if (check_count >= 1000) {
            keypad_enable();
            check_count = 0;
            if (read_keypad(KEY_1)) {
                showChar('1', get_lcd_position(0));
                printf("1\n");
            }
            else if (read_keypad(KEY_2)) {
                showChar('2', get_lcd_position(1));
            }
            else if (read_keypad(KEY_3)) {
                showChar('3', get_lcd_position(2));
            }
            else if (read_keypad(KEY_4)) {
                showChar('4', get_lcd_position(3));
                printf("4\n");
            }
            else if (read_keypad(KEY_5)) {
                showChar('5', get_lcd_position(4));
            }
            else if (read_keypad(KEY_6)) {
               showChar('6', get_lcd_position(5));
            }
            else if (read_keypad(KEY_7)) {
               showChar('7', get_lcd_position(1));
            }
            else if (read_keypad(KEY_8)) {
               showChar('8', get_lcd_position(2));
            }
            else if (read_keypad(KEY_9)) {
               showChar('9', get_lcd_position(3));
            }
            else if (read_keypad(KEY_0)) {
               showChar('0', get_lcd_position(4));
            }
            else if (read_keypad(KEY_NUM)) {
               showChar('N', get_lcd_position(5));
            }
            else if (read_keypad(KEY_STAR)) {
               showChar('X', get_lcd_position(5));
            }
            else {
                clearLCD();
            }
            keypad_disable();
            uint8_t i;
            for (i = 0; i < 4; i++) {
                if (!read_reed_sw(i)) {
                    reed_trigger_count[i]++;
                    if (reed_trigger_count[i] >= 10) {
                        zone_status[i] = ALARM;
                    }
                }
                else {
                    reed_trigger_count[i] = 0;
                }
            }
        }
        // TODO: use RTC timer value for count
        if (light_count >= 10000) {
            int i;
            for (i = 0; i < 4; i++) {
                if (zone_status[i] == ALARM) {
                    if (blink_on) {
                        set_zone_led(i, 0x1);
                    }
                    else {
                        set_zone_led(i, 0x0);
                    }
                }
            }
            light_count = 0;
            blink_on = !blink_on;
        }
        light_count++;

        check_count++;
    }
}

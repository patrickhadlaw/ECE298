#include "keypad.h"

void init() {
    Init_LCD();
    Clock_init();
    Terminal_init();
    Keypad_init();

    // LEDs
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN3);
}

enum TerminalScreen {
    SET_PASSWORD_SCREEN,
    HOME_SCREEN,
    TIME_EDITOR_ZONE_SELECT_SCREEN,
    TIME_EDITOR_SCREEN,
    ARMED_SCREEN,
    ALARM_SCREEN,
};

enum ZoneState {
    STANDBY,
    ALARM
};

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
    Terminal_clear();
    Terminal_printf("----------------------------------------------------------\r\n");
    Terminal_printf("    Home Security System\r\n");
    Terminal_printf("    By: Patrick Hadlaw & Erik Kuhne\r\n");
    Terminal_printf("----------------------------------------------------------\r\n\r\n\r\n\r\n");
}

struct ZoneTime {
    int start_time;
    int end_time;
};

struct ZoneTime zone_times[4];

void print_screen(enum TerminalScreen screen) {
    print_terminal_header();
    if (screen == SET_PASSWORD_SCREEN) {
        Terminal_printf("Enter your 4 number PIN (using the keypad): ");
    }
    else if (screen == HOME_SCREEN) {
        Terminal_printf("Zone activation settings:\r\n");
        int i;
        for (i = 0; i < 4; i++) {
            Terminal_printf(
                    "[%d] %d:%d - %d:%d\r\n",
                    i + 1,
                    (zone_times[i].start_time / 60),
                    (zone_times[i].start_time % 60),
                    (zone_times[i].end_time / 60),
                    (zone_times[i].end_time % 60)
            );
        }

        Terminal_printf("\r\n\r\nSelect the following options using the keypad:\r\n\r\n");
        Terminal_printf("1) Change arm activation times\r\n");
        Terminal_printf("2) ARM the system\r\n");
    }
    else if (screen == ALARM_SCREEN) {
        Terminal_printf("***WARNING***\r\n");
        Terminal_printf("Intruder detected!\r\n");
        Terminal_printf("Enter your PIN to disarm system: ");
    }
    else if (screen == ARMED_SCREEN) {
        Terminal_printf("System is ARMED\r\n");
        Terminal_printf("Enter your PIN to disarm system: ");
    }
    else if (screen == TIME_EDITOR_ZONE_SELECT_SCREEN) {
        Terminal_printf("Press 1 to set start time, press 2 to set end time or press * to go back (on the keypad): ");
    }
    else if (screen == TIME_EDITOR_SCREEN) {
        Terminal_printf("Enter hour: ");
    }
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

    enum TerminalScreen screen = SET_PASSWORD_SCREEN;
    print_screen(screen);

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
    bool zone_armed[4];
    memset(zone_times, 0, 4 * sizeof(struct ZoneTime));
    zone_armed[0] = true;
    zone_armed[1] = true;
    zone_armed[2] = true;
    zone_armed[3] = true;
    int reed_trigger_count[4];
    memset(zone_status, 0, 4 * sizeof(enum ZoneState));
    memset(reed_trigger_count, 0, 4 * sizeof(int));

    int password[4];
    int password_check[4];
    int password_counter = 0;

    int selected_zone = 0;
    int set_start_time = -1;
    int hour_upper = -1;
    int hour_lower = -1;
    int minute_upper = -1;
    int minute_lower = -1;

    int check_count = 0;
    int last_light = 0;
    bool blink_on = false;

    while(1) {
        if (check_count >= 100) {
            Keypad_poll();

            enum KeyPad key = Keypad_getkey();
            if (screen == SET_PASSWORD_SCREEN) {
                if (key != NONE && key != KEY_STAR && key != KEY_NUM) {
                    password[password_counter] = key;
                    password_counter++;
                    Terminal_printf("*");
                    if (password_counter >= 4) {
                        screen = HOME_SCREEN;
                        print_screen(screen);
                        password_counter = 0;
                    }
                }
            }
            else if (screen == HOME_SCREEN) {
                if (key == KEY_1) {
                    screen = TIME_EDITOR_ZONE_SELECT_SCREEN;
                    print_screen(screen);
                }
                else if (key == KEY_2) {
                    screen = ARMED_SCREEN;
                    print_screen(screen);
                }
            }
            else if (screen == ALARM_SCREEN) {
                if (key != NONE && key != KEY_STAR && key != KEY_NUM) {
                   password_check[password_counter] = key;
                   password_counter++;
                   Terminal_printf("*");
                   if (password_counter >= 4) {
                       bool check = true;
                       int i;
                       for (i = 0; i < 4; i++) {
                           check &= password_check[i] == password[i];
                       }
                       if (check) {
                          global_status = STANDBY;
                          zone_status[0] = STANDBY;
                          set_zone_led(0, 0x0);
                          zone_status[1] = STANDBY;
                          set_zone_led(1, 0x0);
                          zone_status[2] = STANDBY;
                          set_zone_led(2, 0x0);
                          zone_status[3] = STANDBY;
                          set_zone_led(3, 0x0);
                          screen = HOME_SCREEN;
                          print_screen(screen);
                          password_counter = 0;
                       }
                       else {
                           password_counter = 0;
                           print_screen(screen);
                       }
                   }
                }
            }
            else if (screen == TIME_EDITOR_ZONE_SELECT_SCREEN) {

                if (key > KEY_0 && key <= KEY_4) {
                    if (set_start_time < 0) {
                        if (key == KEY_1 || key == KEY_2) {
                            Terminal_printf("%c\r\nSelect the zone to edit or press * to go back (on the keypad): ", '0' + key);
                            set_start_time = key;
                        }
                    }
                    else {
                        selected_zone = key;
                        screen = TIME_EDITOR_SCREEN;
                        print_screen(screen);
                    }
                }
                else if (key == KEY_STAR) {
                    set_start_time = -1;
                    screen = HOME_SCREEN;
                    print_screen(screen);
                }
            }
            else if (screen == TIME_EDITOR_SCREEN) {
                if (key != NONE && key != KEY_STAR && key != KEY_NUM) {
                    if (hour_upper < 0) {
                        hour_upper = key;
                        Terminal_printf("%c", '0' + key);
                    }
                    else if (hour_lower < 0) {
                        hour_lower = key;
                        Terminal_printf("%c:", '0' + key);
                    }
                    else if (minute_upper < 0) {
                        minute_upper = key;
                        Terminal_printf("%c", '0' + key);
                    }
                    else {
                        minute_lower = key;
                        Terminal_printf("%c", '0' + key);
                        int tval = (hour_upper * 10 + hour_lower) * 60 + (minute_upper * 10 + minute_lower);
                        if (set_start_time == 1) {
                            zone_times[selected_zone - 1].start_time = tval;
                        }
                        else {
                            zone_times[selected_zone - 1].end_time = tval;
                        }
                        zone_armed[selected_zone - 1] = zone_times[selected_zone - 1].start_time == zone_times[selected_zone - 1].end_time;
                        hour_upper = -1;
                        hour_lower = -1;
                        minute_upper = -1;
                        minute_lower = -1;
                        set_start_time = -1;
                        screen = HOME_SCREEN;
                        print_screen(screen);
                    }
                }
                else if (key == KEY_STAR) {
                    set_start_time = -1;
                    hour_upper = -1;
                    hour_lower = -1;
                    minute_upper = -1;
                    minute_lower = -1;
                    screen = TIME_EDITOR_ZONE_SELECT_SCREEN;
                    print_screen(screen);
                }
            }
            int time_in_mins = rtc / 600000;
            showChar('0', pos2);
            showChar('0', pos3);
            showChar('0', pos5);
            showChar('0', pos6);
            display_number((time_in_mins / 60) * 1000);
            display_number(time_in_mins % 60);
            showChar(' ', pos4);
            uint8_t i;
            for (i = 0; i < 4; i++) {
                if (zone_times[i].start_time != zone_times[i].end_time) {
                    if (time_in_mins >= zone_times[i].start_time && time_in_mins <= zone_times[i].end_time) {
                        zone_armed[i] = true;
                    }
                    else {
                        zone_armed[i] = false;
                    }
                }
                if (!read_reed_sw(i)) {
                    reed_trigger_count[i]++;
                    if (zone_armed[i] && reed_trigger_count[i] >= 10) {
                        zone_status[i] = ALARM;
                        if (global_status != ALARM) {
                            screen = ALARM_SCREEN;
                            print_screen(screen);
                        }
                        global_status = ALARM;
                    }
                }
                else {
                    reed_trigger_count[i] = 0;
                }
            }
            check_count = 0;
        }
        if (rtc - last_light >= 3000) {
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
            last_light = rtc;
            blink_on = !blink_on;
        }

        check_count++;
    }
}

#include "keypad.h"
#include "buzzer.h"
#include "microphone.h"

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

enum PushButtonState {
    PB_STANDBY,
    PB_HOLD,
    PB_PRESSED,
};

struct ZoneTime {
    int start_time;
    int end_time;
};

/* ---------------------- */
//                        //
//  Function prototypes   //
//                        //
/* ---------------------- */

void init();
bool read_reed_sw(uint8_t sw);
void set_zone_led(uint8_t zone, uint8_t value);
int get_lcd_position(int pos);
void display_number(int num);
void display_text(const char* text, int offset);
void display_text_scroll(const char* text, bool delay);
void print_terminal_header();
void print_screen(enum TerminalScreen screen);
int num_digits(int num);

/* ---------------------- */
//                        //
//  Globals               //
//                        //
/* ---------------------- */

struct ZoneTime zone_times[4];
bool zone_armed[4];
enum ZoneState zone_status[4];
bool reed_open[4];
int max_mic_reading = 0;
int last_mic_strlen = 0;

/* ---------------------- */
//                        //
//  Main function         //
//                        //
/* ---------------------- */
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

    clearLCD();

    enum ZoneState global_status = STANDBY;
    memset(zone_times, 0, 4 * sizeof(struct ZoneTime));
    zone_armed[0] = true;
    zone_armed[1] = true;
    zone_armed[2] = true;
    zone_armed[3] = true;
    int reed_trigger_count[4];
    memset(zone_status, 0, 4 * sizeof(enum ZoneState));
    memset(reed_trigger_count, 0, 4 * sizeof(int));
    memset(reed_open, 0, 4 * sizeof(bool));

    int password[50];
    int password_check[50];
    int password_size = 0;
    int password_counter = 0;

    int selected_zone = 0;
    int set_start_time = -1;
    int hour_upper = -1;
    int hour_lower = -1;
    int minute_upper = -1;
    int minute_lower = -1;

    int mic_data[4];
    memset(mic_data, 0, 4 * sizeof(int));

    int check_count = 0;
    long long last_light = 0;
    long long last_mic_update = 0;
    bool blink_on = false;
    enum PushButtonState pbstate = PB_STANDBY;

    while(1) {

        enum TerminalScreen last_screen = screen;

        if (check_count >= 100) {

            if (pbstate == PB_STANDBY && GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN6) == 0x0) {
                pbstate = PB_HOLD;
            }
            else if (pbstate == PB_HOLD && GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN6) == 0x1) {
                pbstate = PB_PRESSED;
            }

            Keypad_poll();

            enum KeyPad key = Keypad_getkey();
            if (screen == SET_PASSWORD_SCREEN) {
                if (key != NONE && key != KEY_STAR && key != KEY_NUM) {
                    password[password_counter] = key;
                    password_counter++;
                    Terminal_printf("*");
                    if (password_counter >= 50) {
                        screen = HOME_SCREEN;
                        print_screen(screen);
                        password_counter = 0;
                        password_size = 50;
                    }
                }
                else if (pbstate == PB_PRESSED || key == KEY_NUM) {
                    if (password_counter >= 4) {
                        screen = HOME_SCREEN;
                        print_screen(screen);
                        password_size = password_counter;
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
            else if (screen == ALARM_SCREEN || screen == ARMED_SCREEN) {
                if (key != NONE && key != KEY_STAR && key != KEY_NUM) {
                   password_check[password_counter] = key;
                   password_counter++;
                   Terminal_printf("*");
                }
                if (password_counter >= 50 || pbstate == PB_PRESSED || key == KEY_NUM) {
                   bool check = password_counter == password_size;
                   int i;
                   for (i = 0; i < password_size; i++) {
                       check = check && password_check[i] == password[i];
                   }
                   if (check) {
                      Buzzer_off();
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
                        screen = HOME_SCREEN;
                        print_screen(screen);
                    }
                }
                else if (key == KEY_STAR) {
                    screen = TIME_EDITOR_ZONE_SELECT_SCREEN;
                    print_screen(screen);
                }
            }

            if (last_screen != screen && screen != TIME_EDITOR_SCREEN) {
                password_counter = 0;
                hour_upper = -1;
                hour_lower = -1;
                minute_upper = -1;
                minute_lower = -1;
                set_start_time = -1;
            }
            if (pbstate == PB_PRESSED) {
                pbstate = PB_STANDBY;
            }

            // Display time in format hh mm on LCD
            int time_in_mins = rtc / 600000;
            showChar('0', pos2);
            showChar('0', pos3);
            showChar('0', pos5);
            showChar('0', pos6);
            display_number((time_in_mins / 60) * 1000);
            display_number(time_in_mins % 60);
            showChar(' ', pos4);

            // Check reed switch status and arm zone if current time is within zone activation range
            uint8_t i;
            for (i = 0; i < 4; i++) {
                if (zone_times[i].start_time != zone_times[i].end_time) {
                    bool last = zone_armed[i];
                    if (time_in_mins >= zone_times[i].start_time && time_in_mins <= zone_times[i].end_time) {
                        zone_armed[i] = true;
                    }
                    else {
                        zone_armed[i] = false;
                    }
                    if (last != zone_armed[i] && screen == HOME_SCREEN) {
                        print_screen(screen);
                    }
                }
                bool last_reading = reed_open[i];
                if (!read_reed_sw(i)) {
                    reed_trigger_count[i]++;
                    bool setalarm = screen == ARMED_SCREEN || screen == ALARM_SCREEN;
                    if (setalarm && zone_armed[i] && reed_trigger_count[i] >= 10) {
                        zone_status[i] = ALARM;
                        if (global_status != ALARM) {
                            screen = ALARM_SCREEN;
                            print_screen(screen);
                        }
                        if (global_status != ALARM) {
                            Buzzer_on();
                        }
                        global_status = ALARM;
                    }
                    reed_open[i] = true;
                }
                else {
                    reed_trigger_count[i] = 0;
                    reed_open[i] = false;
                }
                if (reed_open[i] != last_reading) {
                    print_screen(screen);
                }
            }
            check_count = 0;
        }

        /* Check Microphone */
        if (ADCState == 0) {
            ADCState = 1; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        }
        mic_data[0] = mic_data[1];
        mic_data[1] = mic_data[2];
        mic_data[2] = mic_data[3];
        mic_data[3] = ADCResult;

        bool setalarm = screen == ARMED_SCREEN || screen == ALARM_SCREEN;
        int simpsons38th = ((3 * (3)) / 8) * (mic_data[0] + (3 * mic_data[1]) + (3 * mic_data[2]) + mic_data[3]);
        if (setalarm && zone_armed[0] && simpsons38th > 5000) {
            zone_status[0] = ALARM;
            if (global_status != ALARM) {
                Buzzer_on();
                screen = ALARM_SCREEN;
                print_screen(screen);
            }
            global_status = ALARM;
        }

        if (simpsons38th > max_mic_reading) {
            max_mic_reading = simpsons38th;
        }

        bool update = screen == HOME_SCREEN;
        if (rtc - last_mic_update >= 5000 && update) {
            int i;
            for (i = 0; i < last_mic_strlen + 23; i++) {
                Terminal_printf("\x08");
            }
            Terminal_printf("%d", max_mic_reading);
            Terminal_printf(" [");
            int threshold = (20 * (max_mic_reading - 4500)) / 500;
            for (i = 0; i < 20; i++) {
                if (i <= threshold) {
                    Terminal_printf("#");
                }
                else {
                    Terminal_printf(" ");
                }
            }
            Terminal_printf("]");
            last_mic_strlen = num_digits(max_mic_reading);
            last_mic_update = rtc;
            max_mic_reading = 0;
        }

        if (rtc - last_light >= 5000) {
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


/* ------------------------- */
//                           //
//  Function implementation  //
//                           //
/* ------------------------- */

void init() {
    Init_LCD();
    Clock_init();
    Terminal_init();
    Keypad_init();
    PWM_init();
    Microphone_init();

    // LEDs
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN3);

    // Push button
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN6);
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
    Terminal_clear();
    Terminal_printf("----------------------------------------------------------\r\n");
    Terminal_printf("    Home Security System\r\n");
    Terminal_printf("    By: Patrick Hadlaw & Erik Kuhne\r\n");
    Terminal_printf("----------------------------------------------------------\r\n");
}

void print_screen(enum TerminalScreen screen) {
    print_terminal_header();
    if (screen == HOME_SCREEN || screen == ALARM_SCREEN || screen == ARMED_SCREEN) {
        Terminal_printf("System Status: \r\n");

        int i;
        for (i = 0; i < 4; i++) {
            Terminal_printf("Zone #%d: ", i + 1);
            if (zone_status[i] == ALARM) {
                Terminal_printf("ALARM");
            }
            else {
                Terminal_printf("STANDBY");
            }
            Terminal_printf("\r\n");
        }

        Terminal_printf("\r\n");

        for (i = 0; i < 4; i++) {
            Terminal_printf("Reed Switch #%d: ", i + 1);
            if (reed_open[i]) {
                Terminal_printf("OPEN");
            }
            else {
                Terminal_printf("CLOSED");
            }
            Terminal_printf("\r\n");
        }

        Terminal_printf("----------------------------------------------------------\r\n");
    }

    if (screen == SET_PASSWORD_SCREEN) {
        Terminal_printf("\r\n\r\nEnter your (min 4 digit) PIN (using the keypad): ");
    }
    else if (screen == HOME_SCREEN) {
        Terminal_printf("Zone activation settings:\r\n");
        int i;
        for (i = 0; i < 4; i++) {
            Terminal_printf(
                    "[%d] %d:%d - %d:%d",
                    i + 1,
                    (zone_times[i].start_time / 60),
                    (zone_times[i].start_time % 60),
                    (zone_times[i].end_time / 60),
                    (zone_times[i].end_time % 60)
            );
            if (zone_armed[i]) {
                Terminal_printf(" - ARMED");
            }
            Terminal_printf("\r\n");
        }
        Terminal_printf("----------------------------------------------------------\r\n");

        Terminal_printf("\r\n\r\nSelect the following options using the keypad:\r\n\r\n");
        Terminal_printf("1) Change arm activation times\r\n");
        Terminal_printf("2) ARM the system\r\n");

        Terminal_printf("\r\n----------------------------------------------------------\r\n");
        Terminal_printf("\r\nMicrophone reading:  [                    ]");
        last_mic_strlen = 0;
    }
    else if (screen == ALARM_SCREEN) {
        Terminal_printf("***ALERT***\r\n");
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

int num_digits(int num) {
    int i = 0;
    while (num > 0) {
        num /= 10;
        i++;
    }
    return i;
}

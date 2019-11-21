#ifndef KEYPAD_H
#define KEYPAD_H

#include "terminal.h"

#define KEYPAD_STABALIZE_TIME 30

enum KeyPad {
    KEY_0 = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_STAR,
    KEY_NUM,
    NONE,
};

extern long long rtc;

extern enum KeyPad current_key;
extern bool key_wait;
extern long long key_time;

void Keypad_init();
void set_mux_control(uint8_t ctl);
void Keypad_enable();
void Keypad_disable();
bool Keypad_read(enum KeyPad key);
void Keypad_poll();
enum KeyPad Keypad_getkey();
void RTC_ISR(void);


#endif // KEYPAD_H

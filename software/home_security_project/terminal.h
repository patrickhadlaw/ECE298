#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>

#include "driverlib/driverlib.h"
#include "hal_LCD.h"

#define TERMINAL_CLEAR_LINES 250

extern bool terminal_enabled;

typedef void(*ProgramFun) (char*);

struct option {
    char* name;
    ProgramFun func;
};

struct lcd_terminal_program {
    struct option* options;
    int length;
};

void Clock_init();
int Terminal_init();
void Terminal_enable();
void Terminal_disable();
void Terminal_close();

int Terminal_printf(const char* format, ...);

int Terminal_clear();

void EUSCIA0_ISR(void);

#endif // TERMINAL_H

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driverlib/driverlib.h"
#include "hal_LCD.h"

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
void Terminal_close();

int Terminal_printf(const char* format, ...);

void EUSCIA0_ISR(void);

#endif // TERMINAL_H

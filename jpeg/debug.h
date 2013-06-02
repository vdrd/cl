#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
//#include "config.h"
#define _printInfo_ 
void printInfo(int condition, const char *fmt_string, ...);
void printError(int condition, const char *fmt_string, ...);

#endif
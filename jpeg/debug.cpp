#ifndef DEBUG_CPP
#define DEBUG_CPP

#include "debug.h" 

void printInfo(int condition, const char *fmt_string, ...)  
{
#ifdef _printInfo_
	if(condition)
	{
		va_list arg;
		va_start(arg,fmt_string);
		vprintf_s(fmt_string,arg); 
		va_end(arg);
	}
#endif 
}

void printError(int condition, const char *fmt_string, ...)
{
#ifdef _printError_
	if(condition)
	{
		va_list arg;
		va_start(arg,fmt_string);
		vprintf_s(fmt_string,arg);
		va_end(arg);
		getchar();
		exit(-1);
	}
#endif
}

#endif
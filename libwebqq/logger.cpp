/**
 * @file   logger.c
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Sun May 20 23:25:33 2012
 * 
 * @brief  Linux WebQQ Logger API
 * 
 * 
 */

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
#endif

#include "logger.h"

static const char *levels[] = {
	"DEBUG",
	"NOTICE",
	"WARNING",
	"ERROR",
};

static FILE* logto = stderr;

/** 
 * This is standard logger function
 * 
 * @param level Which level of this message, e.g. debug
 * @param file Which file this function called in
 * @param line Which line this function call at
 * @param function Which function call this function 
 * @param msg Log message
 */
void lwqq_log(int level, const char *file, int line,
              const char *function, const char* msg, ...)
{
	va_list  va;
	time_t  t = time(NULL);
	struct tm *tm;
	int buf_used = 0;
	char date[256];

	tm = localtime(&t);
	strftime(date, sizeof(date), "%b %e %T", tm);
	
    if(level > 1){
		fprintf(logto, "[%s] %s[%ld]: %s:%d %s: \n\t", date, levels[level],
#ifdef _WIN32
			(long)GetCurrentProcessId(),
#else
			(long)getpid(),
#endif
			file, line, function);
		
	}

	va_start (va, msg);
	vfprintf(logto , msg, va);
	va_end(va);
}


#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

extern FILE *log_file;
extern pthread_mutex_t log_mutex;

void ehsan_logger_init(void);
void ehsanLog(const char *fmt, ...);
void ehsanLog(va_list args, const char *fmt);
void ehsanLogNoNewLine(const char *fmt, ...);
void ehsan_logger_shutdown(void);

#endif // LOGGER_H
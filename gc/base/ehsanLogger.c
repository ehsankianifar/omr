#include "ehsanLogger.h"

FILE *log_file = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void ehsan_logger_init(void) {
    if (log_file) return; // Already initialized
    const char *path = getenv("EHSAN_LOG_FILE");
    if (path) {
        log_file = fopen(path, "a");
    }
}

void ehsanLog(const char *fmt, ...) {
    if (!log_file) return;

    pthread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    va_end(args);

    pthread_mutex_unlock(&log_mutex);
}

void ehsanLogNoNewLine(const char *fmt, ...) {
    if (!log_file) return;

    pthread_mutex_lock(&log_mutex);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    fflush(log_file);
    va_end(args);

    pthread_mutex_unlock(&log_mutex);
}

void ehsan_logger_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}
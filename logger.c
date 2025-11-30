#define _POSIX_C_SOURCE 199309L // necessário para vscode não reclamar de CLOCK_REALTIME, caso haja outra solução pode ser excluido
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

// Mutex privado apenas deste arquivo
static pthread_mutex_t log_mutex;

void log_init() {
    pthread_mutex_init(&log_mutex, NULL);
}

void log_destroy() {
    pthread_mutex_destroy(&log_mutex);
}

void log_print(const char* format, ...) {
    // 1. Bloqueia o acesso ao terminal para ser exclusivo desta thread
    pthread_mutex_lock(&log_mutex);

    // 2. Obtém o tempo atual
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    // Converte para estrutura de tempo legível (Horas:Min:Seg)
    struct tm *tm_info = localtime(&ts.tv_sec);
    
    // Calcula milissegundos
    long ms = ts.tv_nsec / 1000000;

    // 3. Imprime o Timestamp: [HH:MM:SS.ms]
    printf("[%02d:%02d:%02d.%03ld] ", 
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ms);

    // 4. Processa a mensagem formatada (igual printf)
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // Adiciona quebra de linha automática
    printf("\n");

    // 5. Libera o terminal para outras threads
    pthread_mutex_unlock(&log_mutex);
}
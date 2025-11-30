#ifndef LOGGER_H
#define LOGGER_H

// Inicializa o mutex de log (Chamar no início da main)
void log_init();

// Destroi o mutex (Chamar no final da main)
void log_destroy();

/* Função principal de log.
   Uso idêntico ao printf, mas adiciona timestamp e garante thread-safety.
   Exemplo: log_print("Aeronave %d solicitou setor %d", id_nave, id_setor);
*/
void log_print(const char* format, ...);

#endif
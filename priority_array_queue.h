#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdlib.h>

typedef struct {
    int id_aeronave;
    int id_setor_desejado;
} DadosSolicitacao;

// --- ESTRUTURA DA LISTA
typedef struct Node {
    DadosSolicitacao data;
    unsigned int priority; // Guardamos a prioridade aqui para comparar
    struct Node* next;
} Node;

// A "Fila" agora é apenas um ponteiro para o início da lista
typedef struct {
    Node* head;
    int size;
} priority_array_queue;


void initialize(priority_array_queue* queue); 
void destroy(priority_array_queue* queue);

// Inserção ordenada
void enqueue(priority_array_queue* queue, DadosSolicitacao value, unsigned int priority);

// Retira o elemento de maior prioridade (o head)
DadosSolicitacao dequeue(priority_array_queue* queue);

int is_empty(priority_array_queue* queue);

void remove_specific_request(priority_array_queue* queue, int id_aeronave);

#endif
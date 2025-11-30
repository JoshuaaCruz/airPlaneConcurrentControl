#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdlib.h>

//UTILIZANDO AGORA LISTA ENCADEADA, MUITO MAIS FÁCIL, PORÉM DEIXEI A CIRCULAR CASO DISCORDEM

// --- DEFINIÇÃO DOS DADOS (O Contrato) ---
typedef struct {
    int id_aeronave;
    int id_setor_desejado;
    int tipo_movimento; 
} DadosSolicitacao;

// --- ESTRUTURA DA LISTA (Baseado no Node do seu C++) ---
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

// --- ASSINATURAS ---

void initialize(priority_array_queue* queue); // Mudou: não precisa de size
void destroy(priority_array_queue* queue);

// Inserção ordenada (A lógica do insert_sorted do seu C++)
void enqueue(priority_array_queue* queue, DadosSolicitacao value, unsigned int priority);

// Retira o elemento de maior prioridade (o head)
DadosSolicitacao dequeue(priority_array_queue* queue);

// Espia o primeiro
DadosSolicitacao peek(priority_array_queue* queue);

int is_empty(priority_array_queue* queue);

#endif
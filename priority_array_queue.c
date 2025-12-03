#include "priority_array_queue.h"
#include <stdio.h>
#include <stdlib.h>

void initialize(priority_array_queue* queue) {
    queue->head = NULL;
    queue->size = 0;
}

void destroy(priority_array_queue* queue) {
    while (!is_empty(queue)) {
        dequeue(queue);
    }
}

void enqueue(priority_array_queue* queue, DadosSolicitacao value, unsigned int priority) {
   

    Node* novo_no = (Node*) malloc(sizeof(Node));

    if (novo_no == NULL) { exit(1); }

    novo_no->data = value;
    novo_no->priority = priority;
    novo_no->next = NULL;

    // Fila vazia OU Nova prioridade é maior que a do Head
    if (queue->head == NULL || priority > queue->head->priority) {
        novo_no->next = queue->head;
        queue->head = novo_no;
    } 
    else {
        // Inserção no meio ou fim (Percorre a lista)
        Node* atual = queue->head;
        
        // Caminha enquanto houver próximo E a prioridade do próximo for maior ou igual à nova
        while (atual->next != NULL && atual->next->priority >= priority) {
            atual = atual->next;
        }
        
        // Insere o nó na posição encontrada
        novo_no->next = atual->next;
        atual->next = novo_no;
    }
    
    queue->size++;
}

DadosSolicitacao dequeue(priority_array_queue* queue) {
    DadosSolicitacao dados_vazios = {-1, -1}; // Retorno de erro caso vazio
    
    if (is_empty(queue)) return dados_vazios;

    // Remove do início (Head), pois já está ordenado por prioridade
    Node* temp = queue->head;
    DadosSolicitacao dados = temp->data;
    
    queue->head = queue->head->next;
    free(temp);
    
    queue->size--;
    return dados;
}

int is_empty(priority_array_queue* queue) {
    return (queue->head == NULL);
}

void remove_specific_request(priority_array_queue* queue, int id_aeronave) {
    if (is_empty(queue)) return;

    Node* atual = queue->head;
    Node* anterior = NULL;

    // Procura o nó que tem o ID da aeronave
    while (atual != NULL) {
        if (atual->data.id_aeronave == id_aeronave) {

            if (anterior == NULL) {
                // Se é o primeiro da fila (Head)
                queue->head = atual->next;
            } else {
                // Meio ou fim da fila
                anterior->next = atual->next;
            }
            
            free(atual); // Libera a memória do nó
            queue->size--;
            return;
        }
        
        // Avança os ponteiros
        anterior = atual;
        atual = atual->next;
    }
}
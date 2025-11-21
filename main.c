// CÓDIGO POTENCIALMENTE SENSÍVEL À ANTIVIRUS NÃO FAÇO ABSOLUTAMENTE A MENOR IDEIA!!!

/*
    A existência desse arquivo por ora é completamente desnecessária, criei por que
    achei que seria um "adiantamento".
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "priority_array_queue.h"

typedef struct {
    int aeronave_id;
    unsigned int prioridade;
} Solicitacao;

typedef struct {
    int id;
    pthread_mutex_t mutex;       // um mutex por setor
    int aeronave_atual_id;       // -1 se desocupado
} Setor;

typedef struct {
    int id;                         
    pthread_t thread;               
    unsigned int prioridade;        
    int *rota;                      
    int tamanho_rota;
    int setor_atual_index;          
    long tempo_espera_total;        
} Aeronave;

void does_the_thing(void* threads, void* sectors) {
    // the function that does the thing
    printf("(the thing)");
}

int main(int argc, char** argv) {

    unsigned int n_sectors = atoi(argv[1]), n_threads = atoi(argv[2]); //n_aeronaves
    if (n_sectors == 0 || n_threads == 0) return -1;

    srand(time(NULL));

    // não sei o que ele poderia fazer aqui no meio ou se ele iria fazer direto o que
    // tem que fazer na função imediatamente embaixo

    Setor *setores = (Setor *)malloc(n_sectors * sizeof(Setor));

    for (int i = 0; i < n_sectors; i++) { //inicializando setores
        setores[i].id = i;
        setores[i].aeronave_atual_id = -1; // só para marcar que tá livre
        
        pthread_mutex_init(&setores[i].mutex, NULL);
    }

    Aeronave *aeronaves = (Aeronave *)malloc(n_threads * sizeof(Aeronave));
    for (int i = 0; i < n_threads; i++) {
        aeronaves[i].id = i;                                

        aeronaves[i].prioridade = (rand() % 1000) + 1;   //sla prioridade aleatoria
        
        // criando rota simples indo de setor em setor:
        aeronaves[i].tamanho_rota = n_sectors;
        aeronaves[i].rota = (int *)malloc(n_sectors * sizeof(int));
        for (int j = 0; j < n_sectors; j++) {
            aeronaves[i].rota[j] = j; 
        }

        aeronaves[i].setor_atual_index = 0;
        aeronaves[i].tempo_espera_total = 0;
    }

    //does_the_thing(n_sectors, n_threads);
    return 0;
}
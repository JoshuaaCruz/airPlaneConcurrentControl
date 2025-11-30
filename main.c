#define _POSIX_C_SOURCE 199309L // necessário para vscode não reclamar de CLOCK_REALTIME, caso haja outra solução pode ser excluido
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "priority_array_queue.h"
#include "logger.h"

// --- ESTRUTURAS DO SISTEMA ---

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
    
    // Sincronização
    pthread_cond_t cond_liberacao;  // Onde a aeronave dorme esperando a Torre
    int permissao_concedida;        // Flag: 0 = esperando, 1 = liberado

    struct timespec inicio_espera; // Marca a hora que pediu
    double tempo_total_espera;     // Acumula (Fim - Inicio) em segundos
} Aeronave;

// --- VARIÁVEIS GLOBAIS (O ESTADO DO MUNDO) ---

Setor *setores;
Aeronave *aeronaves;
int n_sectors_global = 0;  // Armazenar número de setores globalmente

//Array de filas, uma para cada setor da aplicação
priority_array_queue *filas_espera;

volatile int sistema_ativo = 1; // 1 = Rodando, 0 = Parar

// Mutex Geral: Protege a fila de pedidos e a integridade do sistema
pthread_mutex_t mutex_geral;        
pthread_cond_t cond_novo_pedido;    // Avisa o controle que tem gente querendo voar
pthread_cond_t cond_setor_liberado; // (Opcional) Avisa controle que alguém saiu

// --- FUNÇÕES AUXILIARES (INTERFACE) ---

/* Verifica se todas as filas de espera estão vazias
   Retorna: 1 se TODAS as filas estão vazias
            0 se HOUVER alguma solicitação pendente em qualquer fila
   
   Deve ser chamada COM O MUTEX_GERAL TRAVADO!
*/
int todas_filas_vazias() {
    for (int i = 0; i < n_sectors_global; i++) {
        if (!is_empty(&filas_espera[i])) {
            return 0;  // Encontrou uma fila não vazia
        }
    }
    return 1;  // Todas as filas estão vazias
}

/* DEV 2: UTILIZE ESTA FUNÇÃO. NÃO MODIFIQUE.
   Esta função faz a aeronave "pedir" o setor e dormir até a torre autorizar.
*/
void solicitar_acesso(int id_aeronave, int id_setor_desejado, int prioridade, Aeronave* minha_struct) {

    clock_gettime(CLOCK_REALTIME, &minha_struct->inicio_espera); //hora que pediu acesso
    
    pthread_mutex_lock(&mutex_geral); // Trava o sistema para falar com a torre
    
    // 1. Cria o pacote de dados
    DadosSolicitacao dados;
    dados.id_aeronave = id_aeronave;
    dados.id_setor_desejado = id_setor_desejado;
    dados.tipo_movimento = 1; // 1 = Passagem/Entrada
    
    log_print("Aeronave %d solicitando Setor %d (Prio: %d)", id_aeronave, id_setor_desejado, prioridade);

    // 2. Entra na fila ESPECÍFICA daquele setor
    // O Dev 3 (Controle) agora vai olhar fila por fila
    enqueue(&filas_espera[id_setor_desejado], dados, prioridade);
    
    // 3. Acorda a Torre de Controle
    pthread_cond_signal(&cond_novo_pedido);
    
    // 4. Dorme e aguarda autorização
    minha_struct->permissao_concedida = 0;
    while (minha_struct->permissao_concedida == 0) {
        pthread_cond_wait(&minha_struct->cond_liberacao, &mutex_geral);
    }

    // Logo após acordar (quando ganha permissão):
    struct timespec fim_espera;
    clock_gettime(CLOCK_REALTIME, &fim_espera);

    // Calcula delta e soma (simplificado): conferir se isso aqui tá certo
    double segundos = (fim_espera.tv_sec - minha_struct->inicio_espera.tv_sec);
    double nanos = (fim_espera.tv_nsec - minha_struct->inicio_espera.tv_nsec);
    minha_struct->tempo_total_espera += segundos + (nanos / 1e9);
    
    log_print("Aeronave %d obteve permissão para o Setor %d", id_aeronave, id_setor_desejado);
    
    pthread_mutex_unlock(&mutex_geral); // Destrava e segue viagem
}

/*
    DEV 2: UTILIZE ESTA FUNÇÃO AO SAIR DE UM SETOR.
*/
void notificar_saida(int id_setor, int id_aeronave) {
    pthread_mutex_lock(&mutex_geral);
    
    if (setores[id_setor].aeronave_atual_id == id_aeronave) {
        setores[id_setor].aeronave_atual_id = -1; // Libera o setor
        log_print("Aeronave %d liberou o Setor %d", id_aeronave, id_setor);
        
        // Avisa a torre que vagou um espaço (importante para evitar deadlock)
        pthread_cond_signal(&cond_novo_pedido); 
    }
    
    pthread_mutex_unlock(&mutex_geral);
}


// --- TAREFA DO DEV 2 (AERONAVES) ---

void* thread_aeronave(void* arg) {
    Aeronave* nave = (Aeronave*) arg;
    
    log_print("Aeronave %d iniciou voo.", nave->id);

    int setor_anterior = -1; // Começa sem nenhum setor

    // Loop pela rota definida
    for (int i = 0; i < nave->tamanho_rota; i++) {
        int proximo_setor = nave->rota[i];

        // 1. Tenta entrar no próximo setor (e dorme se estiver ocupado)
        solicitar_acesso(nave->id, proximo_setor, nave->prioridade, nave);

        // 2. Agora que garantiu o novo, libera o anterior (se existir)
        if (setor_anterior != -1) {
            notificar_saida(setor_anterior, nave->id);
        }

        // 3. Voa (Simulação de tempo)
        // Manter uso do usleep como recomendado pelo professor
        usleep((rand() % 500) * 1000); 

        // 4. Atualiza onde estou
        setor_anterior = proximo_setor;
    }

    // 5. Saiu do espaço aéreo (libera o último setor que ficou segurando)
    if (setor_anterior != -1) {
        notificar_saida(setor_anterior, nave->id);
    }
    
    log_print("Aeronave %d concluiu a rota e pousou.", nave->id);
    return NULL;
}


// --- TAREFA DO DEV 3 (CONTROLE) ---

void* thread_controle(void* arg) {
    while (sistema_ativo) { 
        pthread_mutex_lock(&mutex_geral);
        
        // Se não tem pedidos E o sistema ainda está ativo, dorme
        while (todas_filas_vazias() && sistema_ativo) {
             pthread_cond_wait(&cond_novo_pedido, &mutex_geral);
        }
        
        if (!sistema_ativo) {
            pthread_mutex_unlock(&mutex_geral);
            break; // Sai do loop para encerrar a thread
        }

        // OUTRAS COISAS
    }
    return NULL;
}


int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Uso: %s <n_setores> <n_aeronaves>\n", argv[0]);
        return 1;
    }

    log_init();
    
    int n_sectors = atoi(argv[1]);
    int n_threads = atoi(argv[2]);
    
    n_sectors_global = n_sectors;  // Armazena globalmente para usar em todas_filas_vazias()

    // INICIALIZAÇÃO DAS FILAS (AGORA É UM ARRAY)
    filas_espera = (priority_array_queue*) malloc(n_sectors * sizeof(priority_array_queue));
    for(int i=0; i < n_sectors; i++) {
        initialize(&filas_espera[i]); // Inicializa uma fila para cada setor
    }

    pthread_mutex_init(&mutex_geral, NULL);
    pthread_cond_init(&cond_novo_pedido, NULL);
    pthread_cond_init(&cond_setor_liberado, NULL);

    // Inicializa Setores
    setores = (Setor *)malloc(n_sectors * sizeof(Setor));
    for (int i = 0; i < n_sectors; i++) {
        setores[i].id = i;
        setores[i].aeronave_atual_id = -1; // só para marcar que tá livre
        
        pthread_mutex_init(&setores[i].mutex, NULL);
    }

    // Inicializa Aeronaves
    aeronaves = (Aeronave *)malloc(n_threads * sizeof(Aeronave));
    for (int i = 0; i < n_threads; i++) {
        aeronaves[i].id = i;
        aeronaves[i].prioridade = (rand() % 1000) + 1; //sla prioridade aleatoria
        aeronaves[i].tempo_total_espera = 0.0;
        aeronaves[i].tamanho_rota = n_sectors;
        aeronaves[i].rota = (int *)malloc(n_sectors * sizeof(int));
        
        // Gera rota sequencial 0 -> 1 -> 2 ... 
        for (int j = 0; j < n_sectors; j++) {
            aeronaves[i].rota[j] = j; 
        }

        // Init Condicional da Aeronave
        pthread_cond_init(&aeronaves[i].cond_liberacao, NULL);
        aeronaves[i].permissao_concedida = 0;
    }

    // Criação das Threads
    pthread_t t_controle;
    pthread_create(&t_controle, NULL, thread_controle, (void*)&n_sectors);

    for (int i = 0; i < n_threads; i++) {
        pthread_create(&aeronaves[i].thread, NULL, thread_aeronave, (void*)&aeronaves[i]);
    }

    // Join (Espera as naves terminarem)
    for (int i = 0; i < n_threads; i++) {
        pthread_join(aeronaves[i].thread, NULL);
    }

    //Flag para thread controle morrer
    sistema_ativo = 0;

    // 2. Acorda o controle (caso ele esteja dormindo no cond_wait) para ele ler o aviso
    pthread_mutex_lock(&mutex_geral);
    pthread_cond_broadcast(&cond_novo_pedido); 
    pthread_mutex_unlock(&mutex_geral);

    // 3. Espera o controle terminar o loop dele
    pthread_join(t_controle, NULL);

    
    // Limpeza de Memória (Destroys)
    for(int i=0; i < n_sectors; i++) {
        destroy(&filas_espera[i]);
    }
    free(filas_espera);

    log_destroy();
    free(setores);
    free(aeronaves);

    //TODO: DEV 1: PRINTAR MÉDIAS DAS AERONAVES E MÉDIA GLOBAL AQUI
    
    return 0;
}
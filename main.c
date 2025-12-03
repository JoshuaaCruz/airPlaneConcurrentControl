#define _POSIX_C_SOURCE 199309L // necessário para vscode não reclamar de CLOCK_REALTIME
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "priority_array_queue.h"
#include "logger.h"
#include <stdbool.h> 

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
    bool tenhoPermissao;

    struct timespec inicio_espera; // Marca a hora que pediu permissão
    double tempo_total_espera;     // Acumula (Fim - Inicio) em segundos
} Aeronave;

//  VARIÁVEIS GLOBAIS 

Setor *setores;
Aeronave *aeronaves;
int n_sectors_global = 0;  // Armazenar número de setores globalmente

//Array de filas, uma para cada setor da aplicação
priority_array_queue *filas_espera;

volatile bool sistema_ativo = true;

// Mutex Geral: Protege a fila de pedidos e a integridade do sistema
pthread_mutex_t mutex_geral;        
pthread_cond_t cond_alteracao_feita;    // Avisa o controle que houve alteracao nos setores

//  FUNÇÕES AUXILIARES 

int todas_filas_vazias() {
    for (int i = 0; i < n_sectors_global; i++) {
        if (!is_empty(&filas_espera[i])) {
            return 0;  // Encontrou uma fila não vazia
        }
    }
    return 1;  // Todas as filas estão vazias
}

bool solicitar_acesso(int id_aeronave, int id_setor_desejado, int prioridade, Aeronave* aeronaveSolicitante) {

        log_print("FILA Setor %d: %d aeronaves esperando", 
          id_setor_desejado, 
          (&filas_espera[id_setor_desejado])->size );

    clock_gettime(CLOCK_REALTIME, &aeronaveSolicitante->inicio_espera); //hora que pediu acesso

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 3; // +3 segundos de paciência
    // Normaliza os nanosegundos (se passar de 1 bi, incrementa segundos)
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
    }

    
    pthread_mutex_lock(&mutex_geral); // Trava o sistema para falar com a torre
    
    // Cria o pacote de dados da Aeronave que esta solicitando 
    DadosSolicitacao dadosAeronave = {id_aeronave, id_setor_desejado};
    
    log_print("Aeronave %d solicitando Setor %d (Prio: %d)", id_aeronave, id_setor_desejado, prioridade);

    // Entra na fila ESPECÍFICA daquele setor
    enqueue(&filas_espera[id_setor_desejado], dadosAeronave, prioridade);
    
    //  Acorda a Torre de Controle 
    pthread_cond_signal(&cond_alteracao_feita);
    
    //  Dorme e aguarda autorização
    aeronaveSolicitante->tenhoPermissao = false;

    int ret = 0; //serve para salvar o retorno de timedwait e ver se deu timeout
    while (aeronaveSolicitante->tenhoPermissao == false && ret == 0) {
        //Irá dormir até alguém acordar ou até tempo acabar (ret != 0)
        ret = pthread_cond_timedwait(&aeronaveSolicitante->cond_liberacao, &mutex_geral, &ts); //quando ele dá cond_wait ele libera o mutex geral e espera por alguém que dê signal pra ele "acordar", que no caso só quem dá é a torre de controle
    }
    //quando é acordado recupera o mutex geral, por isso tem que dar unlock depois
    
    // Verificação se recebeu permissão, se não acordou por causa do timeout
    if (aeronaveSolicitante->tenhoPermissao == true)
    {   
        // Calcula diferença e soma 
        struct timespec fim_espera;
        clock_gettime(CLOCK_REALTIME, &fim_espera);
        double segundos = (fim_espera.tv_sec - aeronaveSolicitante->inicio_espera.tv_sec);
        double nanos = (fim_espera.tv_nsec - aeronaveSolicitante->inicio_espera.tv_nsec);
        aeronaveSolicitante->tempo_total_espera += segundos + (nanos / 1e9);


        log_print("Aeronave %d obteve permissão para o Setor %d", id_aeronave, id_setor_desejado);
        pthread_mutex_unlock(&mutex_geral); // Destrava e segue viagem
        return true;

    } else {
        // TIMEOUT (ret = ETIMEDOUT)

        // Aeronave remove próprio pedido da fila antes de sair
        remove_specific_request(&filas_espera[id_setor_desejado], id_aeronave);

        log_print("Aeronave %d DESISTIU e REMOVEU PEDIDO do Setor %d (Timeout)", id_aeronave, id_setor_desejado);
        pthread_mutex_unlock(&mutex_geral);
        return false; 
    }
}

void notificar_saida(int id_setor, int id_aeronave) {
    pthread_mutex_lock(&mutex_geral);
    
    if (setores[id_setor].aeronave_atual_id == id_aeronave) {
        setores[id_setor].aeronave_atual_id = -1; // Libera o setor
        log_print("Aeronave %d liberou o Setor %d", id_aeronave, id_setor);
        
        // Avisa a torre que vagou um espaço (evitar deadlock)
        pthread_cond_signal(&cond_alteracao_feita); 
    }
    
    pthread_mutex_unlock(&mutex_geral);
}


void* thread_aeronave(void* arg) {
    Aeronave* nave = (Aeronave*) arg;
    
    log_print("Aeronave %d decolou.", nave->id);

    int setor_anterior = -1; // Começa sem nenhum setor

    // Loop pela rota definida
    for (int i = 0; i < nave->tamanho_rota; i++) {
        int proximo_setor = nave->rota[i];

        //  Tenta entrar no próximo setor (e dorme se estiver ocupado), caso durma demais, retorna um false
        bool conseguiuAcesso = solicitar_acesso(nave->id, proximo_setor, nave->prioridade, nave);

        if (conseguiuAcesso)
        {

            if (setor_anterior != -1) //se não foi o primeiro setor que ele conseguiu entrar
            {
                notificar_saida(setor_anterior, nave->id); //fala pra central liberar o anterior
            }

            //  Voa (Simulação de tempo)
            // Manter uso do usleep como recomendado pelo professor
            usleep((rand() % 100 ) * 1000); 

            // Atualiza onde aeronava esta
            setor_anterior = proximo_setor;
        
        } else {
            //não conseguiu acesso, timeout
            
            if (setor_anterior != -1) //se essa aeronave esta segurando algum setor
            {
                notificar_saida(setor_anterior, nave->id);
                setor_anterior = -1; //a aeronave agora não controla mais nenhum setor
                //ela liberou o que ela tinha já que esperou muito tempo tentando pegar o próximo e vai esperar um tempo random pra tentar pegar o proximo enquanto voa no limbo
            }

            //  Espera aleatória (para evitar Livelock/Colisão)
            usleep((50 + rand() % 100) * 1000); // 0.5s a 1.0s

            i--; //decrementando i para no próximo loop ele tentar novamente esse setor

        }
    }

    // Saiu do espaço aéreo (libera o último setor que ficou segurando)
    if (setor_anterior != -1) {
        notificar_saida(setor_anterior, nave->id);
    }
    
    log_print("Aeronave %d concluiu a rota e pousou.", nave->id);
    double media_espera = 0.0;
    if (nave->tamanho_rota > 0) {
        media_espera = nave->tempo_total_espera / nave->tamanho_rota;
    }
    
    printf("ESTATISTICA: Aeronave %d concluiu a rota e pousou. Tempo MEDIO de espera: %.6f segundos.\n", 
          nave->id, media_espera);
    return NULL;
}


void* thread_controle(void* arg) {

    int n_sectors = *(int*)arg;

    while (sistema_ativo) { 
        pthread_mutex_lock(&mutex_geral);
        
        // Se não tem pedidos E o sistema ainda está ativo, dorme
        while (todas_filas_vazias() && sistema_ativo) {
             pthread_cond_wait(&cond_alteracao_feita, &mutex_geral);
        }
        
        if (!sistema_ativo) {
            pthread_mutex_unlock(&mutex_geral);
            break; // Sai do loop para encerrar a thread
        }

        // CASO CHEGUE AQUI ENTÃO ALGUMA FILA NÃO ESTÁ VAZIA E O SISTEMA AINDA ESTÁ ATIVO

        for (int i = 0; i < n_sectors; i++) {
            
            // Procurando pela fila que não tem aeronava e está vazia
            if (setores[i].aeronave_atual_id == -1 && !is_empty(&filas_espera[i])) {
                
                DadosSolicitacao dadosAviaoAtual = dequeue(&filas_espera[i]);
                
                //Atualiza setor com o id da aeronave, diz que essa aeronave tem permissão e acorda ela para voar
                setores[i].aeronave_atual_id = dadosAviaoAtual.id_aeronave;
                aeronaves[dadosAviaoAtual.id_aeronave].tenhoPermissao = true;
                
                pthread_cond_signal(&aeronaves[dadosAviaoAtual.id_aeronave].cond_liberacao);

                log_print("[TORRE] Autorizado: Nave %d entrando no Setor %d", dadosAviaoAtual.id_aeronave, i);
            }
        }

        pthread_mutex_unlock(&mutex_geral);
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

    if (n_sectors <= 0 || n_threads <= 0) {
        printf("Erro: Valores devem ser positivos...\n");
        return 1;
    }
    
    n_sectors_global = n_sectors;  // Armazena globalmente para usar em todas_filas_vazias()

    // INICIALIZAÇÃO DAS FILAS DE ESPERA DOS SETORES
    filas_espera = (priority_array_queue*) malloc(n_sectors * sizeof(priority_array_queue));
    for(int i=0; i < n_sectors; i++) {
        initialize(&filas_espera[i]); // Inicializa uma fila para cada setor
    }

    pthread_mutex_init(&mutex_geral, NULL);
    pthread_cond_init(&cond_alteracao_feita, NULL);

    // Inicializa Setores
    setores = (Setor *)malloc(n_sectors * sizeof(Setor));
    for (int i = 0; i < n_sectors; i++) {
        setores[i].id = i;
        setores[i].aeronave_atual_id = -1; // marcação de setor livre
        
        pthread_mutex_init(&setores[i].mutex, NULL);
    }

    // Inicializa Aeronaves
    aeronaves = (Aeronave *)malloc(n_threads * sizeof(Aeronave));
    for (int i = 0; i < n_threads; i++) {
        aeronaves[i].id = i;
        aeronaves[i].prioridade = (rand() % 1000) + 1; // prioridade aleatoria
        aeronaves[i].tempo_total_espera = 0.0;
        
        int tamanho_random = (rand() % (n_sectors * 2)) + 1;
        if (tamanho_random < 2) tamanho_random = 2; // Garante pelo menos 2 passos

        aeronaves[i].tamanho_rota = tamanho_random;
        aeronaves[i].rota = (int *)malloc(tamanho_random * sizeof(int));

        for (int j = 0; j < tamanho_random; j++) {
            int setor_sorteado = rand() % n_sectors;
            
            //  Só entra no while se houver mais de 1 setor no mundo
            if (j > 0 && n_sectors > 1) { 
                while (setor_sorteado == aeronaves[i].rota[j-1]) {
                    setor_sorteado = rand() % n_sectors;
                }
            }
            aeronaves[i].rota[j] = setor_sorteado;
        }

        // Init Condicional da Aeronave
        pthread_cond_init(&aeronaves[i].cond_liberacao, NULL);
        aeronaves[i].tenhoPermissao = false;
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
    sistema_ativo = false;

    // Acorda o controle (caso ele esteja dormindo no cond_wait) para ele ler o aviso
    pthread_mutex_lock(&mutex_geral); 
    pthread_cond_broadcast(&cond_alteracao_feita); 
    pthread_mutex_unlock(&mutex_geral);

    pthread_join(t_controle, NULL);

    printf("\n===============================================================================\n");
    printf(" RELATÓRIO FINAL DE TEMPOS DE ESPERA\n");
    printf("===============================================================================\n");
    // Ajustei a largura das colunas para caber a Prioridade
    printf("%-10s | %-10s | %-15s | %-15s\n", "Aeronave", "Prioridade", "Rota (Setores)", "Tempo Médio (s)");
    printf("-------------------------------------------------------------------------------\n");

    double soma_medias_global = 0.0;
    
    // Variáveis para a média ponderada
    double soma_tempo_ponderado = 0.0;
    long soma_total_prioridades = 0;

    for (int i = 0; i < n_threads; i++) {
        double media_nave = 0.0;
        
        if (aeronaves[i].tamanho_rota > 0) {
            media_nave = aeronaves[i].tempo_total_espera / aeronaves[i].tamanho_rota;
        }

        soma_medias_global += media_nave;

        // Acumula para a média ponderada: (Tempo * Peso)
        soma_tempo_ponderado += (media_nave * aeronaves[i].prioridade);
        
        // Acumula o peso total
        soma_total_prioridades += aeronaves[i].prioridade;

        printf("Nave %-5d | %-10d | %-15d | %-15.6f\n", 
               aeronaves[i].id,
               aeronaves[i].prioridade, // Exibindo a prioridade na tabela
               aeronaves[i].tamanho_rota, 
               media_nave);
    }

    printf("-------------------------------------------------------------------------------\n");
    
    if (n_threads > 0) {
        // 1. Média Aritmética Simples (Todas as naves valem o mesmo)
        printf("MÉDIA SIMPLES (Geral)    : %.6f s\n", soma_medias_global / n_threads);
        
        // 2. Média Ponderada (Considera a importância da nave)
        if (soma_total_prioridades > 0) {
            double media_ponderada = soma_tempo_ponderado / soma_total_prioridades;
            printf("MÉDIA PONDERADA (Prio)   : %.6f s\n", media_ponderada);
        } else {
            printf("MÉDIA PONDERADA (Prio)   : N/A (Soma das prioridades é 0)\n");
        }
    }
    printf("===============================================================================\n");

    // Limpeza de Memória (Destroys)
    for(int i=0; i < n_sectors; i++) {
        destroy(&filas_espera[i]);
    }
    free(filas_espera);

    log_destroy();
    free(setores);
    free(aeronaves);

    return 0;
}
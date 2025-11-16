// CÓDIGO POTENCIALMENTE SENSÍVEL À ANTIVIRUS NÃO FAÇO ABSOLUTAMENTE A MENOR IDEIA!!!
// Não sei se eu deveria por este arquivo como .h em vez de .c, visto que seu propósito
// é utilitário.
// Também não sei se é melhor fazer uma lista em vez de uma fila.

#include <stdio.h>
#include <stdlib.h>

/*
    struct criada exclusivamente para fazer a fila funcionar; tentei criar a fila com
    ponteiros void, mas o C simplesmente não aceita inserções em arrays nesta ocasião.
*/
typedef struct {
    char* name;
    unsigned int age;
} person;

/*
    struct que representa a "instância" de uma fila circular prioritária. Seu diferencial
    é a existência de um array de "prioridades", que funciona sempre se maneira síncrona
    ao array de valores, pois cada valor terá uma prioridade. É necessário sempre armazenar
    as prioridades pois, se não, não se saberia a próxima prioridade ao retirar um valor.
    A criação desta fila foi baseada na fila circular que foi feito na disciplina de Estrutura de Dados
    (professor Alexandre): por mais que as funções principais foram autoria de alunos, o "template" inteiro
    da fila (atributos, funções, etc) são dele.
*/
typedef struct {
    unsigned int* priorities;
    person* values;
    int head, tail, size, max_size;
} priority_array_queue;

/*
    Inicializa uma fila a partir de um valor iniciado nativamente (estaticamente ou dinamicamente),
    configurando seus atributos de acordo. As funções foram criadas de modo a parecer o máximo possível
    com as funções utilizadas de pthreads e semáforos, onde o ponteiro de uma variável é posto como
    argumento da função, já que a própria variável é incapaz de "ter" funções (isto é, métodos).

*/
void initialize(priority_array_queue* queue, int size) {
    (*queue).priorities = (int*) malloc(size * sizeof(int));
    (*queue).values = malloc(size * sizeof(person));
    (*queue).head = 0;
    (*queue).tail = -1;
    (*queue).size = 0;
    (*queue).max_size = size;
}

/*
    "Limpa" os valores da fila. Um free explicito dos vetores contidos nele é desnecessário, pois o que será
    inserido e retornado pela fila dependerá exclusivamente dos seus atributos. Isso também vale para o vetor
    de prioridades.
*/
void clear(priority_array_queue* queue) {
    (*queue).head = 0;
    (*queue).tail = -1;
    (*queue).size = 0;
}

/*
    Desaloca a fila da memória.
*/
void destroy(priority_array_queue* queue) {
    free((*queue).priorities);
    free((*queue).values);
    free(queue);
}

/*
    Função que insere na um valor na fila baseado em um valor de prioridade: se a chamada tiver
    o maior valor de prioridade até então, esse valor será inserido imediatamente no head, sendo
    assim o próximo a sair da fila. Não sei se é a solução mais justa, visto que se tiver um valor
    com prioridade 1 e, desde então, só for inserido valores com prioridade 2, o valor com prioridade
    1 nunca sairá da fila
*/
void enqueue(priority_array_queue* queue, person* value, unsigned int priority) {
    if (is_full(queue)) return;
    // substituir por um while que itera os valores até achar um valor de propriedade que seja
    // menor que a propriedade inserida, então inserir na frente deste valor. Um "push_front"
    // seria necessário (talvez uma lista seja mais indicada no fim das contas).

    (*queue).size++;
    if (priority <= *((*queue).priorities + (*queue).head) || (*queue).tail == -1) {
        (*queue).tail = ((*queue).tail + 1) % (*queue).max_size;
        *((*queue).priorities + (*queue).tail) = priority;
        *((*queue).values + (*queue).tail) = *value;
        return;
    }

    if (--(*queue).head < 0) (*queue).head = (*queue).max_size - 1;
    *((*queue).priorities + (*queue).head) = priority;
    *((*queue).values + (*queue).head) = *value;
}

/*
    Função que retira o próximo valor com maior prioridade da fila e retorna um ponteiro dele.
*/
person* dequeue(priority_array_queue* queue) {
    if (is_empty(queue)) return;
    person* temp = ((*queue).values + (*queue).head);
    *((*queue).priorities + (*queue).head) = 0;
    (*queue).head = ((*queue).head + 1) % (*queue).max_size;
    (*queue).size--;
    return temp;
}

/*
    Função que retorna um ponteiro para o primeiro valor da fila.
*/
person* back(priority_array_queue* queue) {
    return (*queue).values + (*queue).tail;
}

/*
    Função que retorna quantas posições da fila foram ocupadas.
*/
unsigned int size(priority_array_queue* queue) {
    return (*queue).size;
}

/*
    Função que retorna quantas posições da fila podem ser ocupadas.
*/
unsigned int max_size(priority_array_queue* queue) {
    return (*queue).max_size;
}

/*
    Função que retorna 1 se vazio, se não retorna 0.
*/
int is_empty(priority_array_queue* queue) {
    if ((*queue).size == 0) return 1;
    return 0;
}

/*
    Função que retorna 1 se cheio, se não retorna 0.
*/
int is_full(priority_array_queue* queue) {
    if ((*queue).size == (*queue).max_size) return 1;
    return 0;
}

/*
    Função main criada só para testar a fila.
*/
int main(void) {
    priority_array_queue fila;
    initialize(&fila, 8);
    person a, b, c;
    a.name = "Anderson";
    b.name = "Rochelle";
    c.name = "Joanna D'Arc";
    a.age = 15;
    b.age = 34;
    c.age = 800;

    enqueue(&fila, &a, 2);
    enqueue(&fila, &b, 2);
    enqueue(&fila, &c, 3);
    while (size(&fila) != 0) {
        person d = *back(&fila);
        printf("%s - %d\n", d.name, d.age);
        dequeue(&fila);
    }
    return 0;
}

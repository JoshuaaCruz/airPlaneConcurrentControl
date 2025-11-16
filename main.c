// CÓDIGO POTENCIALMENTE SENSÍVEL À ANTIVIRUS NÃO FAÇO ABSOLUTAMENTE A MENOR IDEIA!!!

/*
    A existência desse arquivo por ora é completamente desnecessária, criei por que
    achei que seria um "adiantamento".
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void does_the_thing(void* threads, void* sectors) {
    // the function that does the thing
    printf("(the thing)");
}

int main(int argc, char** argv) {
    unsigned int n_sectors = atoi(argv[1]), n_threads = atoi(argv[2]);
    if (n_sectors == 0 || n_threads == 0) return -1;

    // não sei o que ele poderia fazer aqui no meio ou se ele iria fazer direto o que
    // tem que fazer na função imediatamente embaixo

    does_the_thing(n_sectors, n_threads);
    return 0;
}
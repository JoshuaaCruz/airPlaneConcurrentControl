CC = gcc
CFLAGS = -Wall -Wextra -g -D_POSIX_C_SOURCE=199309L -D_DEFAULT_SOURCE

# Flags do Linker: recomendado para usar a biblioteca <pthread.h>
LDFLAGS = -pthread

# Nome do executável final
TARGET = traffic_control

SRCS = main.c priority_array_queue.c logger.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

.PHONY: submission compile clean #Diz que essas palavras não são arquivos mas "ações"

# Limpa os binários gerados
clean:
	rm -f $(TARGET) *.tar.gz

submission: clean
	@rm -fr build $(OUTPUT)
	@SUBNAME=$$(basename "$$(pwd)"); \
		echo "Empacotando diretório: $$SUBNAME"; \
		cd ..; \
		tar zcf "$$SUBNAME.tar.gz" --exclude='*.git' "$$SUBNAME"; \
		mv "$$SUBNAME.tar.gz" "$$SUBNAME/"
	@echo "Criado pacote para submissão: $$(basename "$$(pwd)").tar.gz"
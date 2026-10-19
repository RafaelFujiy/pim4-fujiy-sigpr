CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -Iinclude
MODULOS = src/entrada.c src/arquivos.c src/producao.c src/qualidade.c \
          src/rastreio.c src/relatorios.c

.PHONY: all teste memoria limpar

all: sigpr

sigpr: src/main.c $(MODULOS)
	$(CC) $(CFLAGS) -O2 -o $@ $^

teste: tests/teste_sigpr.c $(MODULOS)
	$(CC) $(CFLAGS) -o teste_sigpr $^
	./teste_sigpr

# Mesmos testes com verificacao de memoria (estouro de vetor, uso de
# ponteiro invalido e vazamento)
memoria: tests/teste_sigpr.c $(MODULOS)
	$(CC) $(CFLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer -o teste_memoria $^
	./teste_memoria

limpar:
	rm -f sigpr sigpr.exe teste_sigpr teste_memoria

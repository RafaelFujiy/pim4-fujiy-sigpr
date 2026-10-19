/*
 * entrada.h
 * Leitura validada do teclado, datas e validacao de CNPJ.
 */
#ifndef ENTRADA_H
#define ENTRADA_H

void   configurar_console(void);
void   remover_quebra(char *texto);
void   imprimir_coluna(const char *texto, int largura);

void   ler_texto(const char *pergunta, char *destino, int tamanho);
int    ler_inteiro(const char *pergunta, int minimo, int maximo);
double ler_real(const char *pergunta, double minimo, double maximo);
void   ler_data(const char *pergunta, char *destino);
int    confirmar(const char *pergunta);

int    data_valida(const char *data);
long   data_para_dias(const char *data);
void   dias_para_data(long dias, char *destino);
void   data_hoje(char *destino);
void   data_hora_agora(char *destino);
int    minutos_entre(const char *inicio, const char *fim);
void   recuar_minutos(const char *data_hora, int minutos, char *destino);

int    cnpj_valido(const char *cnpj);
void   somente_digitos(const char *origem, char *destino, int tamanho);

#endif

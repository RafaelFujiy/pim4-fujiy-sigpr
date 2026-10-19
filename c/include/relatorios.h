/*
 * relatorios.h
 * Indicadores para o PCP e a qualidade: gargalo, refugo por etapa e mes,
 * entregas no prazo e sequenciamento das OPs pela data de entrega.
 */
#ifndef RELATORIOS_H
#define RELATORIOS_H

#include "modelos.h"

int  montar_matriz_tempos(const BaseDados *bd, int minutos[][NUM_ETAPAS],
                          int pecas[][NUM_ETAPAS], int numeros_op[], int maximo);
int  calcular_gargalo(int minutos[][NUM_ETAPAS], int pecas[][NUM_ETAPAS],
                      int linhas, double min_por_peca[], int *etapa_gargalo);
void montar_matriz_refugo(const BaseDados *bd, int ano, int refugo[][12], int entradas[]);
int  calcular_otd(const BaseDados *bd, int *no_prazo, int *total, double *percentual);

void ordenar_por_entrega(const BaseDados *bd, int indices[], int n);
void simular_sequencia(const BaseDados *bd, const int indices[], int n, const char *inicio,
                       long conclusao[], int *atrasadas, long *dias_atraso, long *maior_atraso);

void listar_ops(const BaseDados *bd);
void relatorio_gargalo(const BaseDados *bd);
void relatorio_refugo(const BaseDados *bd, int ano);
void relatorio_otd(const BaseDados *bd);
void relatorio_sequenciamento(const BaseDados *bd, const char *hoje);

#endif

/*
 * qualidade.h
 * Inspecao final por amostragem (ABNT NBR 5426, nivel geral II, plano de
 * amostragem simples, inspecao normal, NQA 1,0) e especificacoes internas
 * de cada familia de produto.
 */
#ifndef QUALIDADE_H
#define QUALIDADE_H

#include "modelos.h"

#define NQA_PADRAO 1.0

typedef struct {
    char   familia[4];          /* CUR, SON, ESP, ESC, PIN, TES, PAG */
    char   caracteristica[32];
    char   unidade[8];
    double minimo;
    double maximo;
} Especificacao;

int  plano_amostragem(int tamanho_lote, char *letra, int *amostra,
                      int *aceitacao, int *rejeicao);
void familia_do_produto(const char *codigo, char *familia);
int  especificacoes_da_familia(const char *familia, const Especificacao *lista[], int maximo);
const Especificacao *buscar_especificacao(const char *familia, const char *caracteristica);
int  medicao_conforme(const Especificacao *esp, double valor);
int  registrar_inspecao(BaseDados *bd, Inspecao *insp, int minutos);

#endif

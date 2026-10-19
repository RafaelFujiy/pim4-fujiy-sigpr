/*
 * producao.h
 * Ordens de producao, apontamento das etapas e expedicao de lotes.
 */
#ifndef PRODUCAO_H
#define PRODUCAO_H

#include "modelos.h"

/* Codigos de retorno das operacoes */
#define OK                    0
#define ERRO_SITUACAO        -1
#define ERRO_SEQUENCIA       -2
#define ERRO_QUANTIDADE      -3
#define ERRO_MOTIVO          -4
#define ERRO_HABILITACAO     -5
#define ERRO_NAO_ENCONTRADO  -6
#define ERRO_SALDO           -7
#define ERRO_LIMITE          -8
#define ERRO_DATA            -9
#define ERRO_CNPJ           -10
#define ERRO_MATERIAL       -11

/* Capacidade diaria do recurso que limita a fabrica (dois turnos) */
#define MINUTOS_POR_DIA     960

const char *nome_etapa(int etapa);
const char *nome_situacao(int situacao);
int         situacao_por_nome(const char *nome);
double      tempo_padrao_peca(int etapa);
const char *mensagem_erro(int codigo);

int  buscar_produto(const BaseDados *bd, const char *codigo);
int  buscar_operador(const BaseDados *bd, const char *matricula);
int  buscar_lote_mp(const BaseDados *bd, const char *codigo);
int  buscar_op(const BaseDados *bd, int numero);
int  buscar_op_por_lote(const BaseDados *bd, const char *lote);

int  proximo_numero_op(const BaseDados *bd);
void gerar_codigo_lote(int numero, const char *data, char *destino);
int  abrir_op(BaseDados *bd, const char *produto, int quantidade,
              const char *data_entrega, const char *hoje);
int  registrar_consumo(BaseDados *bd, int numero_op, const char *lote_mp, double peso_kg);

int  validar_apontamento(const OrdemProducao *op, const Operador *operador, int etapa,
                         int qtd_boa, int qtd_refugo, const char *motivo);
int  aplicar_apontamento(BaseDados *bd, const Apontamento *ap);

int  quantidade_expedida(const BaseDados *bd, int numero_op);
int  registrar_expedicao(BaseDados *bd, const Expedicao *exp);

#endif

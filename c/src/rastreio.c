/*
 * rastreio.c
 * Consultas de rastreabilidade sobre os vetores carregados na memoria.
 */
#include <stdio.h>
#include <string.h>
#include "modelos.h"
#include "rastreio.h"
#include "producao.h"
#include "entrada.h"

/* Numeros das OPs que consumiram o lote de materia-prima. */
int ops_do_lote_mp(const BaseDados *bd, const char *lote_mp, int numeros_op[], int maximo)
{
    int i, j, total = 0, repetido;

    for (i = 0; i < bd->num_consumos; i++) {
        if (strcmp(bd->consumos[i].lote_mp, lote_mp) != 0) {
            continue;
        }
        repetido = 0;
        for (j = 0; j < total; j++) {
            if (numeros_op[j] == bd->consumos[i].numero_op) {
                repetido = 1;
                break;
            }
        }
        if (!repetido && total < maximo) {
            numeros_op[total] = bd->consumos[i].numero_op;
            total++;
        }
    }
    return total;
}

/* Clientes (CNPJ, sem repeticao) que receberam pecas feitas com o lote. */
int clientes_do_lote_mp(const BaseDados *bd, const char *lote_mp, char cnpjs[][15], int maximo)
{
    int ops[MAX_OPS];
    int num_ops, i, j, k, total = 0, repetido;
    const Expedicao *e;

    num_ops = ops_do_lote_mp(bd, lote_mp, ops, MAX_OPS);
    for (i = 0; i < num_ops; i++) {
        for (j = 0; j < bd->num_expedicoes; j++) {
            e = &bd->expedicoes[j];
            if (e->numero_op != ops[i]) {
                continue;
            }
            repetido = 0;
            for (k = 0; k < total; k++) {
                if (strcmp(cnpjs[k], e->cnpj_cliente) == 0) {
                    repetido = 1;
                    break;
                }
            }
            if (!repetido && total < maximo) {
                strcpy(cnpjs[total], e->cnpj_cliente);
                total++;
            }
        }
    }
    return total;
}

void rastrear_lote_produto(const BaseDados *bd, const char *lote)
{
    int ind_op, ind_prod, ind_lote, ind_oper, i;
    const OrdemProducao *op;
    const Inspecao *insp;

    ind_op = buscar_op_por_lote(bd, lote);
    if (ind_op < 0) {
        printf("Lote %s não encontrado.\n", lote);
        return;
    }
    op = &bd->ops[ind_op];
    ind_prod = buscar_produto(bd, op->produto);

    printf("\nLote %s | OP %d | %s - %s\n", op->lote, op->numero, op->produto,
           ind_prod >= 0 ? bd->produtos[ind_prod].descricao : "?");
    printf("Quantidade lançada: %d | Boas: %d | Refugo: %d | Situação: %s\n",
           op->quantidade, op->qtd_boa, op->qtd_refugo, nome_situacao(op->situacao));

    printf("\nMatéria-prima utilizada:\n");
    for (i = 0; i < bd->num_consumos; i++) {
        if (bd->consumos[i].numero_op != op->numero) {
            continue;
        }
        ind_lote = buscar_lote_mp(bd, bd->consumos[i].lote_mp);
        if (ind_lote >= 0) {
            printf("  %-12s %-8s corrida %-10s %7.2f kg  fornecedor: %s\n",
                   bd->lotes_mp[ind_lote].codigo, bd->lotes_mp[ind_lote].aco,
                   bd->lotes_mp[ind_lote].corrida, bd->consumos[i].peso_kg,
                   bd->lotes_mp[ind_lote].fornecedor);
        }
    }

    printf("\nEtapas:\n");
    for (i = 0; i < bd->num_apontamentos; i++) {
        if (bd->apontamentos[i].numero_op != op->numero) {
            continue;
        }
        ind_oper = buscar_operador(bd, bd->apontamentos[i].matricula);
        printf("  %d ", bd->apontamentos[i].etapa);
        imprimir_coluna(nome_etapa(bd->apontamentos[i].etapa), 27);
        printf("%s  ", bd->apontamentos[i].fim);
        imprimir_coluna(ind_oper >= 0 ? bd->operadores[ind_oper].nome
                                      : bd->apontamentos[i].matricula, 20);
        printf("boas %4d  refugo %3d  %s\n", bd->apontamentos[i].qtd_boa,
               bd->apontamentos[i].qtd_refugo, bd->apontamentos[i].motivo);
    }

    for (i = 0; i < bd->num_inspecoes; i++) {
        insp = &bd->inspecoes[i];
        if (insp->numero_op == op->numero) {
            printf("\nInspeção %s: lote %d, letra %c, amostra %d, Ac %d Re %d, defeitos %d -> %s\n",
                   insp->data, insp->tamanho_lote, insp->letra_codigo, insp->amostra,
                   insp->aceitacao, insp->rejeicao, insp->defeitos,
                   insp->resultado == 'A' ? "APROVADO" : "REPROVADO");
        }
    }

    printf("\nExpedições:\n");
    for (i = 0; i < bd->num_expedicoes; i++) {
        if (bd->expedicoes[i].numero_op == op->numero) {
            printf("  NF %d  %s  %s  %d peças\n", bd->expedicoes[i].nota_fiscal,
                   bd->expedicoes[i].data, bd->expedicoes[i].cliente,
                   bd->expedicoes[i].quantidade);
        }
    }
}

/* Lista de recall: todos os lotes feitos com a barra suspeita e os
   clientes que receberam as pecas. */
void rastrear_lote_mp(const BaseDados *bd, const char *lote_mp)
{
    int ops[MAX_OPS];
    int num_ops, i, j, ind_op, pecas_expedidas = 0;
    const OrdemProducao *op;

    if (buscar_lote_mp(bd, lote_mp) < 0) {
        printf("Lote de matéria-prima %s não encontrado.\n", lote_mp);
        return;
    }
    num_ops = ops_do_lote_mp(bd, lote_mp, ops, MAX_OPS);
    printf("\nO lote %s foi usado em %d OP(s):\n", lote_mp, num_ops);
    for (i = 0; i < num_ops; i++) {
        ind_op = buscar_op(bd, ops[i]);
        if (ind_op < 0) {
            continue;
        }
        op = &bd->ops[ind_op];
        printf("  OP %d  lote %s  %s  situação %s\n", op->numero, op->lote,
               op->produto, nome_situacao(op->situacao));
        for (j = 0; j < bd->num_expedicoes; j++) {
            if (bd->expedicoes[j].numero_op == op->numero) {
                printf("      -> NF %d  %s  %s (CNPJ %s)  %d peças\n",
                       bd->expedicoes[j].nota_fiscal, bd->expedicoes[j].data,
                       bd->expedicoes[j].cliente, bd->expedicoes[j].cnpj_cliente,
                       bd->expedicoes[j].quantidade);
                pecas_expedidas += bd->expedicoes[j].quantidade;
            }
        }
    }
    printf("Peças já expedidas a clientes: %d\n", pecas_expedidas);
}

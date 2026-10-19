/*
 * relatorios.c
 * Indicadores calculados a partir dos apontamentos. As matrizes usam a
 * etapa como coluna: a coluna j guarda a etapa j + 1.
 */
#include <stdio.h>
#include <stdlib.h>
#include "modelos.h"
#include "relatorios.h"
#include "producao.h"
#include "entrada.h"

/* Uma linha por OP que ja teve apontamento. minutos[l][e] e pecas[l][e]
   guardam o tempo gasto e as pecas processadas pela OP l na etapa e. */
int montar_matriz_tempos(const BaseDados *bd, int minutos[][NUM_ETAPAS],
                         int pecas[][NUM_ETAPAS], int numeros_op[], int maximo)
{
    int i, j, linha, linhas = 0;
    const Apontamento *a;

    for (i = 0; i < bd->num_apontamentos; i++) {
        a = &bd->apontamentos[i];
        linha = -1;
        for (j = 0; j < linhas; j++) {
            if (numeros_op[j] == a->numero_op) {
                linha = j;
                break;
            }
        }
        if (linha < 0) {
            if (linhas >= maximo) {
                continue;
            }
            linha = linhas;
            numeros_op[linha] = a->numero_op;
            for (j = 0; j < NUM_ETAPAS; j++) {
                minutos[linha][j] = 0;
                pecas[linha][j] = 0;
            }
            linhas++;
        }
        minutos[linha][a->etapa - 1] += a->minutos;
        pecas[linha][a->etapa - 1] += a->qtd_boa + a->qtd_refugo;
    }
    return linhas;
}

/* Gargalo = etapa com mais minutos por peca, ou seja, a de menor
   capacidade. Percorre a matriz coluna por coluna. */
int calcular_gargalo(int minutos[][NUM_ETAPAS], int pecas[][NUM_ETAPAS],
                     int linhas, double min_por_peca[], int *etapa_gargalo)
{
    long soma_min, soma_pecas;
    int l, e;

    *etapa_gargalo = 0;
    for (e = 0; e < NUM_ETAPAS; e++) {
        soma_min = 0;
        soma_pecas = 0;
        for (l = 0; l < linhas; l++) {
            soma_min += minutos[l][e];
            soma_pecas += pecas[l][e];
        }
        min_por_peca[e] = (soma_pecas > 0) ? (double) soma_min / soma_pecas : 0.0;
        if (min_por_peca[e] > min_por_peca[*etapa_gargalo]) {
            *etapa_gargalo = e;
        }
    }
    *etapa_gargalo = *etapa_gargalo + 1;
    return (linhas > 0) ? 0 : -1;
}

void montar_matriz_refugo(const BaseDados *bd, int ano, int refugo[][12], int entradas[])
{
    int i, e, m, mes;
    const Apontamento *a;

    for (e = 0; e < NUM_ETAPAS; e++) {
        entradas[e] = 0;
        for (m = 0; m < 12; m++) {
            refugo[e][m] = 0;
        }
    }
    for (i = 0; i < bd->num_apontamentos; i++) {
        a = &bd->apontamentos[i];
        if (atoi(a->fim) != ano) {
            continue;
        }
        mes = atoi(a->fim + 5);
        refugo[a->etapa - 1][mes - 1] += a->qtd_refugo;
        entradas[a->etapa - 1] += a->qtd_boa + a->qtd_refugo;
    }
}

/* OTD (on-time delivery): OPs totalmente expedidas ate a data prometida. */
int calcular_otd(const BaseDados *bd, int *no_prazo, int *total, double *percentual)
{
    int i, j;
    long ultima, entrega;
    const OrdemProducao *op;

    *no_prazo = 0;
    *total = 0;
    for (i = 0; i < bd->num_ops; i++) {
        op = &bd->ops[i];
        if (op->situacao != OP_EXPEDIDA) {
            continue;
        }
        ultima = 0;
        for (j = 0; j < bd->num_expedicoes; j++) {
            if (bd->expedicoes[j].numero_op == op->numero &&
                data_para_dias(bd->expedicoes[j].data) > ultima) {
                ultima = data_para_dias(bd->expedicoes[j].data);
            }
        }
        entrega = data_para_dias(op->data_entrega);
        if (ultima <= entrega) {
            (*no_prazo)++;
        }
        (*total)++;
    }
    *percentual = (*total > 0) ? 100.0 * *no_prazo / *total : 0.0;
    return *total;
}

/* Ordenacao por insercao do vetor de indices pela data de entrega
   (regra EDD). Em caso de empate, a OP mais antiga vem primeiro.
   Custo O(n^2) no pior caso; com algumas dezenas de OPs abertas e
   imediato, e o algoritmo e estavel. */
void ordenar_por_entrega(const BaseDados *bd, int indices[], int n)
{
    int i, j, atual;
    long chave;

    for (i = 1; i < n; i++) {
        atual = indices[i];
        chave = data_para_dias(bd->ops[atual].data_entrega);
        j = i - 1;
        while (j >= 0 && (data_para_dias(bd->ops[indices[j]].data_entrega) > chave ||
                          (data_para_dias(bd->ops[indices[j]].data_entrega) == chave &&
                           bd->ops[indices[j]].numero > bd->ops[atual].numero))) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = atual;
    }
}

/* Simplificacao: as OPs passam pela fabrica uma depois da outra, com
   MINUTOS_POR_DIA minutos-padrao de capacidade por dia corrido. */
void simular_sequencia(const BaseDados *bd, const int indices[], int n, const char *inicio,
                       long conclusao[], int *atrasadas, long *dias_atraso, long *maior_atraso)
{
    long dia_inicio = data_para_dias(inicio);
    double acumulado = 0.0;
    long fim, atraso;
    int i, e;
    const OrdemProducao *op;

    *atrasadas = 0;
    *dias_atraso = 0;
    *maior_atraso = 0;
    for (i = 0; i < n; i++) {
        op = &bd->ops[indices[i]];
        for (e = op->etapa_atual + 1; e <= NUM_ETAPAS; e++) {
            acumulado += tempo_padrao_peca(e) * op->qtd_boa;
        }
        fim = dia_inicio + (long) ((acumulado + MINUTOS_POR_DIA - 1) / MINUTOS_POR_DIA);
        if (conclusao != NULL) {
            conclusao[i] = fim;
        }
        atraso = fim - data_para_dias(op->data_entrega);
        if (atraso > 0) {
            (*atrasadas)++;
            *dias_atraso += atraso;
            if (atraso > *maior_atraso) {
                *maior_atraso = atraso;
            }
        }
    }
}

/* ---------- Relatorios impressos ---------- */

void listar_ops(const BaseDados *bd)
{
    int i;
    const OrdemProducao *op;

    printf("\n  OP  Lote        Produto      Qtd  Boas  Entrega     Situação     Etapa\n");
    for (i = 0; i < bd->num_ops; i++) {
        op = &bd->ops[i];
        printf("%4d  %-10s  %-10s %5d %5d  %s  %-12s %d/%d\n", op->numero, op->lote,
               op->produto, op->quantidade, op->qtd_boa, op->data_entrega,
               nome_situacao(op->situacao), op->etapa_atual, NUM_ETAPAS);
    }
}

void relatorio_gargalo(const BaseDados *bd)
{
    static int minutos[MAX_OPS][NUM_ETAPAS];
    static int pecas[MAX_OPS][NUM_ETAPAS];
    int numeros_op[MAX_OPS];
    double min_por_peca[NUM_ETAPAS];
    int linhas, gargalo, l, e, primeira;

    linhas = montar_matriz_tempos(bd, minutos, pecas, numeros_op, MAX_OPS);
    if (calcular_gargalo(minutos, pecas, linhas, min_por_peca, &gargalo) != 0) {
        printf("Sem apontamentos para analisar.\n");
        return;
    }

    primeira = (linhas > 8) ? linhas - 8 : 0;
    printf("\nMatriz de tempos (minutos por OP e etapa) - últimas %d OPs\n", linhas - primeira);
    printf("   OP");
    for (e = 1; e <= NUM_ETAPAS; e++) {
        printf("    E%d", e);
    }
    printf("\n");
    for (l = primeira; l < linhas; l++) {
        printf("%5d", numeros_op[l]);
        for (e = 0; e < NUM_ETAPAS; e++) {
            printf(" %5d", minutos[l][e]);
        }
        printf("\n");
    }

    printf("\nEtapa                          min/peça   padrão   capacidade (peças/dia)\n");
    for (e = 0; e < NUM_ETAPAS; e++) {
        printf("%d ", e + 1);
        imprimir_coluna(nome_etapa(e + 1), 28);
        if (min_por_peca[e] > 0.0) {
            printf("%8.2f %8.2f %12.0f%s\n", min_por_peca[e], tempo_padrao_peca(e + 1),
                   MINUTOS_POR_DIA / min_por_peca[e], (e + 1 == gargalo) ? "   <- gargalo" : "");
        } else {
            printf("%8s %8.2f %12s\n", "-", tempo_padrao_peca(e + 1), "-");
        }
    }
    printf("\nGargalo: etapa %d (%s), %.2f min por peça, %d OPs analisadas.\n", gargalo,
           nome_etapa(gargalo), min_por_peca[gargalo - 1], linhas);
}

void relatorio_refugo(const BaseDados *bd, int ano)
{
    int refugo[NUM_ETAPAS][12];
    int entradas[NUM_ETAPAS];
    int total_mes[12];
    int e, m, total_etapa, total_geral = 0, mes_ini = 12, mes_fim = -1;

    montar_matriz_refugo(bd, ano, refugo, entradas);
    for (m = 0; m < 12; m++) {
        total_mes[m] = 0;
        for (e = 0; e < NUM_ETAPAS; e++) {
            total_mes[m] += refugo[e][m];
        }
    }
    /* mostra apenas os meses que tiveram producao */
    for (e = 0; e < bd->num_apontamentos; e++) {
        if (atoi(bd->apontamentos[e].fim) == ano) {
            m = atoi(bd->apontamentos[e].fim + 5) - 1;
            if (m < mes_ini) {
                mes_ini = m;
            }
            if (m > mes_fim) {
                mes_fim = m;
            }
        }
    }
    if (mes_fim < 0) {
        printf("Sem apontamentos em %d.\n", ano);
        return;
    }

    printf("\nRefugo (peças) por etapa e mês - %d\n", ano);
    printf("Etapa ");
    for (m = mes_ini; m <= mes_fim; m++) {
        printf("  %02d/%d", m + 1, ano % 100);
    }
    printf("   Total  %% refugo\n");
    for (e = 0; e < NUM_ETAPAS; e++) {
        total_etapa = 0;
        printf("  E%d  ", e + 1);
        for (m = mes_ini; m <= mes_fim; m++) {
            printf(" %6d", refugo[e][m]);
            total_etapa += refugo[e][m];
        }
        total_geral += total_etapa;
        printf("  %6d  %7.2f%%\n", total_etapa,
               entradas[e] > 0 ? 100.0 * total_etapa / entradas[e] : 0.0);
    }
    printf("Total ");
    for (m = mes_ini; m <= mes_fim; m++) {
        printf(" %6d", total_mes[m]);
    }
    printf("  %6d\n", total_geral);
}

void relatorio_otd(const BaseDados *bd)
{
    int no_prazo, total;
    double percentual;

    if (calcular_otd(bd, &no_prazo, &total, &percentual) == 0) {
        printf("Nenhuma OP expedida ainda.\n");
        return;
    }
    printf("\nEntregas no prazo (OTD): %d de %d OPs expedidas = %.1f%%\n",
           no_prazo, total, percentual);
}

void relatorio_sequenciamento(const BaseDados *bd, const char *hoje)
{
    int fifo[MAX_OPS], edd[MAX_OPS];
    long conclusao[MAX_OPS];
    int n = 0, i, atrasadas_fifo, atrasadas_edd;
    long dias_fifo, dias_edd, maior_fifo, maior_edd, atraso;
    char data[11];
    const OrdemProducao *op;

    for (i = 0; i < bd->num_ops; i++) {
        if (bd->ops[i].situacao == OP_ABERTA || bd->ops[i].situacao == OP_EM_PRODUCAO ||
            bd->ops[i].situacao == OP_EM_INSPECAO) {
            fifo[n] = i;
            edd[n] = i;
            n++;
        }
    }
    if (n == 0) {
        printf("Nenhuma OP pendente.\n");
        return;
    }

    simular_sequencia(bd, fifo, n, hoje, NULL, &atrasadas_fifo, &dias_fifo, &maior_fifo);
    ordenar_por_entrega(bd, edd, n);
    simular_sequencia(bd, edd, n, hoje, conclusao, &atrasadas_edd, &dias_edd, &maior_edd);

    printf("\nSequência sugerida (menor data de entrega primeiro), início em %s\n", hoje);
    printf("Ord   OP  Produto      Peças  Etapa  Entrega     Conclusão   Atraso\n");
    for (i = 0; i < n; i++) {
        op = &bd->ops[edd[i]];
        dias_para_data(conclusao[i], data);
        atraso = conclusao[i] - data_para_dias(op->data_entrega);
        printf("%3d %4d  %-11s %6d   %d/%d   %s  %s  %s", i + 1, op->numero, op->produto,
               op->qtd_boa, op->etapa_atual, NUM_ETAPAS, op->data_entrega, data,
               atraso > 0 ? "" : "    -\n");
        if (atraso > 0) {
            printf("%3ld dias\n", atraso);
        }
    }
    printf("\nComparação           OPs atrasadas   dias de atraso   maior atraso\n");
    printf("Ordem de abertura    %13d   %14ld   %12ld\n", atrasadas_fifo, dias_fifo, maior_fifo);
    printf("Data de entrega      %13d   %14ld   %12ld\n", atrasadas_edd, dias_edd, maior_edd);
}

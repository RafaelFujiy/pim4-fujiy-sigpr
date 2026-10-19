/*
 * arquivos.c
 * Os dados ficam em arquivos texto CSV com ';' como separador e uma
 * linha de cabecalho. Na gravacao o arquivo e escrito primeiro em .tmp
 * e so depois substitui o original, para que uma queda de energia no
 * meio da gravacao nao apague os dados.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modelos.h"
#include "arquivos.h"
#include "entrada.h"
#include "producao.h"
#include "qualidade.h"

#define TAM_CAMINHO 260

/* Copia o campo atual para destino e avanca o cursor ate o proximo.
   Ao chegar no ultimo campo o cursor passa a valer NULL. */
int proximo_campo(char **cursor, char *destino, int tamanho)
{
    char *p = *cursor;
    int i = 0;

    if (p == NULL) {
        destino[0] = '\0';
        return 0;
    }
    while (*p != ';' && *p != '\0') {
        if (i < tamanho - 1) {
            destino[i] = *p;
            i++;
        }
        p++;
    }
    destino[i] = '\0';
    *cursor = (*p == ';') ? p + 1 : NULL;
    return 1;
}

static void montar_caminho(char *destino, const char *pasta, const char *nome)
{
    sprintf(destino, "%s/%s", pasta, nome);
}

/* Abre o arquivo e descarta a linha de cabecalho. */
static FILE *abrir_leitura(const char *pasta, const char *nome)
{
    char caminho[TAM_CAMINHO];
    char cabecalho[TAM_LINHA];
    FILE *arq;

    montar_caminho(caminho, pasta, nome);
    arq = fopen(caminho, "r");
    if (arq != NULL && fgets(cabecalho, TAM_LINHA, arq) == NULL) {
        fclose(arq);
        return NULL;
    }
    return arq;
}

static FILE *abrir_gravacao(const char *pasta, const char *nome, const char *cabecalho)
{
    char caminho[TAM_CAMINHO];
    FILE *arq;

    montar_caminho(caminho, pasta, nome);
    strcat(caminho, ".tmp");
    arq = fopen(caminho, "w");
    if (arq == NULL) {
        perror(caminho);
        return NULL;
    }
    fprintf(arq, "%s\n", cabecalho);
    return arq;
}

static int concluir_gravacao(FILE *arq, const char *pasta, const char *nome)
{
    char final[TAM_CAMINHO];
    char temporario[TAM_CAMINHO];

    if (fclose(arq) != 0) {
        return -1;
    }
    montar_caminho(final, pasta, nome);
    strcpy(temporario, final);
    strcat(temporario, ".tmp");
    remove(final);              /* no Windows rename falha se o destino existe */
    if (rename(temporario, final) != 0) {
        perror(final);
        return -1;
    }
    return 0;
}

static int carregar_produtos(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Produto *p;
    FILE *arq = abrir_leitura(pasta, "produtos.csv");

    bd->num_produtos = 0;
    if (arq == NULL) {
        return -1;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_produtos < MAX_PRODUTOS) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        p = &bd->produtos[bd->num_produtos];
        cursor = linha;
        proximo_campo(&cursor, p->codigo, sizeof(p->codigo));
        proximo_campo(&cursor, p->descricao, sizeof(p->descricao));
        proximo_campo(&cursor, p->linha, sizeof(p->linha));
        proximo_campo(&cursor, p->aco, sizeof(p->aco));
        proximo_campo(&cursor, campo, sizeof(campo));
        p->classe_risco = atoi(campo);
        bd->num_produtos++;
    }
    fclose(arq);
    return bd->num_produtos;
}

static int carregar_operadores(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Operador *o;
    int i;
    FILE *arq = abrir_leitura(pasta, "operadores.csv");

    bd->num_operadores = 0;
    if (arq == NULL) {
        return -1;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_operadores < MAX_OPERADORES) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        o = &bd->operadores[bd->num_operadores];
        cursor = linha;
        proximo_campo(&cursor, o->matricula, sizeof(o->matricula));
        proximo_campo(&cursor, o->nome, sizeof(o->nome));
        proximo_campo(&cursor, campo, sizeof(campo));
        /* habilitacoes vem como texto de 0 e 1, um caractere por etapa */
        for (i = 0; i < NUM_ETAPAS; i++) {
            o->habilitado[i] = (i < (int) strlen(campo) && campo[i] == '1') ? 1 : 0;
        }
        bd->num_operadores++;
    }
    fclose(arq);
    return bd->num_operadores;
}

static int carregar_lotes_mp(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    LoteMP *l;
    FILE *arq = abrir_leitura(pasta, "lotes_mp.csv");

    bd->num_lotes_mp = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_lotes_mp < MAX_LOTES_MP) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        l = &bd->lotes_mp[bd->num_lotes_mp];
        cursor = linha;
        proximo_campo(&cursor, l->codigo, sizeof(l->codigo));
        proximo_campo(&cursor, l->fornecedor, sizeof(l->fornecedor));
        proximo_campo(&cursor, l->aco, sizeof(l->aco));
        proximo_campo(&cursor, l->corrida, sizeof(l->corrida));
        proximo_campo(&cursor, campo, sizeof(campo));
        l->peso_kg = atof(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        l->saldo_kg = atof(campo);
        proximo_campo(&cursor, l->data_recebimento, sizeof(l->data_recebimento));
        bd->num_lotes_mp++;
    }
    fclose(arq);
    return bd->num_lotes_mp;
}

static int carregar_ops(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    OrdemProducao *op;
    FILE *arq = abrir_leitura(pasta, "ops.csv");

    bd->num_ops = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_ops < MAX_OPS) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        op = &bd->ops[bd->num_ops];
        cursor = linha;
        proximo_campo(&cursor, campo, sizeof(campo));
        op->numero = atoi(campo);
        proximo_campo(&cursor, op->produto, sizeof(op->produto));
        proximo_campo(&cursor, campo, sizeof(campo));
        op->quantidade = atoi(campo);
        proximo_campo(&cursor, op->data_abertura, sizeof(op->data_abertura));
        proximo_campo(&cursor, op->data_entrega, sizeof(op->data_entrega));
        proximo_campo(&cursor, campo, sizeof(campo));
        op->situacao = situacao_por_nome(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        op->etapa_atual = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        op->qtd_boa = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        op->qtd_refugo = atoi(campo);
        proximo_campo(&cursor, op->lote, sizeof(op->lote));
        bd->num_ops++;
    }
    fclose(arq);
    return bd->num_ops;
}

static int carregar_consumos(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Consumo *c;
    FILE *arq = abrir_leitura(pasta, "consumos.csv");

    bd->num_consumos = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_consumos < MAX_CONSUMOS) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        c = &bd->consumos[bd->num_consumos];
        cursor = linha;
        proximo_campo(&cursor, campo, sizeof(campo));
        c->numero_op = atoi(campo);
        proximo_campo(&cursor, c->lote_mp, sizeof(c->lote_mp));
        proximo_campo(&cursor, campo, sizeof(campo));
        c->peso_kg = atof(campo);
        bd->num_consumos++;
    }
    fclose(arq);
    return bd->num_consumos;
}

static int carregar_apontamentos(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Apontamento *a;
    FILE *arq = abrir_leitura(pasta, "apontamentos.csv");

    bd->num_apontamentos = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_apontamentos < MAX_APONTAMENTOS) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        a = &bd->apontamentos[bd->num_apontamentos];
        cursor = linha;
        proximo_campo(&cursor, campo, sizeof(campo));
        a->numero_op = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        a->etapa = atoi(campo);
        proximo_campo(&cursor, a->matricula, sizeof(a->matricula));
        proximo_campo(&cursor, a->inicio, sizeof(a->inicio));
        proximo_campo(&cursor, a->fim, sizeof(a->fim));
        proximo_campo(&cursor, campo, sizeof(campo));
        a->minutos = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        a->qtd_boa = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        a->qtd_refugo = atoi(campo);
        proximo_campo(&cursor, a->motivo, sizeof(a->motivo));
        bd->num_apontamentos++;
    }
    fclose(arq);
    return bd->num_apontamentos;
}

/* Medicoes gravadas como "nome=valor|nome=valor" */
static void ler_medicoes(char *texto, Inspecao *insp)
{
    char *p = texto;
    char *separador;
    char *igual;
    int i;

    insp->num_caract = 0;
    while (p != NULL && *p != '\0' && insp->num_caract < MAX_CARACT) {
        separador = strchr(p, '|');
        if (separador != NULL) {
            *separador = '\0';
        }
        igual = strchr(p, '=');
        if (igual != NULL) {
            *igual = '\0';
            for (i = 0; p[i] != '\0' && i < (int) sizeof(insp->caract[0]) - 1; i++) {
                insp->caract[insp->num_caract][i] = p[i];
            }
            insp->caract[insp->num_caract][i] = '\0';
            insp->valor[insp->num_caract] = atof(igual + 1);
            insp->num_caract++;
        }
        p = (separador != NULL) ? separador + 1 : NULL;
    }
}

static int carregar_inspecoes(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Inspecao *insp;
    FILE *arq = abrir_leitura(pasta, "inspecoes.csv");

    bd->num_inspecoes = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_inspecoes < MAX_INSPECOES) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        insp = &bd->inspecoes[bd->num_inspecoes];
        cursor = linha;
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->numero_op = atoi(campo);
        proximo_campo(&cursor, insp->data, sizeof(insp->data));
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->tamanho_lote = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->letra_codigo = campo[0];
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->amostra = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->aceitacao = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->rejeicao = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->defeitos = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        insp->resultado = campo[0];
        proximo_campo(&cursor, insp->inspetor, sizeof(insp->inspetor));
        proximo_campo(&cursor, campo, sizeof(campo));
        ler_medicoes(campo, insp);
        bd->num_inspecoes++;
    }
    fclose(arq);
    return bd->num_inspecoes;
}

static int carregar_expedicoes(BaseDados *bd, const char *pasta)
{
    char linha[TAM_LINHA], campo[TAM_LINHA];
    char *cursor;
    Expedicao *e;
    FILE *arq = abrir_leitura(pasta, "expedicoes.csv");

    bd->num_expedicoes = 0;
    if (arq == NULL) {
        return 0;
    }
    while (fgets(linha, TAM_LINHA, arq) != NULL && bd->num_expedicoes < MAX_EXPEDICOES) {
        remover_quebra(linha);
        if (linha[0] == '\0') {
            continue;
        }
        e = &bd->expedicoes[bd->num_expedicoes];
        cursor = linha;
        proximo_campo(&cursor, campo, sizeof(campo));
        e->nota_fiscal = atoi(campo);
        proximo_campo(&cursor, e->data, sizeof(e->data));
        proximo_campo(&cursor, e->cnpj_cliente, sizeof(e->cnpj_cliente));
        proximo_campo(&cursor, e->cliente, sizeof(e->cliente));
        proximo_campo(&cursor, campo, sizeof(campo));
        e->numero_op = atoi(campo);
        proximo_campo(&cursor, campo, sizeof(campo));
        e->quantidade = atoi(campo);
        bd->num_expedicoes++;
    }
    fclose(arq);
    return bd->num_expedicoes;
}

/* Produtos e operadores sao obrigatorios; os demais podem nao existir
   na primeira execucao. */
int carregar_base(BaseDados *bd, const char *pasta)
{
    if (carregar_produtos(bd, pasta) <= 0) {
        printf("Não foi possível ler %s/produtos.csv\n", pasta);
        return -1;
    }
    if (carregar_operadores(bd, pasta) <= 0) {
        printf("Não foi possível ler %s/operadores.csv\n", pasta);
        return -1;
    }
    carregar_lotes_mp(bd, pasta);
    carregar_ops(bd, pasta);
    carregar_consumos(bd, pasta);
    carregar_apontamentos(bd, pasta);
    carregar_inspecoes(bd, pasta);
    carregar_expedicoes(bd, pasta);
    return 0;
}

static int gravar_lotes_mp(const BaseDados *bd, const char *pasta, const char *nome)
{
    const LoteMP *l;
    int i;
    FILE *arq = abrir_gravacao(pasta, nome,
                               "codigo;fornecedor;aco;corrida;peso_kg;saldo_kg;data_recebimento");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_lotes_mp; i++) {
        l = &bd->lotes_mp[i];
        fprintf(arq, "%s;%s;%s;%s;%.2f;%.2f;%s\n", l->codigo, l->fornecedor, l->aco,
                l->corrida, l->peso_kg, l->saldo_kg, l->data_recebimento);
    }
    return concluir_gravacao(arq, pasta, nome);
}

int salvar_lotes_mp(const BaseDados *bd, const char *pasta)
{
    return gravar_lotes_mp(bd, pasta, "lotes_mp.csv");
}

static int gravar_ops(const BaseDados *bd, const char *pasta, const char *nome)
{
    const OrdemProducao *op;
    int i;
    FILE *arq = abrir_gravacao(pasta, nome,
        "numero;produto;quantidade;data_abertura;data_entrega;situacao;etapa_atual;qtd_boa;qtd_refugo;lote");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_ops; i++) {
        op = &bd->ops[i];
        fprintf(arq, "%d;%s;%d;%s;%s;%s;%d;%d;%d;%s\n", op->numero, op->produto,
                op->quantidade, op->data_abertura, op->data_entrega,
                nome_situacao(op->situacao), op->etapa_atual, op->qtd_boa,
                op->qtd_refugo, op->lote);
    }
    return concluir_gravacao(arq, pasta, nome);
}

int salvar_ops(const BaseDados *bd, const char *pasta)
{
    return gravar_ops(bd, pasta, "ops.csv");
}

static int gravar_consumos(const BaseDados *bd, const char *pasta, const char *nome)
{
    int i;
    FILE *arq = abrir_gravacao(pasta, nome, "numero_op;lote_mp;peso_kg");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_consumos; i++) {
        fprintf(arq, "%d;%s;%.2f\n", bd->consumos[i].numero_op,
                bd->consumos[i].lote_mp, bd->consumos[i].peso_kg);
    }
    return concluir_gravacao(arq, pasta, nome);
}

int salvar_consumos(const BaseDados *bd, const char *pasta)
{
    return gravar_consumos(bd, pasta, "consumos.csv");
}

static int gravar_apontamentos(const BaseDados *bd, const char *pasta, const char *nome)
{
    const Apontamento *a;
    int i;
    FILE *arq = abrir_gravacao(pasta, nome,
        "numero_op;etapa;matricula;inicio;fim;minutos;qtd_boa;qtd_refugo;motivo");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_apontamentos; i++) {
        a = &bd->apontamentos[i];
        fprintf(arq, "%d;%d;%s;%s;%s;%d;%d;%d;%s\n", a->numero_op, a->etapa,
                a->matricula, a->inicio, a->fim, a->minutos, a->qtd_boa,
                a->qtd_refugo, a->motivo);
    }
    return concluir_gravacao(arq, pasta, nome);
}

int salvar_apontamentos(const BaseDados *bd, const char *pasta)
{
    return gravar_apontamentos(bd, pasta, "apontamentos.csv");
}

int salvar_inspecoes(const BaseDados *bd, const char *pasta)
{
    const Inspecao *insp;
    int i, j;
    FILE *arq = abrir_gravacao(pasta, "inspecoes.csv",
        "numero_op;data;tamanho_lote;letra;amostra;ac;re;defeitos;resultado;inspetor;medicoes");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_inspecoes; i++) {
        insp = &bd->inspecoes[i];
        fprintf(arq, "%d;%s;%d;%c;%d;%d;%d;%d;%c;%s;", insp->numero_op, insp->data,
                insp->tamanho_lote, insp->letra_codigo, insp->amostra, insp->aceitacao,
                insp->rejeicao, insp->defeitos, insp->resultado, insp->inspetor);
        for (j = 0; j < insp->num_caract; j++) {
            fprintf(arq, "%s%s=%.2f", j > 0 ? "|" : "", insp->caract[j], insp->valor[j]);
        }
        fprintf(arq, "\n");
    }
    return concluir_gravacao(arq, pasta, "inspecoes.csv");
}

static int gravar_expedicoes(const BaseDados *bd, const char *pasta, const char *nome)
{
    const Expedicao *e;
    int i;
    FILE *arq = abrir_gravacao(pasta, nome,
                               "nota_fiscal;data;cnpj_cliente;cliente;numero_op;quantidade");

    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_expedicoes; i++) {
        e = &bd->expedicoes[i];
        fprintf(arq, "%d;%s;%s;%s;%d;%d\n", e->nota_fiscal, e->data, e->cnpj_cliente,
                e->cliente, e->numero_op, e->quantidade);
    }
    return concluir_gravacao(arq, pasta, nome);
}

int salvar_expedicoes(const BaseDados *bd, const char *pasta)
{
    return gravar_expedicoes(bd, pasta, "expedicoes.csv");
}

/* Cada operacao que altera dados deixa uma linha no log (modo "a"
   acrescenta ao final sem apagar o que ja existe). */
void registrar_auditoria(const char *pasta, const char *evento, const char *detalhe)
{
    char caminho[TAM_CAMINHO];
    char agora[17];
    FILE *arq;

    montar_caminho(caminho, pasta, "auditoria.log");
    arq = fopen(caminho, "a");
    if (arq == NULL) {
        perror(caminho);
        return;
    }
    data_hora_agora(agora);
    fprintf(arq, "%s;%s;%s\n", agora, evento, detalhe);
    fclose(arq);
}

/* ---------- Exportacao ---------- */

/* Gera um arquivo por tabela de destino, no formato lido pelo BULK INSERT
   do script database/sqlserver/05_carga_terminal.sql. */
int exportar_sqlserver(const BaseDados *bd, const char *pasta_saida)
{
    const Inspecao *insp;
    FILE *arq;
    int i;

    if (gravar_lotes_mp(bd, pasta_saida, "carga_lotes_mp.csv") != 0 ||
        gravar_ops(bd, pasta_saida, "carga_ops.csv") != 0 ||
        gravar_consumos(bd, pasta_saida, "carga_consumos.csv") != 0 ||
        gravar_apontamentos(bd, pasta_saida, "carga_apontamentos.csv") != 0 ||
        gravar_expedicoes(bd, pasta_saida, "carga_expedicoes.csv") != 0) {
        return -1;
    }

    /* a inspecao vai sem as medicoes, que seguem para o MongoDB */
    arq = abrir_gravacao(pasta_saida, "carga_inspecoes.csv",
        "numero_op;data;tamanho_lote;letra;amostra;ac;re;defeitos;resultado;inspetor");
    if (arq == NULL) {
        return -1;
    }
    for (i = 0; i < bd->num_inspecoes; i++) {
        insp = &bd->inspecoes[i];
        fprintf(arq, "%d;%s;%d;%c;%d;%d;%d;%d;%s;%s\n", insp->numero_op, insp->data,
                insp->tamanho_lote, insp->letra_codigo, insp->amostra, insp->aceitacao,
                insp->rejeicao, insp->defeitos,
                insp->resultado == 'A' ? "APROVADO" : "REPROVADO", insp->inspetor);
    }
    return concluir_gravacao(arq, pasta_saida, "carga_inspecoes.csv");
}

static void escapar_json(const char *origem, char *destino, int tamanho)
{
    int i, j = 0;

    for (i = 0; origem[i] != '\0' && j < tamanho - 2; i++) {
        if (origem[i] == '"' || origem[i] == '\\') {
            destino[j++] = '\\';
        }
        if ((unsigned char) origem[i] >= 32) {
            destino[j++] = origem[i];
        }
    }
    destino[j] = '\0';
}

/* Um laudo por linha (JSON Lines), formato aceito pelo mongoimport. */
int exportar_mongodb(const BaseDados *bd, const char *pasta_saida)
{
    char caminho[TAM_CAMINHO];
    char familia[4];
    char texto[120];
    const Inspecao *insp;
    const OrdemProducao *op;
    const Produto *prod;
    const Especificacao *esp;
    FILE *arq;
    int i, j, ind_op, ind_prod;

    montar_caminho(caminho, pasta_saida, "laudos.jsonl");
    arq = fopen(caminho, "w");
    if (arq == NULL) {
        perror(caminho);
        return -1;
    }
    for (i = 0; i < bd->num_inspecoes; i++) {
        insp = &bd->inspecoes[i];
        ind_op = buscar_op(bd, insp->numero_op);
        if (ind_op < 0) {
            continue;
        }
        op = &bd->ops[ind_op];
        ind_prod = buscar_produto(bd, op->produto);
        if (ind_prod < 0) {
            continue;
        }
        prod = &bd->produtos[ind_prod];
        familia_do_produto(prod->codigo, familia);
        escapar_json(prod->descricao, texto, sizeof(texto));

        fprintf(arq, "{\"lote\":\"%s\",\"numero_op\":%d,", op->lote, op->numero);
        fprintf(arq, "\"produto\":{\"codigo\":\"%s\",\"descricao\":\"%s\",\"familia\":\"%s\",\"linha\":\"%s\"},",
                prod->codigo, texto, familia, prod->linha);
        /* data no formato Extended JSON: o mongoimport grava como tipo Date */
        fprintf(arq, "\"data\":{\"$date\":\"%sT00:00:00Z\"},\"inspetor\":\"%s\",",
                insp->data, insp->inspetor);
        fprintf(arq, "\"plano\":{\"norma\":\"ABNT NBR 5426\",\"nivel\":\"II\",\"nqa\":%.1f,"
                     "\"letra\":\"%c\",\"tamanho_lote\":%d,\"amostra\":%d,\"ac\":%d,\"re\":%d},",
                NQA_PADRAO, insp->letra_codigo, insp->tamanho_lote, insp->amostra,
                insp->aceitacao, insp->rejeicao);
        fprintf(arq, "\"defeitos\":%d,\"resultado\":\"%s\",\"medicoes\":[", insp->defeitos,
                insp->resultado == 'A' ? "APROVADO" : "REPROVADO");
        for (j = 0; j < insp->num_caract; j++) {
            esp = buscar_especificacao(familia, insp->caract[j]);
            fprintf(arq, "%s{\"caracteristica\":\"%s\",\"valor\":%.2f", j > 0 ? "," : "",
                    insp->caract[j], insp->valor[j]);
            if (esp != NULL) {
                fprintf(arq, ",\"unidade\":\"%s\",\"minimo\":%.2f,\"maximo\":%.2f,\"conforme\":%s",
                        esp->unidade, esp->minimo, esp->maximo,
                        medicao_conforme(esp, insp->valor[j]) ? "true" : "false");
            }
            fprintf(arq, "}");
        }
        fprintf(arq, "]}\n");
    }
    fclose(arq);
    return 0;
}

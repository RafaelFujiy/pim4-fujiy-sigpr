/*
 * qualidade.c
 * Inspecao final por amostragem segundo a ABNT NBR 5426: nivel geral de
 * inspecao II, plano de amostragem simples, inspecao normal, NQA 1,0.
 */
#include <stdio.h>
#include <string.h>
#include "modelos.h"
#include "qualidade.h"
#include "producao.h"
#include "entrada.h"

#define NUM_FAIXAS 15

/* Tabela de letras-codigo (nivel II): limite superior de cada faixa de
   tamanho de lote. A ultima faixa (letra Q) nao tem limite. */
static const int  LIMITE_FAIXA[NUM_FAIXAS - 1] = {
    8, 15, 25, 50, 90, 150, 280, 500, 1200, 3200, 10000, 35000, 150000, 500000
};
static const char LETRAS[NUM_FAIXAS + 1] = "ABCDEFGHJKLMNPQ";

/* Plano efetivo para NQA 1,0 com as setas da tabela ja resolvidas:
   de A a F a seta leva ao plano n = 13 (Ac 0, Re 1) e em G leva ao
   plano n = 50 (Ac 1, Re 2). */
static const int AMOSTRA_NQA[NUM_FAIXAS] = {
    13, 13, 13, 13, 13, 13, 50, 50, 80, 125, 200, 315, 500, 800, 1250
};
static const int ACEITACAO_NQA[NUM_FAIXAS] = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 5, 7, 10, 14, 21
};

/* Especificacoes internas da Fujiy Instrumentais (empresa ficticia). */
static const Especificacao ESPECIFICACOES[] = {
    {"CUR", "dureza_hrc",              "HRC",   50.0, 56.0},
    {"CUR", "angulo_corte_graus",      "graus", 70.0, 80.0},
    {"SON", "dureza_hrc",              "HRC",   48.0, 54.0},
    {"SON", "diametro_ponta_mm",       "mm",     0.20, 0.40},
    {"ESP", "dureza_hrc",              "HRC",   45.0, 52.0},
    {"ESP", "espessura_lamina_mm",     "mm",     0.60, 0.90},
    {"ESC", "dureza_hrc",              "HRC",   50.0, 56.0},
    {"ESC", "diametro_colher_mm",      "mm",     1.00, 1.60},
    {"PIN", "dureza_hrc",              "HRC",   40.0, 48.0},
    {"PIN", "desalinhamento_garras_mm","mm",     0.00, 0.20},
    {"TES", "dureza_hrc",              "HRC",   50.0, 58.0},
    {"TES", "folga_laminas_mm",        "mm",     0.00, 0.05},
    {"PAG", "dureza_hrc",              "HRC",   40.0, 48.0},
    {"PAG", "forca_fechamento_n",      "N",     15.0, 25.0}
};
#define NUM_ESPECIFICACOES ((int) (sizeof(ESPECIFICACOES) / sizeof(ESPECIFICACOES[0])))

/* Devolve o plano pelos ponteiros. Quando a amostra e maior que o lote,
   o lote inteiro e inspecionado (inspecao 100%). */
int plano_amostragem(int tamanho_lote, char *letra, int *amostra,
                     int *aceitacao, int *rejeicao)
{
    int faixa = 0;

    if (tamanho_lote < 2) {
        return -1;
    }
    while (faixa < NUM_FAIXAS - 1 && tamanho_lote > LIMITE_FAIXA[faixa]) {
        faixa++;
    }
    *letra = LETRAS[faixa];
    *amostra = AMOSTRA_NQA[faixa];
    *aceitacao = ACEITACAO_NQA[faixa];
    *rejeicao = *aceitacao + 1;
    if (*amostra > tamanho_lote) {
        *amostra = tamanho_lote;
    }
    return 0;
}

/* Codigo do produto no formato LL-FFF-NNN: FFF e a familia. */
void familia_do_produto(const char *codigo, char *familia)
{
    if (strlen(codigo) >= 6) {
        strncpy(familia, codigo + 3, 3);
        familia[3] = '\0';
    } else {
        familia[0] = '\0';
    }
}

/* Preenche um vetor de ponteiros com as especificacoes da familia e
   devolve quantas encontrou. */
int especificacoes_da_familia(const char *familia, const Especificacao *lista[], int maximo)
{
    int i, total = 0;

    for (i = 0; i < NUM_ESPECIFICACOES && total < maximo; i++) {
        if (strcmp(ESPECIFICACOES[i].familia, familia) == 0) {
            lista[total] = &ESPECIFICACOES[i];
            total++;
        }
    }
    return total;
}

const Especificacao *buscar_especificacao(const char *familia, const char *caracteristica)
{
    int i;

    for (i = 0; i < NUM_ESPECIFICACOES; i++) {
        if (strcmp(ESPECIFICACOES[i].familia, familia) == 0 &&
            strcmp(ESPECIFICACOES[i].caracteristica, caracteristica) == 0) {
            return &ESPECIFICACOES[i];
        }
    }
    return NULL;
}

int medicao_conforme(const Especificacao *esp, double valor)
{
    return valor >= esp->minimo && valor <= esp->maximo;
}

/* Calcula o plano, decide aceitar ou rejeitar o lote e atualiza a OP.
   Lote aprovado ganha o apontamento da etapa 8; as pecas defeituosas
   encontradas na amostra saem como refugo. Lote reprovado fica
   segregado (situacao REPROVADA) para analise da qualidade. */
int registrar_inspecao(BaseDados *bd, Inspecao *insp, int minutos)
{
    int ind_op, ind_oper, resultado;
    OrdemProducao *op;
    Apontamento ap;

    if (bd->num_inspecoes >= MAX_INSPECOES) {
        return ERRO_LIMITE;
    }
    ind_op = buscar_op(bd, insp->numero_op);
    ind_oper = buscar_operador(bd, insp->inspetor);
    if (ind_op < 0 || ind_oper < 0) {
        return ERRO_NAO_ENCONTRADO;
    }
    op = &bd->ops[ind_op];
    if (op->situacao != OP_EM_INSPECAO) {
        return ERRO_SITUACAO;
    }
    if (!bd->operadores[ind_oper].habilitado[NUM_ETAPAS - 1]) {
        return ERRO_HABILITACAO;
    }

    insp->tamanho_lote = op->qtd_boa;
    if (plano_amostragem(insp->tamanho_lote, &insp->letra_codigo, &insp->amostra,
                         &insp->aceitacao, &insp->rejeicao) != 0) {
        return ERRO_QUANTIDADE;
    }
    if (insp->defeitos < 0 || insp->defeitos > insp->amostra) {
        return ERRO_QUANTIDADE;
    }
    insp->resultado = (insp->defeitos <= insp->aceitacao) ? 'A' : 'R';

    if (insp->resultado == 'A') {
        memset(&ap, 0, sizeof(ap));
        ap.numero_op = op->numero;
        ap.etapa = NUM_ETAPAS;
        strcpy(ap.matricula, insp->inspetor);
        data_hora_agora(ap.fim);
        recuar_minutos(ap.fim, minutos, ap.inicio);
        ap.minutos = minutos;
        ap.qtd_refugo = insp->defeitos;
        ap.qtd_boa = op->qtd_boa - insp->defeitos;
        if (insp->defeitos > 0) {
            strcpy(ap.motivo, "defeito na inspecao final");
        }
        resultado = aplicar_apontamento(bd, &ap);
        if (resultado != OK) {
            return resultado;
        }
    } else {
        op->situacao = OP_REPROVADA;
    }

    bd->inspecoes[bd->num_inspecoes] = *insp;
    bd->num_inspecoes++;
    return OK;
}

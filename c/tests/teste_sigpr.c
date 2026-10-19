/*
 * teste_sigpr.c
 * Testes automatizados das regras do terminal. Cada VERIFICA conta um
 * caso; o programa termina com codigo 1 se algum falhar, o que faz a
 * integracao continua (GitHub Actions) acusar o erro.
 */
#include <stdio.h>
#include <string.h>
#include "modelos.h"
#include "entrada.h"
#include "arquivos.h"
#include "producao.h"
#include "qualidade.h"
#include "rastreio.h"
#include "relatorios.h"

static int total = 0;
static int falhas = 0;

#define VERIFICA(condicao)                                                  \
    do {                                                                    \
        total++;                                                            \
        if (!(condicao)) {                                                  \
            falhas++;                                                       \
            printf("  FALHOU (linha %d): %s\n", __LINE__, #condicao);       \
        }                                                                   \
    } while (0)

static BaseDados bd;

static void teste_datas(void)
{
    char data[11], hora[17];
    long dia;
    int erros = 0;

    printf("datas e horarios\n");
    VERIFICA(data_valida("2026-10-19"));
    VERIFICA(data_valida("2024-02-29"));
    VERIFICA(!data_valida("2026-02-29"));
    VERIFICA(!data_valida("2026-13-01"));
    VERIFICA(!data_valida("19/10/2026"));
    VERIFICA(data_para_dias("2026-03-01") - data_para_dias("2026-02-28") == 1);
    VERIFICA(data_para_dias("2024-03-01") - data_para_dias("2024-02-28") == 2);
    VERIFICA(data_para_dias("2027-01-01") - data_para_dias("2026-01-01") == 365);

    /* ida e volta em cinco anos inteiros */
    for (dia = data_para_dias("2024-01-01"); dia <= data_para_dias("2028-12-31"); dia++) {
        dias_para_data(dia, data);
        if (!data_valida(data) || data_para_dias(data) != dia) {
            erros++;
        }
    }
    VERIFICA(erros == 0);

    VERIFICA(minutos_entre("2026-10-19 07:30", "2026-10-19 09:05") == 95);
    VERIFICA(minutos_entre("2026-10-19 23:40", "2026-10-20 00:20") == 40);
    recuar_minutos("2026-10-19 00:10", 20, hora);
    VERIFICA(strcmp(hora, "2026-10-18 23:50") == 0);
}

static void teste_cnpj(void)
{
    printf("validacao de CNPJ\n");
    VERIFICA(cnpj_valido("11444777000161"));
    VERIFICA(!cnpj_valido("11444777000162"));
    VERIFICA(!cnpj_valido("11111111111111"));
    VERIFICA(!cnpj_valido("1144477700016"));
    VERIFICA(!cnpj_valido("11a44777000161"));
}

static void teste_amostragem(void)
{
    char letra;
    int n, ac, re;

    printf("plano de amostragem NBR 5426 (nivel II, NQA 1,0)\n");
    VERIFICA(plano_amostragem(1, &letra, &n, &ac, &re) == -1);
    plano_amostragem(8, &letra, &n, &ac, &re);
    VERIFICA(letra == 'A' && n == 8 && ac == 0 && re == 1);        /* 100% do lote */
    plano_amostragem(90, &letra, &n, &ac, &re);
    VERIFICA(letra == 'E' && n == 13 && ac == 0);
    plano_amostragem(91, &letra, &n, &ac, &re);
    VERIFICA(letra == 'F' && n == 13 && ac == 0);
    plano_amostragem(151, &letra, &n, &ac, &re);
    VERIFICA(letra == 'G' && n == 50 && ac == 1 && re == 2);
    plano_amostragem(500, &letra, &n, &ac, &re);
    VERIFICA(letra == 'H' && n == 50 && ac == 1);
    plano_amostragem(501, &letra, &n, &ac, &re);
    VERIFICA(letra == 'J' && n == 80 && ac == 2 && re == 3);
    plano_amostragem(1201, &letra, &n, &ac, &re);
    VERIFICA(letra == 'K' && n == 125 && ac == 3);
    plano_amostragem(600000, &letra, &n, &ac, &re);
    VERIFICA(letra == 'Q' && n == 1250 && ac == 21 && re == 22);
}

/* Regressao: o nome "desalinhamento_garras_mm" tem 24 letras e nao cabia
   num char[24] junto com o '\0'. */
static void teste_especificacoes(void)
{
    const char *familias[] = {"CUR", "SON", "ESP", "ESC", "PIN", "TES", "PAG"};
    const Especificacao *lista[MAX_CARACT];
    Inspecao insp;
    int f, i, n, cabem = 1;

    printf("especificacoes das familias\n");
    for (f = 0; f < 7; f++) {
        n = especificacoes_da_familia(familias[f], lista, MAX_CARACT);
        VERIFICA(n == 2);
        for (i = 0; i < n; i++) {
            if (strlen(lista[i]->caracteristica) >= sizeof(insp.caract[0])) {
                cabem = 0;
            }
        }
    }
    VERIFICA(cabem);
    strcpy(insp.caract[0], "desalinhamento_garras_mm");
    VERIFICA(buscar_especificacao("PIN", insp.caract[0]) != NULL);
    VERIFICA(medicao_conforme(buscar_especificacao("PIN", "desalinhamento_garras_mm"), 0.08));
    VERIFICA(!medicao_conforme(buscar_especificacao("SON", "dureza_hrc"), 45.25));
}

static void teste_campos(void)
{
    char linha[] = "OD-CUR-001;Cureta Gracey 5/6;;AISI420";
    char campo[12];
    char *cursor = linha;

    printf("leitura de campos CSV\n");
    VERIFICA(proximo_campo(&cursor, campo, sizeof(campo)) && strcmp(campo, "OD-CUR-001") == 0);
    proximo_campo(&cursor, campo, sizeof(campo));
    VERIFICA(strcmp(campo, "Cureta Grac") == 0);      /* cortado no tamanho do destino */
    proximo_campo(&cursor, campo, sizeof(campo));
    VERIFICA(campo[0] == '\0');                      /* campo vazio */
    proximo_campo(&cursor, campo, sizeof(campo));
    VERIFICA(strcmp(campo, "AISI420") == 0 && cursor == NULL);
    VERIFICA(proximo_campo(&cursor, campo, sizeof(campo)) == 0);
}

static void preparar_base(void)
{
    int i;

    memset(&bd, 0, sizeof(bd));
    strcpy(bd.produtos[0].codigo, "OD-CUR-001");
    strcpy(bd.produtos[0].descricao, "Cureta Gracey 5/6");
    strcpy(bd.produtos[0].aco, "AISI420");
    bd.num_produtos = 1;

    strcpy(bd.operadores[0].matricula, "1001");
    strcpy(bd.operadores[0].nome, "Operador geral");
    for (i = 0; i < NUM_ETAPAS - 1; i++) {
        bd.operadores[0].habilitado[i] = 1;
    }
    strcpy(bd.operadores[1].matricula, "1009");
    strcpy(bd.operadores[1].nome, "Inspetora");
    bd.operadores[1].habilitado[NUM_ETAPAS - 1] = 1;
    bd.num_operadores = 2;

    strcpy(bd.lotes_mp[0].codigo, "MP-2609-001");
    strcpy(bd.lotes_mp[0].aco, "AISI420");
    bd.lotes_mp[0].peso_kg = 100.0;
    bd.lotes_mp[0].saldo_kg = 100.0;
    strcpy(bd.lotes_mp[1].codigo, "MP-2609-002");
    strcpy(bd.lotes_mp[1].aco, "AISI304");
    bd.lotes_mp[1].peso_kg = 50.0;
    bd.lotes_mp[1].saldo_kg = 50.0;
    bd.num_lotes_mp = 2;
}

static void preencher_apontamento(Apontamento *ap, int op, int etapa, const char *matricula,
                                  int boas, int refugo, const char *motivo)
{
    memset(ap, 0, sizeof(*ap));
    ap->numero_op = op;
    ap->etapa = etapa;
    strcpy(ap->matricula, matricula);
    strcpy(ap->inicio, "2026-10-19 08:00");
    strcpy(ap->fim, "2026-10-19 09:00");
    ap->minutos = 60;
    ap->qtd_boa = boas;
    ap->qtd_refugo = refugo;
    strcpy(ap->motivo, motivo);
}

static void teste_regras_apontamento(void)
{
    OrdemProducao op;
    Operador oper;
    int i;

    printf("regras do apontamento\n");
    memset(&op, 0, sizeof(op));
    memset(&oper, 0, sizeof(oper));
    op.situacao = OP_EM_PRODUCAO;
    op.etapa_atual = 2;
    op.qtd_boa = 100;
    for (i = 0; i < NUM_ETAPAS - 1; i++) {
        oper.habilitado[i] = 1;
    }

    VERIFICA(validar_apontamento(&op, &oper, 3, 98, 2, "rebarba") == OK);
    VERIFICA(validar_apontamento(&op, &oper, 4, 100, 0, "") == ERRO_SEQUENCIA);
    VERIFICA(validar_apontamento(&op, &oper, 3, 97, 2, "rebarba") == ERRO_QUANTIDADE);
    VERIFICA(validar_apontamento(&op, &oper, 3, 98, 2, "") == ERRO_MOTIVO);
    oper.habilitado[2] = 0;
    VERIFICA(validar_apontamento(&op, &oper, 3, 100, 0, "") == ERRO_HABILITACAO);
    op.situacao = OP_REPROVADA;
    VERIFICA(validar_apontamento(&op, &oper, 3, 100, 0, "") == ERRO_SITUACAO);
}

static void teste_fluxo_completo(void)
{
    Apontamento ap;
    Inspecao insp;
    Expedicao exp;
    char cnpjs[5][15];
    int numero, etapa, ops[5];

    printf("fluxo completo: OP -> etapas -> inspecao -> expedicao -> rastreio\n");
    preparar_base();

    VERIFICA(abrir_op(&bd, "XX-XXX-999", 10, "2026-11-30", "2026-10-19") == ERRO_NAO_ENCONTRADO);
    VERIFICA(abrir_op(&bd, "OD-CUR-001", 10, "2026-10-01", "2026-10-19") == ERRO_DATA);
    numero = abrir_op(&bd, "OD-CUR-001", 200, "2026-11-30", "2026-10-19");
    VERIFICA(numero == 1);
    VERIFICA(strcmp(bd.ops[0].lote, "L2610-0001") == 0);

    VERIFICA(registrar_consumo(&bd, numero, "MP-2609-002", 5.0) == ERRO_MATERIAL);
    VERIFICA(registrar_consumo(&bd, numero, "MP-2609-001", 150.0) == ERRO_SALDO);
    VERIFICA(registrar_consumo(&bd, numero, "MP-2609-001", 12.5) == OK);
    VERIFICA(bd.lotes_mp[0].saldo_kg > 87.49 && bd.lotes_mp[0].saldo_kg < 87.51);

    /* etapas 1 a 7; na forja (2) saem 4 pecas trincadas */
    for (etapa = 1; etapa < NUM_ETAPAS; etapa++) {
        if (etapa == 2) {
            preencher_apontamento(&ap, numero, etapa, "1001", 196, 4, "trinca no forjamento");
        } else {
            preencher_apontamento(&ap, numero, etapa, "1001", bd.ops[0].qtd_boa, 0, "");
        }
        VERIFICA(aplicar_apontamento(&bd, &ap) == OK);
    }
    VERIFICA(bd.ops[0].situacao == OP_EM_INSPECAO);
    VERIFICA(bd.ops[0].qtd_boa == 196 && bd.ops[0].qtd_refugo == 4);

    /* operador sem habilitacao na inspecao nao pode inspecionar */
    memset(&insp, 0, sizeof(insp));
    insp.numero_op = numero;
    strcpy(insp.inspetor, "1001");
    VERIFICA(registrar_inspecao(&bd, &insp, 30) == ERRO_HABILITACAO);

    /* 196 pecas -> letra G -> amostra 50, Ac 1: um defeito ainda aprova */
    strcpy(insp.inspetor, "1009");
    strcpy(insp.data, "2026-10-19");
    insp.defeitos = 1;
    VERIFICA(registrar_inspecao(&bd, &insp, 30) == OK);
    VERIFICA(insp.letra_codigo == 'G' && insp.amostra == 50 && insp.resultado == 'A');
    VERIFICA(bd.ops[0].situacao == OP_APROVADA && bd.ops[0].qtd_boa == 195);

    memset(&exp, 0, sizeof(exp));
    exp.numero_op = numero;
    exp.nota_fiscal = 5001;
    strcpy(exp.data, "2026-10-20");
    strcpy(exp.cliente, "Cliente teste");
    strcpy(exp.cnpj_cliente, "11444777000162");
    exp.quantidade = 100;
    VERIFICA(registrar_expedicao(&bd, &exp) == ERRO_CNPJ);
    strcpy(exp.cnpj_cliente, "11444777000161");
    VERIFICA(registrar_expedicao(&bd, &exp) == OK);
    VERIFICA(bd.ops[0].situacao == OP_APROVADA);           /* ainda ha saldo */
    exp.nota_fiscal = 5002;
    exp.quantidade = 96;
    VERIFICA(registrar_expedicao(&bd, &exp) == ERRO_SALDO);
    exp.quantidade = 95;
    VERIFICA(registrar_expedicao(&bd, &exp) == OK);
    VERIFICA(bd.ops[0].situacao == OP_EXPEDIDA);

    VERIFICA(ops_do_lote_mp(&bd, "MP-2609-001", ops, 5) == 1 && ops[0] == numero);
    VERIFICA(clientes_do_lote_mp(&bd, "MP-2609-001", cnpjs, 5) == 1);
    VERIFICA(ops_do_lote_mp(&bd, "MP-2609-002", ops, 5) == 0);
}

static void teste_inspecao_reprovada(void)
{
    Apontamento ap;
    Inspecao insp;
    int numero, etapa;

    printf("inspecao com lote reprovado\n");
    preparar_base();
    numero = abrir_op(&bd, "OD-CUR-001", 120, "2026-11-30", "2026-10-19");
    registrar_consumo(&bd, numero, "MP-2609-001", 8.0);
    for (etapa = 1; etapa < NUM_ETAPAS; etapa++) {
        preencher_apontamento(&ap, numero, etapa, "1001", 120, 0, "");
        aplicar_apontamento(&bd, &ap);
    }
    memset(&insp, 0, sizeof(insp));
    insp.numero_op = numero;
    strcpy(insp.inspetor, "1009");
    insp.defeitos = 1;                       /* 120 pecas -> F -> n 13, Ac 0 */
    VERIFICA(registrar_inspecao(&bd, &insp, 20) == OK);
    VERIFICA(insp.resultado == 'R' && bd.ops[0].situacao == OP_REPROVADA);
    VERIFICA(bd.num_apontamentos == NUM_ETAPAS - 1);      /* etapa 8 nao foi apontada */
}

static void teste_gargalo_e_refugo(void)
{
    int minutos[3][NUM_ETAPAS] = {
        {10, 20, 30, 15, 50, 25, 5, 10},
        {12, 18, 33, 14, 55, 27, 6, 12},
        { 8, 22, 29, 16, 45, 23, 4,  9}
    };
    int pecas[3][NUM_ETAPAS] = {
        {25, 25, 25, 25, 25, 25, 25, 25},
        {25, 25, 25, 25, 25, 25, 25, 25},
        {25, 25, 25, 25, 25, 25, 25, 25}
    };
    double medias[NUM_ETAPAS];
    int gargalo;
    int refugo[NUM_ETAPAS][12], entradas[NUM_ETAPAS];

    printf("gargalo pela matriz de tempos e matriz de refugo\n");
    VERIFICA(calcular_gargalo(minutos, pecas, 3, medias, &gargalo) == 0);
    VERIFICA(gargalo == 5);
    VERIFICA(medias[4] > 1.99 && medias[4] < 2.01);        /* 150 min / 75 pecas */

    preparar_base();
    bd.num_apontamentos = 2;
    preencher_apontamento(&bd.apontamentos[0], 1, 2, "1001", 96, 4, "trinca");
    preencher_apontamento(&bd.apontamentos[1], 2, 2, "1001", 48, 2, "trinca");
    strcpy(bd.apontamentos[1].fim, "2026-09-10 10:00");
    montar_matriz_refugo(&bd, 2026, refugo, entradas);
    VERIFICA(refugo[1][9] == 4 && refugo[1][8] == 2 && entradas[1] == 150);
}

static void teste_sequenciamento(void)
{
    int indices[3] = {0, 1, 2};
    int atrasadas_fifo, atrasadas_edd;
    long dias_fifo, dias_edd, maior;

    printf("sequenciamento pela data de entrega (EDD)\n");
    preparar_base();
    bd.num_ops = 3;
    bd.ops[0].numero = 1;
    strcpy(bd.ops[0].data_entrega, "2026-11-20");
    bd.ops[0].qtd_boa = 600;
    bd.ops[1].numero = 2;
    strcpy(bd.ops[1].data_entrega, "2026-10-22");
    bd.ops[1].qtd_boa = 200;
    bd.ops[2].numero = 3;
    strcpy(bd.ops[2].data_entrega, "2026-10-25");
    bd.ops[2].qtd_boa = 300;

    simular_sequencia(&bd, indices, 3, "2026-10-19", NULL, &atrasadas_fifo, &dias_fifo, &maior);
    ordenar_por_entrega(&bd, indices, 3);
    VERIFICA(indices[0] == 1 && indices[1] == 2 && indices[2] == 0);
    simular_sequencia(&bd, indices, 3, "2026-10-19", NULL, &atrasadas_edd, &dias_edd, &maior);
    VERIFICA(atrasadas_edd <= atrasadas_fifo && dias_edd < dias_fifo);
}

int main(void)
{
    configurar_console();
    teste_datas();
    teste_cnpj();
    teste_amostragem();
    teste_especificacoes();
    teste_campos();
    teste_regras_apontamento();
    teste_fluxo_completo();
    teste_inspecao_reprovada();
    teste_gargalo_e_refugo();
    teste_sequenciamento();

    printf("\n%d verificacoes, %d falha(s).\n", total, falhas);
    return falhas == 0 ? 0 : 1;
}

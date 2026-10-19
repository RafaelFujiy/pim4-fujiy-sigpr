/*
 * modelos.h
 * Registros (structs), limites e codigos usados por todos os modulos do
 * terminal de apontamento do SIGPR - Fujiy Instrumentais.
 */
#ifndef MODELOS_H
#define MODELOS_H

/* Limites dos vetores. Foram dimensionados para cerca de um ano de
   producao da fabrica; acima disso os dados vao para o SQL Server. */
#define MAX_PRODUTOS      50
#define MAX_OPERADORES    50
#define MAX_LOTES_MP      200
#define MAX_OPS           500
#define MAX_CONSUMOS      1000
#define MAX_APONTAMENTOS  5000
#define MAX_INSPECOES     500
#define MAX_EXPEDICOES    1000

#define NUM_ETAPAS        8
#define MAX_CARACT        4
#define TAM_LINHA         512

/* Situacao da ordem de producao (OP) */
#define OP_ABERTA         0
#define OP_EM_PRODUCAO    1
#define OP_EM_INSPECAO    2
#define OP_APROVADA       3
#define OP_REPROVADA      4
#define OP_EXPEDIDA       5

typedef struct {
    char codigo[12];        /* ex.: OD-CUR-001 */
    char descricao[60];
    char linha[10];         /* ODONTO ou CIRURGICO */
    char aco[10];           /* ex.: AISI420 */
    int  classe_risco;
} Produto;

typedef struct {
    char matricula[8];
    char nome[50];
    int  habilitado[NUM_ETAPAS];    /* 1 = pode apontar a etapa */
} Operador;

typedef struct {
    char   codigo[16];              /* ex.: MP-2609-001 */
    char   fornecedor[50];
    char   aco[10];
    char   corrida[16];             /* numero da corrida no certificado da usina */
    double peso_kg;
    double saldo_kg;
    char   data_recebimento[11];    /* AAAA-MM-DD */
} LoteMP;

typedef struct {
    int  numero;
    char produto[12];
    int  quantidade;
    char data_abertura[11];
    char data_entrega[11];
    int  situacao;
    int  etapa_atual;       /* ultima etapa concluida (0 = nenhuma) */
    int  qtd_boa;           /* pecas boas apos a ultima etapa */
    int  qtd_refugo;        /* refugo acumulado */
    char lote[12];          /* lote do produto acabado, ex.: L2610-0042 */
} OrdemProducao;

typedef struct {
    int    numero_op;
    char   lote_mp[16];
    double peso_kg;
} Consumo;

typedef struct {
    int  numero_op;
    int  etapa;
    char matricula[8];
    char inicio[17];        /* AAAA-MM-DD HH:MM */
    char fim[17];
    int  minutos;
    int  qtd_boa;
    int  qtd_refugo;
    char motivo[40];
} Apontamento;

typedef struct {
    int    numero_op;
    char   data[11];
    int    tamanho_lote;
    char   letra_codigo;
    int    amostra;
    int    aceitacao;
    int    rejeicao;
    int    defeitos;
    char   resultado;       /* 'A' aprovado ou 'R' reprovado */
    char   inspetor[8];
    int    num_caract;
    char   caract[MAX_CARACT][32];
    double valor[MAX_CARACT];
} Inspecao;

typedef struct {
    int  nota_fiscal;
    char data[11];
    char cnpj_cliente[15];  /* so digitos */
    char cliente[60];
    int  numero_op;
    int  quantidade;
} Expedicao;

/* Todos os dados carregados dos arquivos ficam juntos neste registro.
   As funcoes recebem um ponteiro para ele em vez de usar globais. */
typedef struct {
    Produto       produtos[MAX_PRODUTOS];
    int           num_produtos;
    Operador      operadores[MAX_OPERADORES];
    int           num_operadores;
    LoteMP        lotes_mp[MAX_LOTES_MP];
    int           num_lotes_mp;
    OrdemProducao ops[MAX_OPS];
    int           num_ops;
    Consumo       consumos[MAX_CONSUMOS];
    int           num_consumos;
    Apontamento   apontamentos[MAX_APONTAMENTOS];
    int           num_apontamentos;
    Inspecao      inspecoes[MAX_INSPECOES];
    int           num_inspecoes;
    Expedicao     expedicoes[MAX_EXPEDICOES];
    int           num_expedicoes;
} BaseDados;

#endif

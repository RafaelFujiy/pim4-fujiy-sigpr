/*
 * producao.c
 * Regras da ordem de producao (OP): abertura, consumo de materia-prima,
 * apontamento das etapas na sequencia correta e expedicao do lote.
 */
#include <stdio.h>
#include <string.h>
#include "modelos.h"
#include "producao.h"
#include "entrada.h"

static const char *NOMES_ETAPAS[NUM_ETAPAS] = {
    "Corte da barra",
    "Forjamento",
    "Usinagem",
    "Tratamento térmico",
    "Retífica e afiação",
    "Polimento e passivação",
    "Gravação a laser",
    "Inspeção final e embalagem"
};

static const char *NOMES_SITUACOES[] = {
    "ABERTA", "EM_PRODUCAO", "EM_INSPECAO", "APROVADA", "REPROVADA", "EXPEDIDA"
};

/* Minutos por peca medidos pelo PCP em cada etapa (tempo padrao). */
static const double TEMPO_PADRAO_POR_PECA[NUM_ETAPAS] = {
    0.40, 0.90, 1.60, 0.70, 2.10, 1.20, 0.25, 0.50
};

const char *nome_etapa(int etapa)
{
    if (etapa < 1 || etapa > NUM_ETAPAS) {
        return "?";
    }
    return NOMES_ETAPAS[etapa - 1];
}

const char *nome_situacao(int situacao)
{
    if (situacao < OP_ABERTA || situacao > OP_EXPEDIDA) {
        return "?";
    }
    return NOMES_SITUACOES[situacao];
}

int situacao_por_nome(const char *nome)
{
    int i;

    for (i = OP_ABERTA; i <= OP_EXPEDIDA; i++) {
        if (strcmp(nome, NOMES_SITUACOES[i]) == 0) {
            return i;
        }
    }
    return OP_ABERTA;
}

double tempo_padrao_peca(int etapa)
{
    if (etapa < 1 || etapa > NUM_ETAPAS) {
        return 0.0;
    }
    return TEMPO_PADRAO_POR_PECA[etapa - 1];
}

const char *mensagem_erro(int codigo)
{
    switch (codigo) {
    case OK:                  return "Operação concluída.";
    case ERRO_SITUACAO:       return "A situação da OP não permite esta operação.";
    case ERRO_SEQUENCIA:      return "Etapa fora de sequência.";
    case ERRO_QUANTIDADE:     return "Peças boas + refugo devem somar as peças que entraram na etapa.";
    case ERRO_MOTIVO:         return "Informe o motivo do refugo.";
    case ERRO_HABILITACAO:    return "Operador não habilitado para esta etapa.";
    case ERRO_NAO_ENCONTRADO: return "Registro não encontrado.";
    case ERRO_SALDO:          return "Quantidade maior que o saldo disponível.";
    case ERRO_LIMITE:         return "Limite de registros atingido.";
    case ERRO_DATA:           return "Data inválida.";
    case ERRO_CNPJ:           return "CNPJ inválido.";
    case ERRO_MATERIAL:       return "O aço do lote não corresponde ao especificado para o produto.";
    default:                  return "Erro desconhecido.";
    }
}

/* ---------- Buscas (sequenciais: os vetores sao pequenos) ---------- */

int buscar_produto(const BaseDados *bd, const char *codigo)
{
    int i;

    for (i = 0; i < bd->num_produtos; i++) {
        if (strcmp(bd->produtos[i].codigo, codigo) == 0) {
            return i;
        }
    }
    return -1;
}

int buscar_operador(const BaseDados *bd, const char *matricula)
{
    int i;

    for (i = 0; i < bd->num_operadores; i++) {
        if (strcmp(bd->operadores[i].matricula, matricula) == 0) {
            return i;
        }
    }
    return -1;
}

int buscar_lote_mp(const BaseDados *bd, const char *codigo)
{
    int i;

    for (i = 0; i < bd->num_lotes_mp; i++) {
        if (strcmp(bd->lotes_mp[i].codigo, codigo) == 0) {
            return i;
        }
    }
    return -1;
}

int buscar_op(const BaseDados *bd, int numero)
{
    int i;

    for (i = 0; i < bd->num_ops; i++) {
        if (bd->ops[i].numero == numero) {
            return i;
        }
    }
    return -1;
}

int buscar_op_por_lote(const BaseDados *bd, const char *lote)
{
    int i;

    for (i = 0; i < bd->num_ops; i++) {
        if (strcmp(bd->ops[i].lote, lote) == 0) {
            return i;
        }
    }
    return -1;
}

/* ---------- Abertura da OP ---------- */

int proximo_numero_op(const BaseDados *bd)
{
    int i, maior = 0;

    for (i = 0; i < bd->num_ops; i++) {
        if (bd->ops[i].numero > maior) {
            maior = bd->ops[i].numero;
        }
    }
    return maior + 1;
}

/* Lote = L + ano e mes da abertura + numero da OP. Ex.: L2610-0042.
   E este codigo que a gravacao a laser marca na peca. */
void gerar_codigo_lote(int numero, const char *data, char *destino)
{
    sprintf(destino, "L%c%c%c%c-%04d", data[2], data[3], data[5], data[6], numero % 10000);
}

int abrir_op(BaseDados *bd, const char *produto, int quantidade,
             const char *data_entrega, const char *hoje)
{
    OrdemProducao *op;

    if (bd->num_ops >= MAX_OPS) {
        return ERRO_LIMITE;
    }
    if (buscar_produto(bd, produto) < 0) {
        return ERRO_NAO_ENCONTRADO;
    }
    if (quantidade <= 0) {
        return ERRO_QUANTIDADE;
    }
    if (!data_valida(data_entrega) || data_para_dias(data_entrega) < data_para_dias(hoje)) {
        return ERRO_DATA;
    }

    op = &bd->ops[bd->num_ops];
    op->numero = proximo_numero_op(bd);
    strcpy(op->produto, produto);
    op->quantidade = quantidade;
    strcpy(op->data_abertura, hoje);
    strcpy(op->data_entrega, data_entrega);
    op->situacao = OP_ABERTA;
    op->etapa_atual = 0;
    op->qtd_boa = quantidade;
    op->qtd_refugo = 0;
    gerar_codigo_lote(op->numero, hoje, op->lote);
    bd->num_ops++;
    return op->numero;
}

/* Registra a barra de aco usada na OP. E este vinculo que permite o
   rastreio do lote do produto ate o certificado da usina. */
int registrar_consumo(BaseDados *bd, int numero_op, const char *lote_mp, double peso_kg)
{
    int ind_op, ind_lote, ind_prod;
    LoteMP *lote;
    Consumo *c;

    if (bd->num_consumos >= MAX_CONSUMOS) {
        return ERRO_LIMITE;
    }
    ind_op = buscar_op(bd, numero_op);
    ind_lote = buscar_lote_mp(bd, lote_mp);
    if (ind_op < 0 || ind_lote < 0) {
        return ERRO_NAO_ENCONTRADO;
    }
    if (bd->ops[ind_op].situacao != OP_ABERTA && bd->ops[ind_op].situacao != OP_EM_PRODUCAO) {
        return ERRO_SITUACAO;
    }
    lote = &bd->lotes_mp[ind_lote];
    ind_prod = buscar_produto(bd, bd->ops[ind_op].produto);
    if (ind_prod < 0 || strcmp(lote->aco, bd->produtos[ind_prod].aco) != 0) {
        return ERRO_MATERIAL;
    }
    if (peso_kg <= 0.0 || peso_kg > lote->saldo_kg) {
        return ERRO_SALDO;
    }

    lote->saldo_kg -= peso_kg;
    c = &bd->consumos[bd->num_consumos];
    c->numero_op = numero_op;
    strcpy(c->lote_mp, lote_mp);
    c->peso_kg = peso_kg;
    bd->num_consumos++;
    return OK;
}

/* ---------- Apontamento ---------- */

/* So valida; nao altera nada. Separar a validacao da gravacao permite
   testar as regras sem mexer nos arquivos. */
int validar_apontamento(const OrdemProducao *op, const Operador *operador, int etapa,
                        int qtd_boa, int qtd_refugo, const char *motivo)
{
    if (etapa < NUM_ETAPAS) {
        if (op->situacao != OP_ABERTA && op->situacao != OP_EM_PRODUCAO) {
            return ERRO_SITUACAO;
        }
    } else if (op->situacao != OP_EM_INSPECAO) {
        return ERRO_SITUACAO;
    }
    if (etapa != op->etapa_atual + 1) {
        return ERRO_SEQUENCIA;
    }
    if (!operador->habilitado[etapa - 1]) {
        return ERRO_HABILITACAO;
    }
    if (qtd_boa < 0 || qtd_refugo < 0 || qtd_boa + qtd_refugo != op->qtd_boa) {
        return ERRO_QUANTIDADE;
    }
    if (qtd_refugo > 0 && motivo[0] == '\0') {
        return ERRO_MOTIVO;
    }
    return OK;
}

int aplicar_apontamento(BaseDados *bd, const Apontamento *ap)
{
    int ind_op, ind_oper, resultado;
    OrdemProducao *op;

    if (bd->num_apontamentos >= MAX_APONTAMENTOS) {
        return ERRO_LIMITE;
    }
    ind_op = buscar_op(bd, ap->numero_op);
    ind_oper = buscar_operador(bd, ap->matricula);
    if (ind_op < 0 || ind_oper < 0) {
        return ERRO_NAO_ENCONTRADO;
    }
    op = &bd->ops[ind_op];
    resultado = validar_apontamento(op, &bd->operadores[ind_oper], ap->etapa,
                                    ap->qtd_boa, ap->qtd_refugo, ap->motivo);
    if (resultado != OK) {
        return resultado;
    }

    bd->apontamentos[bd->num_apontamentos] = *ap;
    bd->num_apontamentos++;

    op->etapa_atual = ap->etapa;
    op->qtd_boa = ap->qtd_boa;
    op->qtd_refugo += ap->qtd_refugo;
    if (ap->etapa == NUM_ETAPAS) {
        op->situacao = OP_APROVADA;
    } else if (ap->etapa == NUM_ETAPAS - 1) {
        op->situacao = OP_EM_INSPECAO;     /* gravado o lote, segue para a qualidade */
    } else {
        op->situacao = OP_EM_PRODUCAO;
    }
    return OK;
}

/* ---------- Expedicao ---------- */

int quantidade_expedida(const BaseDados *bd, int numero_op)
{
    int i, total = 0;

    for (i = 0; i < bd->num_expedicoes; i++) {
        if (bd->expedicoes[i].numero_op == numero_op) {
            total += bd->expedicoes[i].quantidade;
        }
    }
    return total;
}

/* Lote so sai da fabrica depois de aprovado na inspecao final. */
int registrar_expedicao(BaseDados *bd, const Expedicao *exp)
{
    int ind_op, saldo;
    OrdemProducao *op;

    if (bd->num_expedicoes >= MAX_EXPEDICOES) {
        return ERRO_LIMITE;
    }
    ind_op = buscar_op(bd, exp->numero_op);
    if (ind_op < 0) {
        return ERRO_NAO_ENCONTRADO;
    }
    op = &bd->ops[ind_op];
    if (op->situacao != OP_APROVADA) {
        return ERRO_SITUACAO;
    }
    if (!cnpj_valido(exp->cnpj_cliente)) {
        return ERRO_CNPJ;
    }
    if (!data_valida(exp->data)) {
        return ERRO_DATA;
    }
    saldo = op->qtd_boa - quantidade_expedida(bd, op->numero);
    if (exp->quantidade <= 0 || exp->quantidade > saldo) {
        return ERRO_SALDO;
    }

    bd->expedicoes[bd->num_expedicoes] = *exp;
    bd->num_expedicoes++;
    if (exp->quantidade == saldo) {
        op->situacao = OP_EXPEDIDA;
    }
    return OK;
}

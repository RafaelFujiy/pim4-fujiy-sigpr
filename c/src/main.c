/*
 * main.c
 * SIGPR - Sistema Integrado de Gestao da Producao e Rastreabilidade
 * Fujiy Instrumentais Ltda. (empresa ficticia) - terminal de chao de fabrica
 *
 * Projeto Integrado Multidisciplinar IV - UNIP
 * Autor: Rafael Fujiy Ceregati
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modelos.h"
#include "entrada.h"
#include "arquivos.h"
#include "producao.h"
#include "qualidade.h"
#include "rastreio.h"
#include "relatorios.h"

#define PASTA_DADOS      "dados"
#define PASTA_EXPORTACAO "exportar"

static void cabecalho(const char *titulo)
{
    char hoje[11];

    data_hoje(hoje);
    printf("\n==============================================================\n");
    printf(" SIGPR | Fujiy Instrumentais | %s\n", hoje);
    printf(" %s\n", titulo);
    printf("==============================================================\n");
}

static void mostrar_resultado(int codigo)
{
    printf("%s\n", mensagem_erro(codigo));
}

/* ---------- Ordens de producao ---------- */

static void listar_produtos(const BaseDados *bd)
{
    int i;

    printf("\nCódigo      Descrição                                  Aço\n");
    for (i = 0; i < bd->num_produtos; i++) {
        printf("%-11s ", bd->produtos[i].codigo);
        imprimir_coluna(bd->produtos[i].descricao, 42);
        printf(" %s\n", bd->produtos[i].aco);
    }
}

static void listar_lotes_disponiveis(const BaseDados *bd, const char *aco)
{
    int i;

    printf("\nLotes de matéria-prima em estoque (%s):\n", aco);
    for (i = 0; i < bd->num_lotes_mp; i++) {
        if (strcmp(bd->lotes_mp[i].aco, aco) == 0 && bd->lotes_mp[i].saldo_kg > 0.0) {
            printf("  %-12s corrida %-10s saldo %8.2f kg  %s\n", bd->lotes_mp[i].codigo,
                   bd->lotes_mp[i].corrida, bd->lotes_mp[i].saldo_kg,
                   bd->lotes_mp[i].fornecedor);
        }
    }
}

static void tela_abrir_op(BaseDados *bd)
{
    char produto[12], entrega[11], hoje[11], lote_mp[16], detalhe[120];
    int quantidade, numero, ind_prod, resultado, consumos = 0;
    double peso;

    cabecalho("Abertura de ordem de produção");
    listar_produtos(bd);
    do {
        ler_texto("\nCódigo do produto: ", produto, sizeof(produto));
        ind_prod = buscar_produto(bd, produto);
        if (ind_prod < 0) {
            printf("Produto não cadastrado.\n");
        }
    } while (ind_prod < 0);
    quantidade = ler_inteiro("Quantidade de peças: ", 1, 5000);
    data_hoje(hoje);
    ler_data("Data de entrega (AAAA-MM-DD): ", entrega);

    numero = abrir_op(bd, produto, quantidade, entrega, hoje);
    if (numero < 0) {
        mostrar_resultado(numero);
        return;
    }

    /* toda OP precisa de pelo menos um lote de aco vinculado */
    listar_lotes_disponiveis(bd, bd->produtos[ind_prod].aco);
    do {
        ler_texto("Lote de matéria-prima: ", lote_mp, sizeof(lote_mp));
        peso = ler_real("Peso consumido (kg): ", 0.01, 10000.0);
        resultado = registrar_consumo(bd, numero, lote_mp, peso);
        mostrar_resultado(resultado);
        if (resultado == OK) {
            consumos++;
        }
    } while (consumos == 0 || confirmar("Registrar outro lote de matéria-prima?"));

    salvar_ops(bd, PASTA_DADOS);
    salvar_consumos(bd, PASTA_DADOS);
    salvar_lotes_mp(bd, PASTA_DADOS);
    sprintf(detalhe, "OP %d;%s;%d pecas;entrega %s", numero, produto, quantidade, entrega);
    registrar_auditoria(PASTA_DADOS, "ABERTURA_OP", detalhe);
    printf("\nOP %d aberta. Lote %s.\n", numero, bd->ops[buscar_op(bd, numero)].lote);
}

static void menu_ops(BaseDados *bd)
{
    int opcao;
    char hoje[11];

    do {
        cabecalho("Ordens de produção");
        printf(" 1 - Listar OPs\n 2 - Abrir OP\n 3 - Sequência sugerida pela data de entrega\n 0 - Voltar\n");
        opcao = ler_inteiro("Opção: ", 0, 3);
        switch (opcao) {
        case 1:
            listar_ops(bd);
            break;
        case 2:
            tela_abrir_op(bd);
            break;
        case 3:
            data_hoje(hoje);
            relatorio_sequenciamento(bd, hoje);
            break;
        default:
            break;
        }
    } while (opcao != 0);
}

/* ---------- Apontamento ---------- */

static void tela_apontamento(BaseDados *bd)
{
    Apontamento ap;
    OrdemProducao *op;
    char detalhe[120];
    int ind_op, ind_oper, resultado;

    cabecalho("Apontamento de etapa");
    ind_op = buscar_op(bd, ler_inteiro("Número da OP: ", 1, 999999));
    if (ind_op < 0) {
        mostrar_resultado(ERRO_NAO_ENCONTRADO);
        return;
    }
    op = &bd->ops[ind_op];
    printf("OP %d | %s | lote %s | situação %s | %d peças na etapa\n", op->numero,
           op->produto, op->lote, nome_situacao(op->situacao), op->qtd_boa);
    if (op->etapa_atual + 1 >= NUM_ETAPAS) {
        printf("A próxima etapa é a inspeção final: use o menu Qualidade.\n");
        return;
    }

    memset(&ap, 0, sizeof(ap));
    ap.numero_op = op->numero;
    ap.etapa = op->etapa_atual + 1;
    printf("Próxima etapa: %d - %s\n", ap.etapa, nome_etapa(ap.etapa));

    ler_texto("Matrícula do operador: ", ap.matricula, sizeof(ap.matricula));
    ind_oper = buscar_operador(bd, ap.matricula);
    if (ind_oper < 0) {
        mostrar_resultado(ERRO_NAO_ENCONTRADO);
        return;
    }
    printf("Operador: %s\n", bd->operadores[ind_oper].nome);
    /* recusa logo, antes de pedir tempos e quantidades */
    if (!bd->operadores[ind_oper].habilitado[ap.etapa - 1]) {
        mostrar_resultado(ERRO_HABILITACAO);
        return;
    }
    ap.minutos = ler_inteiro("Minutos trabalhados na etapa: ", 1, 1440);
    ap.qtd_boa = ler_inteiro("Peças boas: ", 0, op->qtd_boa);
    ap.qtd_refugo = op->qtd_boa - ap.qtd_boa;
    if (ap.qtd_refugo > 0) {
        printf("Refugo calculado: %d peça(s).\n", ap.qtd_refugo);
        ler_texto("Motivo do refugo: ", ap.motivo, sizeof(ap.motivo));
    }
    data_hora_agora(ap.fim);
    recuar_minutos(ap.fim, ap.minutos, ap.inicio);

    resultado = aplicar_apontamento(bd, &ap);
    mostrar_resultado(resultado);
    if (resultado == OK) {
        salvar_apontamentos(bd, PASTA_DADOS);
        salvar_ops(bd, PASTA_DADOS);
        sprintf(detalhe, "OP %d;etapa %d;operador %s;boas %d;refugo %d", ap.numero_op,
                ap.etapa, ap.matricula, ap.qtd_boa, ap.qtd_refugo);
        registrar_auditoria(PASTA_DADOS, "APONTAMENTO", detalhe);
        printf("OP %d agora em %s.\n", op->numero, nome_situacao(op->situacao));
    }
}

/* ---------- Qualidade ---------- */

static void tela_inspecao(BaseDados *bd)
{
    Inspecao insp;
    OrdemProducao *op;
    const Especificacao *specs[MAX_CARACT];
    char familia[4], pergunta[120], detalhe[120];
    int ind_op, num_specs, i, minutos, resultado;

    cabecalho("Inspeção final por amostragem (ABNT NBR 5426)");
    ind_op = buscar_op(bd, ler_inteiro("Número da OP: ", 1, 999999));
    if (ind_op < 0) {
        mostrar_resultado(ERRO_NAO_ENCONTRADO);
        return;
    }
    op = &bd->ops[ind_op];
    if (op->situacao != OP_EM_INSPECAO) {
        printf("OP %d está em %s; só lotes EM_INSPECAO podem ser inspecionados.\n",
               op->numero, nome_situacao(op->situacao));
        return;
    }

    memset(&insp, 0, sizeof(insp));
    insp.numero_op = op->numero;
    insp.tamanho_lote = op->qtd_boa;
    plano_amostragem(insp.tamanho_lote, &insp.letra_codigo, &insp.amostra,
                     &insp.aceitacao, &insp.rejeicao);
    printf("Lote %s com %d peças -> letra-código %c, amostra de %d peças, Ac %d, Re %d (NQA %.1f)\n",
           op->lote, insp.tamanho_lote, insp.letra_codigo, insp.amostra,
           insp.aceitacao, insp.rejeicao, NQA_PADRAO);

    ler_texto("Matrícula do inspetor: ", insp.inspetor, sizeof(insp.inspetor));
    familia_do_produto(op->produto, familia);
    num_specs = especificacoes_da_familia(familia, specs, MAX_CARACT);
    for (i = 0; i < num_specs; i++) {
        sprintf(pergunta, "Média da amostra - %s (%s, de %.2f a %.2f): ",
                specs[i]->caracteristica, specs[i]->unidade, specs[i]->minimo, specs[i]->maximo);
        strcpy(insp.caract[i], specs[i]->caracteristica);
        insp.valor[i] = ler_real(pergunta, 0.0, 1000.0);
        if (!medicao_conforme(specs[i], insp.valor[i])) {
            printf("  Atenção: valor fora da especificação.\n");
        }
    }
    insp.num_caract = num_specs;
    insp.defeitos = ler_inteiro("Peças defeituosas na amostra: ", 0, insp.amostra);
    minutos = ler_inteiro("Minutos gastos na inspeção e embalagem: ", 1, 1440);
    data_hoje(insp.data);

    resultado = registrar_inspecao(bd, &insp, minutos);
    if (resultado != OK) {
        mostrar_resultado(resultado);
        return;
    }
    printf("\nResultado: lote %s %s (%d defeito(s); aceita com até %d).\n", op->lote,
           insp.resultado == 'A' ? "APROVADO" : "REPROVADO", insp.defeitos, insp.aceitacao);
    if (insp.resultado == 'R') {
        printf("Lote segregado. Abra o registro de não conformidade para a Qualidade.\n");
    }
    salvar_inspecoes(bd, PASTA_DADOS);
    salvar_apontamentos(bd, PASTA_DADOS);
    salvar_ops(bd, PASTA_DADOS);
    sprintf(detalhe, "OP %d;lote %s;amostra %d;defeitos %d;%s", op->numero, op->lote,
            insp.amostra, insp.defeitos, insp.resultado == 'A' ? "APROVADO" : "REPROVADO");
    registrar_auditoria(PASTA_DADOS, "INSPECAO", detalhe);
}

/* ---------- Expedicao ---------- */

static void tela_expedicao(BaseDados *bd)
{
    Expedicao exp;
    char texto[TAM_LINHA], detalhe[160];
    int i, saldo, resultado, encontrou = 0;

    cabecalho("Expedição de lote aprovado");
    printf("Lotes aprovados com saldo:\n");
    for (i = 0; i < bd->num_ops; i++) {
        if (bd->ops[i].situacao == OP_APROVADA) {
            saldo = bd->ops[i].qtd_boa - quantidade_expedida(bd, bd->ops[i].numero);
            printf("  OP %4d  lote %s  %-11s saldo %d peças\n", bd->ops[i].numero,
                   bd->ops[i].lote, bd->ops[i].produto, saldo);
            encontrou = 1;
        }
    }
    if (!encontrou) {
        printf("  nenhum\n");
        return;
    }

    memset(&exp, 0, sizeof(exp));
    exp.numero_op = ler_inteiro("Número da OP: ", 1, 999999);
    exp.nota_fiscal = ler_inteiro("Número da nota fiscal: ", 1, 999999999);
    do {
        ler_texto("CNPJ do cliente: ", texto, sizeof(texto));
        somente_digitos(texto, exp.cnpj_cliente, sizeof(exp.cnpj_cliente));
        if (!cnpj_valido(exp.cnpj_cliente)) {
            mostrar_resultado(ERRO_CNPJ);
        }
    } while (!cnpj_valido(exp.cnpj_cliente));
    ler_texto("Razão social do cliente: ", exp.cliente, sizeof(exp.cliente));
    exp.quantidade = ler_inteiro("Quantidade expedida: ", 1, 100000);
    data_hoje(exp.data);

    resultado = registrar_expedicao(bd, &exp);
    mostrar_resultado(resultado);
    if (resultado == OK) {
        salvar_expedicoes(bd, PASTA_DADOS);
        salvar_ops(bd, PASTA_DADOS);
        sprintf(detalhe, "NF %d;OP %d;%d pecas;CNPJ %s", exp.nota_fiscal, exp.numero_op,
                exp.quantidade, exp.cnpj_cliente);
        registrar_auditoria(PASTA_DADOS, "EXPEDICAO", detalhe);
    }
}

/* ---------- Rastreabilidade ---------- */

static void menu_rastreio(const BaseDados *bd)
{
    char codigo[16];
    int opcao;

    do {
        cabecalho("Rastreabilidade");
        printf(" 1 - Por lote do produto (para trás)\n");
        printf(" 2 - Por lote de matéria-prima (para frente / recall)\n 0 - Voltar\n");
        opcao = ler_inteiro("Opção: ", 0, 2);
        if (opcao == 1) {
            ler_texto("Lote do produto (ex.: L2610-0042): ", codigo, sizeof(codigo));
            rastrear_lote_produto(bd, codigo);
        } else if (opcao == 2) {
            ler_texto("Lote de matéria-prima (ex.: MP-2609-001): ", codigo, sizeof(codigo));
            rastrear_lote_mp(bd, codigo);
        }
    } while (opcao != 0);
}

/* ---------- Indicadores ---------- */

static void menu_indicadores(const BaseDados *bd)
{
    char hoje[11];
    int opcao;

    data_hoje(hoje);
    do {
        cabecalho("Indicadores");
        printf(" 1 - Gargalo (matriz de tempos)\n 2 - Refugo por etapa e mês\n");
        printf(" 3 - Entregas no prazo (OTD)\n 0 - Voltar\n");
        opcao = ler_inteiro("Opção: ", 0, 3);
        switch (opcao) {
        case 1:
            relatorio_gargalo(bd);
            break;
        case 2:
            relatorio_refugo(bd, atoi(hoje));
            break;
        case 3:
            relatorio_otd(bd);
            break;
        default:
            break;
        }
    } while (opcao != 0);
}

/* ---------- Recebimento de materia-prima ---------- */

static void tela_recebimento(BaseDados *bd)
{
    LoteMP *lote;
    char detalhe[120];

    cabecalho("Recebimento de matéria-prima");
    if (bd->num_lotes_mp >= MAX_LOTES_MP) {
        mostrar_resultado(ERRO_LIMITE);
        return;
    }
    lote = &bd->lotes_mp[bd->num_lotes_mp];
    memset(lote, 0, sizeof(*lote));
    do {
        ler_texto("Código do lote (ex.: MP-2610-004): ", lote->codigo, sizeof(lote->codigo));
        if (buscar_lote_mp(bd, lote->codigo) >= 0) {
            printf("Código já cadastrado.\n");
        }
    } while (buscar_lote_mp(bd, lote->codigo) >= 0);
    ler_texto("Fornecedor: ", lote->fornecedor, sizeof(lote->fornecedor));
    ler_texto("Aço (AISI420, AISI440 ou AISI304): ", lote->aco, sizeof(lote->aco));
    ler_texto("Corrida (certificado da usina): ", lote->corrida, sizeof(lote->corrida));
    lote->peso_kg = ler_real("Peso recebido (kg): ", 0.1, 50000.0);
    lote->saldo_kg = lote->peso_kg;
    data_hoje(lote->data_recebimento);
    bd->num_lotes_mp++;

    salvar_lotes_mp(bd, PASTA_DADOS);
    sprintf(detalhe, "%s;%s;%.2f kg", lote->codigo, lote->aco, lote->peso_kg);
    registrar_auditoria(PASTA_DADOS, "RECEBIMENTO_MP", detalhe);
    printf("Lote %s registrado.\n", lote->codigo);
}

static void tela_exportacao(const BaseDados *bd)
{
    cabecalho("Exportação para o SQL Server e o MongoDB");
    if (exportar_sqlserver(bd, PASTA_EXPORTACAO) != 0 || exportar_mongodb(bd, PASTA_EXPORTACAO) != 0) {
        printf("Falha na exportação. Verifique se a pasta %s existe.\n", PASTA_EXPORTACAO);
        return;
    }
    printf("Arquivos gerados em %s/:\n", PASTA_EXPORTACAO);
    printf("  carga_lotes_mp.csv     %4d registros\n", bd->num_lotes_mp);
    printf("  carga_ops.csv          %4d registros\n", bd->num_ops);
    printf("  carga_consumos.csv     %4d registros\n", bd->num_consumos);
    printf("  carga_apontamentos.csv %4d registros\n", bd->num_apontamentos);
    printf("  carga_inspecoes.csv    %4d registros\n", bd->num_inspecoes);
    printf("  carga_expedicoes.csv   %4d registros\n", bd->num_expedicoes);
    printf("  laudos.jsonl           %4d documentos\n", bd->num_inspecoes);
    registrar_auditoria(PASTA_DADOS, "EXPORTACAO", "sqlserver e mongodb");
}

int main(void)
{
    /* static: a base tem perto de 1 MB e nao deve ficar na pilha */
    static BaseDados base;
    int opcao;

    configurar_console();
    if (carregar_base(&base, PASTA_DADOS) != 0) {
        printf("Execute o programa a partir da pasta que contém \"%s\".\n", PASTA_DADOS);
        return 1;
    }

    do {
        cabecalho("Terminal de apontamento - menu principal");
        printf(" 1 - Ordens de produção\n");
        printf(" 2 - Apontar etapa\n");
        printf(" 3 - Qualidade: inspeção final\n");
        printf(" 4 - Expedição\n");
        printf(" 5 - Rastreabilidade\n");
        printf(" 6 - Indicadores\n");
        printf(" 7 - Recebimento de matéria-prima\n");
        printf(" 8 - Exportar dados (SQL Server / MongoDB)\n");
        printf(" 0 - Sair\n");
        opcao = ler_inteiro("Opção: ", 0, 8);

        switch (opcao) {
        case 1: menu_ops(&base);          break;
        case 2: tela_apontamento(&base);  break;
        case 3: tela_inspecao(&base);     break;
        case 4: tela_expedicao(&base);    break;
        case 5: menu_rastreio(&base);     break;
        case 6: menu_indicadores(&base);  break;
        case 7: tela_recebimento(&base);  break;
        case 8: tela_exportacao(&base);   break;
        default:                          break;
        }
    } while (opcao != 0);

    printf("Até logo.\n");
    return 0;
}

/*
 * entrada.c
 * Toda leitura do teclado passa por estas funcoes. Usar fgets em vez de
 * scanf evita estouro de buffer e lixo no teclado quando o operador
 * digita letra onde se espera numero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "modelos.h"
#include "entrada.h"

#ifdef _WIN32
#include <windows.h>
#endif

void configurar_console(void)
{
#ifdef _WIN32
    /* os fontes estao em UTF-8; sem isso o console do Windows mostra
       os acentos trocados */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    setlocale(LC_ALL, "");
    /* caracteres continuam em "C": com a localidade do Windows (pagina
       1252) a biblioteca converteria de novo os bytes UTF-8 ao escrever
       no console, e os acentos sairiam trocados */
    setlocale(LC_CTYPE, "C");
    /* numeros continuam com ponto decimal: os arquivos CSV sao lidos
       pelo SQL Server e pelo MongoDB, que esperam ponto */
    setlocale(LC_NUMERIC, "C");
}

void remover_quebra(char *texto)
{
    int tamanho = (int) strlen(texto);

    while (tamanho > 0 && (texto[tamanho - 1] == '\n' || texto[tamanho - 1] == '\r')) {
        texto[tamanho - 1] = '\0';
        tamanho--;
    }
}

/* printf("%-20s") conta bytes, e em UTF-8 cada letra acentuada ocupa
   dois. Aqui a largura e contada em caracteres para alinhar as tabelas. */
void imprimir_coluna(const char *texto, int largura)
{
    int i, caracteres = 0;

    for (i = 0; texto[i] != '\0'; i++) {
        if (((unsigned char) texto[i] & 0xC0) != 0x80) {
            caracteres++;
        }
        if (caracteres > largura) {
            break;
        }
        putchar(texto[i]);
    }
    while (caracteres < largura) {
        putchar(' ');
        caracteres++;
    }
}

/* Copia no maximo tamanho - 1 bytes sem cortar um caractere UTF-8 ao
   meio (letras acentuadas ocupam dois bytes). */
static void copiar_utf8(char *destino, const char *origem, int tamanho)
{
    int n = (int) strlen(origem);

    if (n > tamanho - 1) {
        n = tamanho - 1;
        while (n > 0 && ((unsigned char) origem[n] & 0xC0) == 0x80) {
            n--;
        }
    }
    memcpy(destino, origem, n);
    destino[n] = '\0';
}

#ifdef _WIN32
/* No console do Windows, a leitura em UTF-8 corta a linha no primeiro
   caractere acentuado. Quando a entrada e o teclado, a linha e lida em
   UTF-16 com ReadConsoleW e convertida para UTF-8. Retorna 0 no fim da
   entrada (Ctrl+Z ou Ctrl+C). */
static int ler_console(char *buffer, int tamanho)
{
    wchar_t largo[TAM_LINHA];
    char convertido[TAM_LINHA * 3];
    DWORD lidos;

    if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), largo, TAM_LINHA - 1, &lidos, NULL) ||
        lidos == 0 || largo[0] == 0x1A) {
        return 0;
    }
    largo[lidos] = L'\0';
    if (WideCharToMultiByte(CP_UTF8, 0, largo, -1, convertido, (int) sizeof convertido,
                            NULL, NULL) == 0) {
        return 0;
    }
    copiar_utf8(buffer, convertido, tamanho);
    return 1;
}
#endif

/* Le uma linha; se a entrada terminar (fim de arquivo), encerra o
   programa. Todos os dados ja foram gravados a cada operacao. */
static void ler_linha(char *buffer, int tamanho)
{
    int lido;
#ifdef _WIN32
    DWORD modo;
#endif

    /* garante que a pergunta apareca antes da leitura mesmo quando a
       saida esta redirecionada (sem terminal o stdout usa buffer cheio) */
    fflush(stdout);
#ifdef _WIN32
    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &modo)) {
        lido = ler_console(buffer, tamanho);
    } else {
        lido = fgets(buffer, tamanho, stdin) != NULL;
    }
#else
    lido = fgets(buffer, tamanho, stdin) != NULL;
#endif
    if (!lido) {
        printf("\nEntrada encerrada.\n");
        exit(EXIT_SUCCESS);
    }
    /* arquivo de entrada salvo pelo Bloco de Notas pode comecar com a
       marca BOM do UTF-8 (bytes EF BB BF) */
    if ((unsigned char) buffer[0] == 0xEF && (unsigned char) buffer[1] == 0xBB &&
        (unsigned char) buffer[2] == 0xBF) {
        memmove(buffer, buffer + 3, strlen(buffer + 3) + 1);
    }
    remover_quebra(buffer);
}

void ler_texto(const char *pergunta, char *destino, int tamanho)
{
    char buffer[TAM_LINHA];
    int i;

    do {
        printf("%s", pergunta);
        ler_linha(buffer, TAM_LINHA);
        if (buffer[0] == '\0') {
            printf("Campo obrigatório.\n");
        }
    } while (buffer[0] == '\0');

    /* ';' e o separador dos arquivos CSV, entao nao pode entrar no texto */
    for (i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == ';') {
            buffer[i] = ',';
        }
    }
    copiar_utf8(destino, buffer, tamanho);
}

int ler_inteiro(const char *pergunta, int minimo, int maximo)
{
    char buffer[TAM_LINHA];
    char *fim;
    long valor;

    while (1) {
        printf("%s", pergunta);
        ler_linha(buffer, TAM_LINHA);
        valor = strtol(buffer, &fim, 10);
        if (fim != buffer && *fim == '\0' && valor >= minimo && valor <= maximo) {
            return (int) valor;
        }
        printf("Valor inválido. Digite um número inteiro entre %d e %d.\n", minimo, maximo);
    }
}

double ler_real(const char *pergunta, double minimo, double maximo)
{
    char buffer[TAM_LINHA];
    char *fim;
    double valor;
    int i;

    while (1) {
        printf("%s", pergunta);
        ler_linha(buffer, TAM_LINHA);
        /* aceita virgula, que e como o operador digita no Brasil */
        for (i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == ',') {
                buffer[i] = '.';
            }
        }
        valor = strtod(buffer, &fim);
        if (fim != buffer && *fim == '\0' && valor >= minimo && valor <= maximo) {
            return valor;
        }
        printf("Valor inválido. Digite um número entre %.2f e %.2f.\n", minimo, maximo);
    }
}

void ler_data(const char *pergunta, char *destino)
{
    char buffer[TAM_LINHA];

    while (1) {
        printf("%s", pergunta);
        ler_linha(buffer, TAM_LINHA);
        if (data_valida(buffer)) {
            strncpy(destino, buffer, 10);
            destino[10] = '\0';
            return;
        }
        printf("Data inválida. Use o formato AAAA-MM-DD.\n");
    }
}

int confirmar(const char *pergunta)
{
    char buffer[TAM_LINHA];

    while (1) {
        printf("%s (s/n): ", pergunta);
        ler_linha(buffer, TAM_LINHA);
        if (buffer[0] == 's' || buffer[0] == 'S') {
            return 1;
        }
        if (buffer[0] == 'n' || buffer[0] == 'N') {
            return 0;
        }
    }
}

static int ano_bissexto(int ano)
{
    return (ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0;
}

int data_valida(const char *data)
{
    int dias_no_mes[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int ano, mes, dia, i;

    if (strlen(data) != 10 || data[4] != '-' || data[7] != '-') {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        if (i != 4 && i != 7 && (data[i] < '0' || data[i] > '9')) {
            return 0;
        }
    }
    ano = atoi(data);
    mes = atoi(data + 5);
    dia = atoi(data + 8);
    if (ano < 2000 || ano > 2100 || mes < 1 || mes > 12) {
        return 0;
    }
    if (ano_bissexto(ano)) {
        dias_no_mes[1] = 29;
    }
    return dia >= 1 && dia <= dias_no_mes[mes - 1];
}

/* Converte a data em um numero de dias corridos. Contar o ano a partir
   de marco deixa fevereiro no fim, e o dia extra do ano bissexto nao
   interfere no calculo dos meses anteriores. */
long data_para_dias(const char *data)
{
    long ano, mes, dia;

    ano = atol(data);
    mes = atol(data + 5);
    dia = atol(data + 8);
    mes = (mes + 9) % 12;      /* marco = 0 ... fevereiro = 11 */
    ano = ano - mes / 10;
    return 365 * ano + ano / 4 - ano / 100 + ano / 400 + (mes * 306 + 5) / 10 + (dia - 1);
}

void dias_para_data(long dias, char *destino)
{
    long ano, dia_do_ano, mes_aux, mes, dia;

    /* 10000 * dias passa de 2^31; no Windows long tem 32 bits (no Linux
       tem 64), por isso a conta e feita em long long */
    ano = (long) ((10000LL * dias + 14780) / 3652425);
    dia_do_ano = dias - (365 * ano + ano / 4 - ano / 100 + ano / 400);
    if (dia_do_ano < 0) {
        ano--;
        dia_do_ano = dias - (365 * ano + ano / 4 - ano / 100 + ano / 400);
    }
    mes_aux = (100 * dia_do_ano + 52) / 3060;
    mes = (mes_aux + 2) % 12 + 1;
    ano = ano + (mes_aux + 2) / 12;
    dia = dia_do_ano - (mes_aux * 306 + 5) / 10 + 1;
    sprintf(destino, "%04ld-%02ld-%02ld", ano, mes, dia);
}

/* A variavel de ambiente SIGPR_HOJE fixa a data do sistema. Serve para
   repetir os testes e as demonstracoes com a mesma data. */
void data_hoje(char *destino)
{
    const char *fixa = getenv("SIGPR_HOJE");
    time_t agora;
    struct tm *local;

    if (fixa != NULL && data_valida(fixa)) {
        strcpy(destino, fixa);
        return;
    }
    agora = time(NULL);
    local = localtime(&agora);
    strftime(destino, 11, "%Y-%m-%d", local);
}

void data_hora_agora(char *destino)
{
    time_t agora = time(NULL);
    struct tm *local = localtime(&agora);
    char hoje[11];

    data_hoje(hoje);
    sprintf(destino, "%s %02d:%02d", hoje, local->tm_hour, local->tm_min);
}

int minutos_entre(const char *inicio, const char *fim)
{
    long dias = data_para_dias(fim) - data_para_dias(inicio);
    int min_inicio = atoi(inicio + 11) * 60 + atoi(inicio + 14);
    int min_fim = atoi(fim + 11) * 60 + atoi(fim + 14);

    return (int) (dias * 1440 + min_fim - min_inicio);
}

/* Calcula o horario de inicio a partir do fim e da duracao. */
void recuar_minutos(const char *data_hora, int minutos, char *destino)
{
    long total = data_para_dias(data_hora) * 1440
                 + atoi(data_hora + 11) * 60 + atoi(data_hora + 14) - minutos;
    char data[11];

    dias_para_data(total / 1440, data);
    sprintf(destino, "%s %02ld:%02ld", data, (total % 1440) / 60, total % 60);
}

void somente_digitos(const char *origem, char *destino, int tamanho)
{
    int i, j = 0;

    for (i = 0; origem[i] != '\0' && j < tamanho - 1; i++) {
        if (origem[i] >= '0' && origem[i] <= '9') {
            destino[j] = origem[i];
            j++;
        }
    }
    destino[j] = '\0';
}

/* Digitos verificadores do CNPJ: soma ponderada modulo 11. */
int cnpj_valido(const char *cnpj)
{
    int pesos1[12] = {5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};
    int pesos2[13] = {6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};
    int digitos[14];
    int i, soma, resto, todos_iguais = 1;

    if (strlen(cnpj) != 14) {
        return 0;
    }
    for (i = 0; i < 14; i++) {
        if (cnpj[i] < '0' || cnpj[i] > '9') {
            return 0;
        }
        digitos[i] = cnpj[i] - '0';
        if (digitos[i] != digitos[0]) {
            todos_iguais = 0;
        }
    }
    if (todos_iguais) {
        return 0;
    }

    soma = 0;
    for (i = 0; i < 12; i++) {
        soma += digitos[i] * pesos1[i];
    }
    resto = soma % 11;
    if (digitos[12] != (resto < 2 ? 0 : 11 - resto)) {
        return 0;
    }

    soma = 0;
    for (i = 0; i < 13; i++) {
        soma += digitos[i] * pesos2[i];
    }
    resto = soma % 11;
    return digitos[13] == (resto < 2 ? 0 : 11 - resto);
}

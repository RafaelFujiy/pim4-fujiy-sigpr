/*
 * arquivos.h
 * Leitura e gravacao dos arquivos texto (CSV separado por ';'),
 * registro de auditoria e exportacao para o SQL Server e o MongoDB.
 */
#ifndef ARQUIVOS_H
#define ARQUIVOS_H

#include "modelos.h"

int  proximo_campo(char **cursor, char *destino, int tamanho);

int  carregar_base(BaseDados *bd, const char *pasta);
int  salvar_lotes_mp(const BaseDados *bd, const char *pasta);
int  salvar_ops(const BaseDados *bd, const char *pasta);
int  salvar_consumos(const BaseDados *bd, const char *pasta);
int  salvar_apontamentos(const BaseDados *bd, const char *pasta);
int  salvar_inspecoes(const BaseDados *bd, const char *pasta);
int  salvar_expedicoes(const BaseDados *bd, const char *pasta);

void registrar_auditoria(const char *pasta, const char *evento, const char *detalhe);

int  exportar_sqlserver(const BaseDados *bd, const char *pasta_saida);
int  exportar_mongodb(const BaseDados *bd, const char *pasta_saida);

#endif

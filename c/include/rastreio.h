/*
 * rastreio.h
 * Rastreabilidade do lote: para tras (produto -> materia-prima) e para
 * frente (materia-prima -> produtos -> clientes), base de um recall.
 */
#ifndef RASTREIO_H
#define RASTREIO_H

#include "modelos.h"

int  ops_do_lote_mp(const BaseDados *bd, const char *lote_mp, int numeros_op[], int maximo);
int  clientes_do_lote_mp(const BaseDados *bd, const char *lote_mp, char cnpjs[][15], int maximo);
void rastrear_lote_produto(const BaseDados *bd, const char *lote);
void rastrear_lote_mp(const BaseDados *bd, const char *lote_mp);

#endif

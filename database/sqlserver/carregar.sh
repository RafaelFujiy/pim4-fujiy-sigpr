#!/bin/bash
# Importa os arquivos exportados pelo terminal em C (pasta /carga) para a
# area de carga do SIGPR com o bcp em UTF-8 e depois executa a carga nas
# tabelas definitivas.
# Uso: docker exec sigpr-sqlserver bash /scripts/carregar.sh
set -e

FERRAMENTAS=/opt/mssql-tools18/bin
CONEXAO=(-S localhost -U sa -P "$MSSQL_SA_PASSWORD" -d SIGPR)

"$FERRAMENTAS/sqlcmd" "${CONEXAO[@]}" -C -b -Q "
    TRUNCATE TABLE stg.LoteMP;      TRUNCATE TABLE stg.OrdemProducao;
    TRUNCATE TABLE stg.Consumo;     TRUNCATE TABLE stg.Apontamento;
    TRUNCATE TABLE stg.Inspecao;    TRUNCATE TABLE stg.Expedicao;"

for par in stg.LoteMP:carga_lotes_mp.csv \
           stg.OrdemProducao:carga_ops.csv \
           stg.Consumo:carga_consumos.csv \
           stg.Apontamento:carga_apontamentos.csv \
           stg.Inspecao:carga_inspecoes.csv \
           stg.Expedicao:carga_expedicoes.csv; do
    tabela="${par%%:*}"
    arquivo="${par##*:}"
    linhas=$("$FERRAMENTAS/bcp" "$tabela" in "/carga/$arquivo" "${CONEXAO[@]}" -u \
             -c -t ';' -r '0x0a' -F 2 -C 65001 | grep -o '[0-9]* rows copied')
    echo "$arquivo -> $tabela: $linhas"
done

"$FERRAMENTAS/sqlcmd" "${CONEXAO[@]}" -C -b -W -s ' | ' -Q "EXEC stg.sp_CarregarTerminal"

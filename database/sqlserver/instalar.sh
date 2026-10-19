#!/bin/bash
# Cria o banco SIGPR do zero e executa todos os scripts em ordem.
# Antes, gere os arquivos de carga no terminal em C (menu 8 - Exportar).
# Uso: docker exec sigpr-sqlserver bash /scripts/instalar.sh
set -e

SQLCMD=(/opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P "$MSSQL_SA_PASSWORD" -C -I -f 65001 -b -W -s ' | ')

for script in 01_criar_banco 02_tabelas 03_dados_cadastro 04_views_funcoes_procedures 05_carga_terminal; do
    echo "== $script"
    "${SQLCMD[@]}" -i "/scripts/$script.sql" > /dev/null
done

echo "== carga dos arquivos do terminal"
bash /scripts/carregar.sh

for script in 06_consultas 07_indices_desempenho 08_seguranca_backup; do
    echo "== $script"
    "${SQLCMD[@]}" -d SIGPR -i "/scripts/$script.sql"
done

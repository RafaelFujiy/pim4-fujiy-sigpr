/* ============================================================
   SIGPR - Fujiy Instrumentais
   05 - Carga dos arquivos exportados pelo terminal em C
   Os CSV (separador ';', primeira linha de cabecalho, UTF-8) ficam em
   /carga no container (pasta c/exportar do repositorio).
   ============================================================ */
USE SIGPR;
GO

/* Area de carga: mesmas colunas dos arquivos, sem restricoes. Os dados
   so passam para as tabelas definitivas depois de validados. */
DROP TABLE IF EXISTS stg.LoteMP, stg.OrdemProducao, stg.Consumo,
                     stg.Apontamento, stg.Inspecao, stg.Expedicao;
GO
CREATE TABLE stg.LoteMP        (codigo VARCHAR(16), fornecedor VARCHAR(80), aco VARCHAR(10),
                                corrida VARCHAR(16), peso_kg DECIMAL(9,2), saldo_kg DECIMAL(9,2),
                                data_recebimento DATE);
CREATE TABLE stg.OrdemProducao (numero INT, produto VARCHAR(12), quantidade INT, data_abertura DATE,
                                data_entrega DATE, situacao VARCHAR(12), etapa_atual TINYINT,
                                qtd_boa INT, qtd_refugo INT, lote VARCHAR(12));
CREATE TABLE stg.Consumo       (numero_op INT, lote_mp VARCHAR(16), peso_kg DECIMAL(9,2));
CREATE TABLE stg.Apontamento   (numero_op INT, etapa TINYINT, matricula VARCHAR(8), inicio VARCHAR(16),
                                fim VARCHAR(16), minutos INT, qtd_boa INT, qtd_refugo INT,
                                motivo VARCHAR(60));
CREATE TABLE stg.Inspecao      (numero_op INT, data DATE, tamanho_lote INT, letra CHAR(1), amostra INT,
                                ac INT, re INT, defeitos INT, resultado VARCHAR(9), inspetor VARCHAR(8));
CREATE TABLE stg.Expedicao     (nota_fiscal INT, data DATE, cnpj_cliente CHAR(14), cliente VARCHAR(80),
                                numero_op INT, quantidade INT);
GO

/* Importacao no SQL Server para Windows: le os seis arquivos com BULK
   INSERT (SQL dinamico porque o caminho do arquivo nao aceita variavel).
   No container Linux a opcao CODEPAGE nao existe e o arquivo UTF-8 seria
   lido como pagina de codigo OEM; ali a importacao e feita pelo utilitario
   bcp com -C 65001 (script carregar.sh). */
CREATE OR ALTER PROCEDURE stg.sp_ImportarArquivos
    @pasta VARCHAR(200) = '/carga/'
AS
BEGIN
    SET NOCOUNT ON;
    DECLARE @sql NVARCHAR(MAX) = N'', @tabela SYSNAME, @arquivo VARCHAR(60);

    IF (SELECT host_platform FROM sys.dm_os_host_info) = N'Linux'
        THROW 50030, 'No Linux use o script carregar.sh (bcp -C 65001).', 1;
    DECLARE arquivos CURSOR LOCAL FAST_FORWARD FOR
        SELECT tabela, arquivo FROM (VALUES
            ('stg.LoteMP',        'carga_lotes_mp.csv'),
            ('stg.OrdemProducao', 'carga_ops.csv'),
            ('stg.Consumo',       'carga_consumos.csv'),
            ('stg.Apontamento',   'carga_apontamentos.csv'),
            ('stg.Inspecao',      'carga_inspecoes.csv'),
            ('stg.Expedicao',     'carga_expedicoes.csv')) AS a (tabela, arquivo);

    OPEN arquivos;
    FETCH NEXT FROM arquivos INTO @tabela, @arquivo;
    WHILE @@FETCH_STATUS = 0
    BEGIN
        SET @sql = N'TRUNCATE TABLE ' + @tabela + N';
                     BULK INSERT ' + @tabela + N' FROM ''' + @pasta + @arquivo + N'''
                     WITH (FORMAT = ''CSV'', FIRSTROW = 2, FIELDTERMINATOR = '';'',
                           ROWTERMINATOR = ''0x0a'', CODEPAGE = ''65001'', TABLOCK);';
        EXEC sys.sp_executesql @sql;
        FETCH NEXT FROM arquivos INTO @tabela, @arquivo;
    END
    CLOSE arquivos;
    DEALLOCATE arquivos;
END;
GO

/* Passa da area de carga para as tabelas definitivas numa unica
   transacao. Registros que ja existem sao atualizados (MERGE) e o
   que tiver chave desconhecida e recusado e listado no final. */
CREATE OR ALTER PROCEDURE stg.sp_CarregarTerminal
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    BEGIN TRANSACTION;

    MERGE prod.LoteMateriaPrima AS destino
    USING (SELECT s.codigo, f.id_fornecedor, s.aco, s.corrida, s.peso_kg, s.saldo_kg, s.data_recebimento
           FROM stg.LoteMP AS s
           JOIN cad.Fornecedor AS f ON f.razao_social = s.fornecedor) AS origem
       ON destino.codigo = origem.codigo
    WHEN MATCHED THEN
        UPDATE SET saldo_kg = origem.saldo_kg
    WHEN NOT MATCHED THEN
        INSERT (codigo, id_fornecedor, aco, corrida, peso_kg, saldo_kg, data_recebimento)
        VALUES (origem.codigo, origem.id_fornecedor, origem.aco, origem.corrida,
                origem.peso_kg, origem.saldo_kg, origem.data_recebimento);

    MERGE prod.OrdemProducao AS destino
    USING (SELECT s.numero, p.id_produto, ip.id_item, s.quantidade, s.data_abertura, s.data_entrega,
                  s.situacao, s.etapa_atual, s.qtd_boa, s.qtd_refugo, s.lote
           FROM stg.OrdemProducao AS s
           JOIN cad.Produto AS p ON p.codigo = s.produto
           LEFT JOIN com.ItemPedido AS ip ON ip.id_item = s.numero AND ip.id_produto = p.id_produto
          ) AS origem
       ON destino.numero = origem.numero
    WHEN MATCHED THEN
        UPDATE SET situacao = origem.situacao, etapa_atual = origem.etapa_atual,
                   qtd_boa = origem.qtd_boa, qtd_refugo = origem.qtd_refugo
    WHEN NOT MATCHED THEN
        INSERT (numero, id_produto, id_item_pedido, quantidade, data_abertura, data_entrega,
                situacao, etapa_atual, qtd_boa, qtd_refugo, lote)
        VALUES (origem.numero, origem.id_produto, origem.id_item, origem.quantidade,
                origem.data_abertura, origem.data_entrega, origem.situacao, origem.etapa_atual,
                origem.qtd_boa, origem.qtd_refugo, origem.lote);

    INSERT INTO prod.ConsumoMateriaPrima (numero_op, codigo_lote_mp, peso_kg)
    SELECT s.numero_op, s.lote_mp, s.peso_kg
    FROM stg.Consumo AS s
    WHERE NOT EXISTS (SELECT 1 FROM prod.ConsumoMateriaPrima AS c
                      WHERE c.numero_op = s.numero_op AND c.codigo_lote_mp = s.lote_mp);

    INSERT INTO prod.Apontamento
        (numero_op, id_etapa, matricula, inicio, fim, minutos, qtd_boa, qtd_refugo, motivo_refugo)
    SELECT s.numero_op, s.etapa, s.matricula, CAST(s.inicio AS DATETIME2(0)),
           CAST(s.fim AS DATETIME2(0)), s.minutos, s.qtd_boa, s.qtd_refugo, NULLIF(s.motivo, '')
    FROM stg.Apontamento AS s
    WHERE NOT EXISTS (SELECT 1 FROM prod.Apontamento AS a
                      WHERE a.numero_op = s.numero_op AND a.id_etapa = s.etapa);

    INSERT INTO qual.Inspecao
        (numero_op, data_inspecao, tamanho_lote, letra_codigo, amostra, aceitacao, rejeicao,
         defeitos, resultado, matricula_inspetor)
    SELECT s.numero_op, s.data, s.tamanho_lote, s.letra, s.amostra, s.ac, s.re,
           s.defeitos, s.resultado, s.inspetor
    FROM stg.Inspecao AS s
    WHERE NOT EXISTS (SELECT 1 FROM qual.Inspecao AS i WHERE i.numero_op = s.numero_op);

    /* lote reprovado sem registro de nao conformidade ganha um */
    INSERT INTO qual.NaoConformidade (numero_op, id_inspecao, data_abertura, descricao)
    SELECT i.numero_op, i.id_inspecao, i.data_inspecao,
           CONCAT('Lote reprovado na inspecao final: ', i.defeitos,
                  ' defeito(s) em amostra de ', i.amostra, ' (Re = ', i.rejeicao, ').')
    FROM qual.Inspecao AS i
    WHERE i.resultado = 'REPROVADO'
      AND NOT EXISTS (SELECT 1 FROM qual.NaoConformidade AS n WHERE n.id_inspecao = i.id_inspecao);

    INSERT INTO com.Expedicao (nota_fiscal, id_cliente, data_emissao)
    SELECT DISTINCT s.nota_fiscal, c.id_cliente, s.data
    FROM stg.Expedicao AS s
    JOIN cad.Cliente AS c ON c.cnpj = s.cnpj_cliente
    WHERE cad.fn_CnpjValido(s.cnpj_cliente) = 1
      AND NOT EXISTS (SELECT 1 FROM com.Expedicao AS e WHERE e.nota_fiscal = s.nota_fiscal);

    INSERT INTO com.ItemExpedicao (nota_fiscal, numero_op, quantidade)
    SELECT s.nota_fiscal, s.numero_op, s.quantidade
    FROM stg.Expedicao AS s
    WHERE EXISTS (SELECT 1 FROM com.Expedicao AS e WHERE e.nota_fiscal = s.nota_fiscal)
      AND NOT EXISTS (SELECT 1 FROM com.ItemExpedicao AS i
                      WHERE i.nota_fiscal = s.nota_fiscal AND i.numero_op = s.numero_op);

    COMMIT TRANSACTION;

    /* Resumo da carga e o que ficou de fora */
    SELECT 'stg.OrdemProducao' AS arquivo, COUNT(*) AS lidos,
           SUM(CASE WHEN p.id_produto IS NULL THEN 1 ELSE 0 END) AS recusados
    FROM stg.OrdemProducao AS s LEFT JOIN cad.Produto AS p ON p.codigo = s.produto
    UNION ALL
    SELECT 'stg.Apontamento', COUNT(*), 0 FROM stg.Apontamento
    UNION ALL
    SELECT 'stg.Inspecao', COUNT(*), 0 FROM stg.Inspecao
    UNION ALL
    SELECT 'stg.Expedicao', COUNT(*),
           SUM(CASE WHEN c.id_cliente IS NULL OR cad.fn_CnpjValido(s.cnpj_cliente) = 0 THEN 1 ELSE 0 END)
    FROM stg.Expedicao AS s LEFT JOIN cad.Cliente AS c ON c.cnpj = s.cnpj_cliente;
END;
GO

/* Uso:
   Windows: EXEC stg.sp_ImportarArquivos 'C:\SIGPR\carga\'; EXEC stg.sp_CarregarTerminal;
   Docker/Linux: docker exec sigpr-sqlserver bash /scripts/carregar.sh */

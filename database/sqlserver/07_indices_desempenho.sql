/* ============================================================
   SIGPR - Fujiy Instrumentais
   07 - Indices e desempenho (teste de carga)
   A tabela carga.Apontamento repete a estrutura de prod.Apontamento e
   recebe cerca de 400 mil linhas sinteticas, volume de muitos anos de
   producao, para medir o efeito do indice antes que ele faca falta.
   ============================================================ */
USE SIGPR;
GO

IF SCHEMA_ID('carga') IS NULL
    EXEC ('CREATE SCHEMA carga');
GO

DROP TABLE IF EXISTS carga.Apontamento;
GO

CREATE TABLE carga.Apontamento (
    id_apontamento  BIGINT        NOT NULL IDENTITY(1,1),
    numero_op       INT           NOT NULL,
    id_etapa        TINYINT       NOT NULL,
    matricula       VARCHAR(8)    NOT NULL,
    inicio          DATETIME2(0)  NOT NULL,
    fim             DATETIME2(0)  NOT NULL,
    minutos         INT           NOT NULL,
    qtd_boa         INT           NOT NULL,
    qtd_refugo      INT           NOT NULL,
    motivo_refugo   VARCHAR(60)   NULL,
    CONSTRAINT PK_carga_Apontamento PRIMARY KEY CLUSTERED (id_apontamento)
);
GO

/* 50 mil OPs x 8 etapas = 400 mil apontamentos entre 2018 e 2026.
   Os numeros vem de uma sequencia gerada com CROSS JOIN. */
WITH n AS (
    SELECT TOP (50000) ROW_NUMBER() OVER (ORDER BY (SELECT NULL)) AS k
    FROM sys.all_objects AS a CROSS JOIN sys.all_objects AS b
),
op AS (
    SELECT k AS numero_op,
           DATEADD(MINUTE, k * 83, CAST('2018-10-01T07:00:00' AS DATETIME2(0))) AS inicio_op,
           100 + (k * 37) % 500 AS pecas
    FROM n
)
INSERT INTO carga.Apontamento
    (numero_op, id_etapa, matricula, inicio, fim, minutos, qtd_boa, qtd_refugo, motivo_refugo)
SELECT op.numero_op, e.id_etapa, CONCAT('10', RIGHT(CONCAT('0', 1 + (op.numero_op + e.id_etapa) % 12), 2)),
       DATEADD(HOUR, 6 * e.id_etapa, op.inicio_op),
       DATEADD(MINUTE, CAST(op.pecas * e.tempo_padrao_min_peca AS INT) + 1,
               DATEADD(HOUR, 6 * e.id_etapa, op.inicio_op)),
       CAST(op.pecas * e.tempo_padrao_min_peca AS INT) + 1,
       op.pecas - (op.numero_op * e.id_etapa) % 4,
       (op.numero_op * e.id_etapa) % 4,
       CASE WHEN (op.numero_op * e.id_etapa) % 4 > 0 THEN 'refugo simulado' END
FROM op
CROSS JOIN cad.Etapa AS e;
GO

SELECT COUNT(*) AS linhas, MIN(fim) AS primeiro, MAX(fim) AS ultimo FROM carga.Apontamento;
GO

/* Consulta de auditoria do PCP: desempenho por etapa num mes. */
PRINT '--- 1) Sem indice em fim ---';
SET STATISTICS IO, TIME ON;
SELECT id_etapa, SUM(minutos) AS minutos, SUM(qtd_refugo) AS refugo,
       CAST(SUM(minutos) * 1.0 / SUM(qtd_boa + qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca
FROM carga.Apontamento
WHERE fim >= '2025-03-01' AND fim < '2025-04-01'
GROUP BY id_etapa
ORDER BY id_etapa;
SET STATISTICS IO, TIME OFF;
GO

SET SHOWPLAN_TEXT ON;
GO
SELECT id_etapa, SUM(minutos) AS minutos, SUM(qtd_refugo) AS refugo,
       CAST(SUM(minutos) * 1.0 / SUM(qtd_boa + qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca
FROM carga.Apontamento
WHERE fim >= '2025-03-01' AND fim < '2025-04-01'
GROUP BY id_etapa
ORDER BY id_etapa;
GO
SET SHOWPLAN_TEXT OFF;
GO

/* Indice nao clusterizado pela data de fim com colunas incluidas: a
   consulta encontra tudo no proprio indice (indice de cobertura) e nao
   precisa voltar a tabela. */
CREATE NONCLUSTERED INDEX IX_carga_Apontamento_fim
    ON carga.Apontamento (fim)
    INCLUDE (id_etapa, minutos, qtd_boa, qtd_refugo);
GO

PRINT '--- 2) Com indice de cobertura em fim ---';
SET STATISTICS IO, TIME ON;
SELECT id_etapa, SUM(minutos) AS minutos, SUM(qtd_refugo) AS refugo,
       CAST(SUM(minutos) * 1.0 / SUM(qtd_boa + qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca
FROM carga.Apontamento
WHERE fim >= '2025-03-01' AND fim < '2025-04-01'
GROUP BY id_etapa
ORDER BY id_etapa;
SET STATISTICS IO, TIME OFF;
GO

SET SHOWPLAN_TEXT ON;
GO
SELECT id_etapa, SUM(minutos) AS minutos, SUM(qtd_refugo) AS refugo,
       CAST(SUM(minutos) * 1.0 / SUM(qtd_boa + qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca
FROM carga.Apontamento
WHERE fim >= '2025-03-01' AND fim < '2025-04-01'
GROUP BY id_etapa
ORDER BY id_etapa;
GO
SET SHOWPLAN_TEXT OFF;
GO

/* Mesma pergunta escrita com funcao sobre a coluna: o filtro deixa de
   ser "pesquisavel" (SARGable) e o indice nao e usado para busca. */
PRINT '--- 3) Funcao sobre a coluna (YEAR/MONTH) ---';
SET STATISTICS IO, TIME ON;
SELECT id_etapa, SUM(minutos) AS minutos, SUM(qtd_refugo) AS refugo,
       CAST(SUM(minutos) * 1.0 / SUM(qtd_boa + qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca
FROM carga.Apontamento
WHERE YEAR(fim) = 2025 AND MONTH(fim) = 3
GROUP BY id_etapa
ORDER BY id_etapa;
SET STATISTICS IO, TIME OFF;
GO

SET SHOWPLAN_TEXT ON;
GO
SELECT id_etapa, SUM(minutos) AS minutos
FROM carga.Apontamento
WHERE YEAR(fim) = 2025 AND MONTH(fim) = 3
GROUP BY id_etapa;
GO
SET SHOWPLAN_TEXT OFF;
GO

/* Tamanho e fragmentacao dos indices da tabela de teste. */
SELECT i.name AS indice, ps.index_type_desc, ps.page_count,
       CAST(ps.avg_fragmentation_in_percent AS DECIMAL(5,2)) AS fragmentacao_perc
FROM sys.dm_db_index_physical_stats(DB_ID(), OBJECT_ID('carga.Apontamento'), NULL, NULL, 'LIMITED') AS ps
JOIN sys.indexes AS i ON i.object_id = ps.object_id AND i.index_id = ps.index_id;
GO

/* Indices definitivos nas tabelas de producao, escolhidos pelas
   consultas do arquivo 06 (chaves estrangeiras usadas em JOIN e filtros
   por data). */
CREATE NONCLUSTERED INDEX IX_Apontamento_fim
    ON prod.Apontamento (fim) INCLUDE (id_etapa, minutos, qtd_boa, qtd_refugo);
CREATE NONCLUSTERED INDEX IX_ConsumoMP_lote
    ON prod.ConsumoMateriaPrima (codigo_lote_mp) INCLUDE (peso_kg);
CREATE NONCLUSTERED INDEX IX_ItemExpedicao_op
    ON com.ItemExpedicao (numero_op) INCLUDE (quantidade);
CREATE NONCLUSTERED INDEX IX_OP_situacao_entrega
    ON prod.OrdemProducao (situacao, data_entrega) INCLUDE (id_produto, qtd_boa, etapa_atual);
GO

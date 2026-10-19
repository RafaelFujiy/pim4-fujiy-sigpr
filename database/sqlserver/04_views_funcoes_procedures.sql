/* ============================================================
   SIGPR - Fujiy Instrumentais
   04 - Views, funcoes, procedures e triggers
   Executar com: sqlcmd -I (QUOTED_IDENTIFIER ON, exigido pela view indexada)
   ============================================================ */
USE SIGPR;
GO

/* ---------- Funcoes ---------- */

/* Plano de amostragem da ABNT NBR 5426 (nivel geral II, plano simples,
   inspecao normal, NQA 1,0), o mesmo usado pelo terminal em C.
   Funcao com valor de tabela em linha: funciona como uma view com
   parametro e o otimizador a expande dentro da consulta. */
CREATE OR ALTER FUNCTION qual.fn_PlanoAmostragem (@tamanho_lote INT)
RETURNS TABLE
AS
RETURN
    SELECT TOP (1)
        f.letra,
        CASE WHEN f.amostra > @tamanho_lote THEN @tamanho_lote ELSE f.amostra END AS amostra,
        f.aceitacao,
        f.aceitacao + 1 AS rejeicao
    FROM (VALUES
        (8,       'A', 13, 0), (15,     'B', 13, 0), (25,     'C', 13, 0),
        (50,      'D', 13, 0), (90,     'E', 13, 0), (150,    'F', 13, 0),
        (280,     'G', 50, 1), (500,    'H', 50, 1), (1200,   'J', 80, 2),
        (3200,    'K', 125, 3), (10000, 'L', 200, 5), (35000, 'M', 315, 7),
        (150000,  'N', 500, 10), (500000, 'P', 800, 14), (2147483647, 'Q', 1250, 21)
    ) AS f (limite, letra, amostra, aceitacao)
    WHERE @tamanho_lote >= 2 AND @tamanho_lote <= f.limite
    ORDER BY f.limite;
GO

/* Digitos verificadores do CNPJ (mesma regra da funcao cnpj_valido em C). */
CREATE OR ALTER FUNCTION cad.fn_CnpjValido (@cnpj CHAR(14))
RETURNS BIT
AS
BEGIN
    DECLARE @i INT = 1, @soma INT = 0, @resto INT, @dv1 INT, @dv2 INT;

    IF @cnpj IS NULL OR @cnpj LIKE '%[^0-9]%' OR LEN(@cnpj) <> 14
       OR @cnpj = REPLICATE(LEFT(@cnpj, 1), 14)
        RETURN 0;

    WHILE @i <= 12
    BEGIN
        SET @soma += CAST(SUBSTRING(@cnpj, @i, 1) AS INT)
                     * CAST(SUBSTRING('543298765432', @i, 1) AS INT);
        SET @i += 1;
    END
    SET @resto = @soma % 11;
    SET @dv1 = CASE WHEN @resto < 2 THEN 0 ELSE 11 - @resto END;

    SELECT @i = 1, @soma = 0;
    WHILE @i <= 13
    BEGIN
        SET @soma += CAST(SUBSTRING(@cnpj, @i, 1) AS INT)
                     * CAST(SUBSTRING('6543298765432', @i, 1) AS INT);
        SET @i += 1;
    END
    SET @resto = @soma % 11;
    SET @dv2 = CASE WHEN @resto < 2 THEN 0 ELSE 11 - @resto END;

    RETURN CASE WHEN CAST(SUBSTRING(@cnpj, 13, 1) AS INT) = @dv1
                 AND CAST(SUBSTRING(@cnpj, 14, 1) AS INT) = @dv2 THEN 1 ELSE 0 END;
END;
GO

/* ---------- Views ---------- */

/* Rastreabilidade completa: uma linha por lote de produto, lote de
   materia-prima consumido e nota fiscal de saida. */
CREATE OR ALTER VIEW prod.vw_Rastreabilidade
AS
SELECT
    op.lote                 AS lote_produto,
    op.numero               AS numero_op,
    p.codigo                AS produto,
    p.descricao,
    op.situacao,
    c.codigo_lote_mp        AS lote_mp,
    mp.corrida,
    f.razao_social          AS fornecedor_aco,
    ie.nota_fiscal,
    e.data_emissao          AS data_expedicao,
    cli.razao_social        AS cliente,
    cli.cnpj                AS cnpj_cliente,
    ie.quantidade           AS pecas_expedidas
FROM prod.OrdemProducao AS op
JOIN cad.Produto AS p                  ON p.id_produto = op.id_produto
JOIN prod.ConsumoMateriaPrima AS c     ON c.numero_op = op.numero
JOIN prod.LoteMateriaPrima AS mp       ON mp.codigo = c.codigo_lote_mp
JOIN cad.Fornecedor AS f               ON f.id_fornecedor = mp.id_fornecedor
LEFT JOIN com.ItemExpedicao AS ie      ON ie.numero_op = op.numero
LEFT JOIN com.Expedicao AS e           ON e.nota_fiscal = ie.nota_fiscal
LEFT JOIN cad.Cliente AS cli           ON cli.id_cliente = e.id_cliente;
GO

/* Painel do PCP: andamento e prazo de cada OP em aberto. */
CREATE OR ALTER VIEW prod.vw_PainelProducao
AS
SELECT
    op.numero,
    op.lote,
    p.codigo                                        AS produto,
    op.quantidade,
    op.qtd_boa,
    op.situacao,
    op.etapa_atual,
    CAST(100.0 * op.etapa_atual / 8 AS DECIMAL(5,1)) AS percentual_concluido,
    op.data_entrega,
    DATEDIFF(DAY, CAST(SYSDATETIME() AS DATE), op.data_entrega) AS dias_para_entrega
FROM prod.OrdemProducao AS op
JOIN cad.Produto AS p ON p.id_produto = op.id_produto
WHERE op.situacao IN ('ABERTA', 'EM_PRODUCAO', 'EM_INSPECAO', 'APROVADA');
GO

/* Desnormalizacao controlada: view indexada (materializada) com o
   refugo somado por etapa e mes. O SQL Server mantem os totais a cada
   apontamento, e o relatorio mensal nao precisa varrer a tabela. */
CREATE OR ALTER VIEW prod.vw_RefugoEtapaMes
WITH SCHEMABINDING
AS
SELECT
    a.id_etapa,
    YEAR(a.fim)                          AS ano,
    MONTH(a.fim)                         AS mes,
    SUM(a.qtd_refugo)                    AS refugo,
    SUM(a.qtd_boa + a.qtd_refugo)        AS pecas_processadas,
    COUNT_BIG(*)                         AS apontamentos
FROM prod.Apontamento AS a
GROUP BY a.id_etapa, YEAR(a.fim), MONTH(a.fim);
GO

CREATE UNIQUE CLUSTERED INDEX IX_vw_RefugoEtapaMes
    ON prod.vw_RefugoEtapaMes (id_etapa, ano, mes);
GO

/* ---------- Procedures ---------- */

/* Apontamento com as mesmas regras do terminal: etapa na sequencia,
   operador habilitado, pecas boas + refugo = pecas que entraram e
   motivo obrigatorio para refugo. Tudo dentro de uma transacao:
   ou grava o apontamento e atualiza a OP, ou nao grava nada. */
CREATE OR ALTER PROCEDURE prod.sp_ApontarEtapa
    @numero_op   INT,
    @id_etapa    TINYINT,
    @matricula   VARCHAR(8),
    @inicio      DATETIME2(0),
    @fim         DATETIME2(0),
    @qtd_boa     INT,
    @qtd_refugo  INT,
    @motivo      VARCHAR(60) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE @situacao VARCHAR(12), @etapa_atual TINYINT, @pecas INT;

    BEGIN TRY
        BEGIN TRANSACTION;

        /* UPDLOCK impede que dois terminais apontem a mesma OP ao mesmo tempo */
        SELECT @situacao = situacao, @etapa_atual = etapa_atual, @pecas = qtd_boa
        FROM prod.OrdemProducao WITH (UPDLOCK, ROWLOCK)
        WHERE numero = @numero_op;

        IF @situacao IS NULL
            THROW 50001, 'OP inexistente.', 1;
        IF @id_etapa < 8 AND @situacao NOT IN ('ABERTA', 'EM_PRODUCAO')
            THROW 50002, 'A situacao da OP nao permite apontamento.', 1;
        IF @id_etapa = 8 AND @situacao <> 'EM_INSPECAO'
            THROW 50002, 'A etapa 8 exige a OP em inspecao.', 1;
        IF @id_etapa <> @etapa_atual + 1
            THROW 50003, 'Etapa fora de sequencia.', 1;
        IF NOT EXISTS (SELECT 1 FROM cad.OperadorEtapa
                       WHERE matricula = @matricula AND id_etapa = @id_etapa)
            THROW 50004, 'Operador nao habilitado para a etapa.', 1;
        IF @qtd_boa < 0 OR @qtd_refugo < 0 OR @qtd_boa + @qtd_refugo <> @pecas
            THROW 50005, 'Boas + refugo devem somar as pecas da etapa.', 1;
        IF @qtd_refugo > 0 AND NULLIF(LTRIM(@motivo), '') IS NULL
            THROW 50006, 'Informe o motivo do refugo.', 1;

        INSERT INTO prod.Apontamento
            (numero_op, id_etapa, matricula, inicio, fim, minutos, qtd_boa, qtd_refugo, motivo_refugo)
        VALUES
            (@numero_op, @id_etapa, @matricula, @inicio, @fim,
             DATEDIFF(MINUTE, @inicio, @fim), @qtd_boa, @qtd_refugo, NULLIF(@motivo, ''));

        UPDATE prod.OrdemProducao
        SET etapa_atual = @id_etapa,
            qtd_boa     = @qtd_boa,
            qtd_refugo  = qtd_refugo + @qtd_refugo,
            situacao    = CASE WHEN @id_etapa = 8 THEN 'APROVADA'
                               WHEN @id_etapa = 7 THEN 'EM_INSPECAO'
                               ELSE 'EM_PRODUCAO' END
        WHERE numero = @numero_op;

        COMMIT TRANSACTION;
    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0
            ROLLBACK TRANSACTION;
        THROW;
    END CATCH
END;
GO

/* Inspecao final: calcula o plano pela funcao, decide o resultado,
   aponta a etapa 8 quando aprovado ou abre nao conformidade quando
   reprovado. */
CREATE OR ALTER PROCEDURE qual.sp_RegistrarInspecao
    @numero_op   INT,
    @matricula   VARCHAR(8),
    @defeitos    INT,
    @minutos     INT,
    @data_hora   DATETIME2(0) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE @tamanho INT, @situacao VARCHAR(12), @letra CHAR(1), @amostra INT,
            @ac INT, @re INT, @resultado VARCHAR(9), @id_inspecao INT;

    SET @data_hora = COALESCE(@data_hora, CAST(SYSDATETIME() AS DATETIME2(0)));

    BEGIN TRY
        BEGIN TRANSACTION;

        SELECT @tamanho = qtd_boa, @situacao = situacao
        FROM prod.OrdemProducao WITH (UPDLOCK, ROWLOCK)
        WHERE numero = @numero_op;

        IF @situacao IS NULL OR @situacao <> 'EM_INSPECAO'
            THROW 50011, 'Somente OP em inspecao pode ser inspecionada.', 1;
        IF NOT EXISTS (SELECT 1 FROM cad.OperadorEtapa WHERE matricula = @matricula AND id_etapa = 8)
            THROW 50004, 'Inspetor nao habilitado.', 1;

        SELECT @letra = letra, @amostra = amostra, @ac = aceitacao, @re = rejeicao
        FROM qual.fn_PlanoAmostragem(@tamanho);

        IF @defeitos < 0 OR @defeitos > @amostra
            THROW 50012, 'Numero de defeitos invalido para a amostra.', 1;

        SET @resultado = CASE WHEN @defeitos <= @ac THEN 'APROVADO' ELSE 'REPROVADO' END;

        INSERT INTO qual.Inspecao
            (numero_op, data_inspecao, tamanho_lote, letra_codigo, amostra, aceitacao,
             rejeicao, defeitos, resultado, matricula_inspetor)
        VALUES
            (@numero_op, CAST(@data_hora AS DATE), @tamanho, @letra, @amostra, @ac,
             @re, @defeitos, @resultado, @matricula);
        SET @id_inspecao = SCOPE_IDENTITY();

        IF @resultado = 'APROVADO'
        BEGIN
            DECLARE @inicio DATETIME2(0) = DATEADD(MINUTE, -@minutos, @data_hora);
            DECLARE @boas INT = @tamanho - @defeitos;
            DECLARE @motivo VARCHAR(60) = CASE WHEN @defeitos > 0 THEN 'defeito na inspecao final' END;
            EXEC prod.sp_ApontarEtapa @numero_op, 8, @matricula, @inicio, @data_hora,
                 @boas, @defeitos, @motivo;
        END
        ELSE
        BEGIN
            UPDATE prod.OrdemProducao SET situacao = 'REPROVADA' WHERE numero = @numero_op;
            INSERT INTO qual.NaoConformidade (numero_op, id_inspecao, data_abertura, descricao)
            VALUES (@numero_op, @id_inspecao, CAST(@data_hora AS DATE),
                    CONCAT('Lote reprovado na inspecao final: ', @defeitos,
                           ' defeito(s) em amostra de ', @amostra, ' (Re = ', @re, ').'));
        END

        COMMIT TRANSACTION;
        SELECT @letra AS letra, @amostra AS amostra, @ac AS ac, @re AS re, @resultado AS resultado;
    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0
            ROLLBACK TRANSACTION;
        THROW;
    END CATCH
END;
GO

/* ---------- Triggers ---------- */

/* Regra de negocio no banco: lote so sai da fabrica aprovado. Vale
   mesmo para quem tentar inserir direto na tabela, sem o terminal. */
CREATE OR ALTER TRIGGER com.trg_ItemExpedicao_LoteAprovado
ON com.ItemExpedicao
AFTER INSERT
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (SELECT 1
               FROM inserted AS i
               JOIN prod.OrdemProducao AS op ON op.numero = i.numero_op
               WHERE op.situacao NOT IN ('APROVADA', 'EXPEDIDA'))
        THROW 50020, 'Lote nao aprovado na inspecao final nao pode ser expedido.', 1;
END;
GO

/* Trilha de auditoria: registros de producao e de inspecao nao podem
   ser alterados ou apagados sem deixar rastro (integridade dos
   registros exigida pelas boas praticas de fabricacao). */
CREATE OR ALTER TRIGGER prod.trg_Apontamento_Auditoria
ON prod.Apontamento
AFTER UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;
    INSERT INTO seg.Auditoria (tabela, operacao, chave, valores_antigos, valores_novos)
    SELECT 'prod.Apontamento',
           CASE WHEN EXISTS (SELECT 1 FROM inserted) THEN 'U' ELSE 'D' END,
           CONCAT('OP ', d.numero_op, ' etapa ', d.id_etapa),
           (SELECT x.* FROM deleted AS x WHERE x.id_apontamento = d.id_apontamento
            FOR JSON PATH, WITHOUT_ARRAY_WRAPPER),
           (SELECT i.* FROM inserted AS i WHERE i.id_apontamento = d.id_apontamento
            FOR JSON PATH, WITHOUT_ARRAY_WRAPPER)
    FROM deleted AS d;
END;
GO

CREATE OR ALTER TRIGGER qual.trg_Inspecao_Auditoria
ON qual.Inspecao
AFTER UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;
    INSERT INTO seg.Auditoria (tabela, operacao, chave, valores_antigos, valores_novos)
    SELECT 'qual.Inspecao',
           CASE WHEN EXISTS (SELECT 1 FROM inserted) THEN 'U' ELSE 'D' END,
           CONCAT('OP ', d.numero_op, ' inspecao ', d.id_inspecao),
           (SELECT x.* FROM deleted AS x WHERE x.id_inspecao = d.id_inspecao
            FOR JSON PATH, WITHOUT_ARRAY_WRAPPER),
           (SELECT i.* FROM inserted AS i WHERE i.id_inspecao = d.id_inspecao
            FOR JSON PATH, WITHOUT_ARRAY_WRAPPER)
    FROM deleted AS d;
END;
GO

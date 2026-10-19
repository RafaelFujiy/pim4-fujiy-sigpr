/* ============================================================
   SIGPR - Fujiy Instrumentais
   08 - Perfis de acesso, testes das regras e copia de seguranca
   ============================================================ */
USE SIGPR;
GO

/* ---------- Perfis (menor privilegio) ---------- */
DROP USER IF EXISTS u_operador;
DROP USER IF EXISTS u_qualidade;
DROP USER IF EXISTS u_gestor;
DROP ROLE IF EXISTS papel_operador;
DROP ROLE IF EXISTS papel_qualidade;
DROP ROLE IF EXISTS papel_gestor;
GO

CREATE ROLE papel_operador;
CREATE ROLE papel_qualidade;
CREATE ROLE papel_gestor;
GO

/* Operador: so aponta pela procedure, que aplica as regras; nao le nem
   altera tabelas diretamente. */
GRANT EXECUTE ON OBJECT::prod.sp_ApontarEtapa   TO papel_operador;
GRANT SELECT  ON OBJECT::prod.vw_PainelProducao TO papel_operador;

/* Qualidade: inspeciona, consulta rastreabilidade e trata nao conformidades. */
GRANT EXECUTE ON OBJECT::qual.sp_RegistrarInspecao TO papel_qualidade;
GRANT SELECT  ON SCHEMA::qual                      TO papel_qualidade;
GRANT SELECT  ON OBJECT::prod.vw_Rastreabilidade   TO papel_qualidade;
GRANT UPDATE  ON OBJECT::qual.NaoConformidade (causa, acao, situacao) TO papel_qualidade;

/* Gestao: leitura de tudo, menos o e-mail de contato dos clientes
   (minimizacao de dados pessoais, LGPD art. 6, III) e a auditoria. */
GRANT SELECT ON SCHEMA::cad  TO papel_gestor;
GRANT SELECT ON SCHEMA::prod TO papel_gestor;
GRANT SELECT ON SCHEMA::qual TO papel_gestor;
GRANT SELECT ON SCHEMA::com  TO papel_gestor;
DENY  SELECT ON OBJECT::cad.Cliente (email_contato) TO papel_gestor;
DENY  SELECT ON SCHEMA::seg TO papel_operador, papel_qualidade, papel_gestor;
GO

/* Usuarios sem login, usados para testar as permissoes com EXECUTE AS.
   Na empresa, cada perfil recebe os logins do Active Directory. */
CREATE USER u_operador  WITHOUT LOGIN;
CREATE USER u_qualidade WITHOUT LOGIN;
CREATE USER u_gestor    WITHOUT LOGIN;
ALTER ROLE papel_operador  ADD MEMBER u_operador;
ALTER ROLE papel_qualidade ADD MEMBER u_qualidade;
ALTER ROLE papel_gestor    ADD MEMBER u_gestor;
GO

/* ---------- Testes das permissoes e das regras ---------- */
SET NOCOUNT ON;
PRINT '--- T1: operador tenta ler clientes e apagar apontamento ---';
EXECUTE AS USER = 'u_operador';
BEGIN TRY
    SELECT TOP (1) razao_social FROM cad.Cliente;
END TRY
BEGIN CATCH
    PRINT CONCAT('Erro ', ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
BEGIN TRY
    DELETE FROM prod.Apontamento WHERE numero_op = 1;
END TRY
BEGIN CATCH
    PRINT CONCAT('Erro ', ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
REVERT;
GO

/* As chamadas com erro se desfazem sozinhas dentro da procedure. A
   chamada valida roda numa transacao externa desfeita no fim, para o
   teste nao alterar os dados. Atencao: o ROLLBACK de uma procedure
   desfaz tambem a transacao de quem a chamou, por isso as chamadas com
   erro ficam fora dela. */
PRINT '--- T2: operador aponta pela procedure ---';
EXECUTE AS USER = 'u_operador';
DECLARE @op INT, @etapa TINYINT, @pecas INT, @agora DATETIME2(0) = '2026-10-19T10:00:00';
SELECT TOP (1) @op = numero, @etapa = etapa_atual + 1, @pecas = qtd_boa
FROM prod.vw_PainelProducao
WHERE situacao = 'EM_PRODUCAO' AND etapa_atual = 1;
BEGIN TRY
    /* matricula 1003 so e habilitada na usinagem (etapa 3) */
    EXEC prod.sp_ApontarEtapa @op, @etapa, '1003', '2026-10-19T08:00:00', @agora, @pecas, 0;
END TRY
BEGIN CATCH
    PRINT CONCAT('OP ', @op, ' etapa ', @etapa, ' com a matricula 1003 -> erro ',
                 ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
BEGIN TRY
    DECLARE @boas INT = @pecas - 2;
    EXEC prod.sp_ApontarEtapa @op, @etapa, '1012', '2026-10-19T08:00:00', @agora, @boas, 2, NULL;
END TRY
BEGIN CATCH
    PRINT CONCAT('Refugo sem motivo -> erro ', ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
BEGIN TRANSACTION;
EXEC prod.sp_ApontarEtapa @op, @etapa, '1012', '2026-10-19T08:00:00', @agora, @boas, 2, 'falha de preenchimento na matriz';
PRINT CONCAT('OP ', @op, ': etapa ', @etapa, ' apontada pela procedure.');
SELECT numero, situacao, etapa_atual, qtd_boa FROM prod.vw_PainelProducao WHERE numero = @op;
ROLLBACK TRANSACTION;
REVERT;
GO

PRINT '--- T3: gestor le clientes, mas nao o e-mail de contato ---';
EXECUTE AS USER = 'u_gestor';
SELECT TOP (2) razao_social, cidade, uf FROM cad.Cliente ORDER BY razao_social;
BEGIN TRY
    SELECT TOP (1) email_contato FROM cad.Cliente;
END TRY
BEGIN CATCH
    PRINT CONCAT('Erro ', ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
REVERT;
GO

PRINT '--- T4: expedicao de lote nao aprovado (trigger) ---';
BEGIN TRANSACTION;
BEGIN TRY
    INSERT INTO com.Expedicao (nota_fiscal, id_cliente, data_emissao) VALUES (99999, 1, '2026-10-19');
    INSERT INTO com.ItemExpedicao (nota_fiscal, numero_op, quantidade)
    SELECT TOP (1) 99999, numero, 10 FROM prod.OrdemProducao WHERE situacao = 'EM_PRODUCAO';
END TRY
BEGIN CATCH
    PRINT CONCAT('Erro ', ERROR_NUMBER(), ': ', ERROR_MESSAGE());
END CATCH
IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION;
GO

PRINT '--- T5: alteracao de apontamento fica registrada na auditoria ---';
BEGIN TRANSACTION;
UPDATE prod.Apontamento SET minutos = minutos + 5 WHERE numero_op = 1 AND id_etapa = 5;
SELECT tabela, operacao, chave, usuario,
       JSON_VALUE(valores_antigos, '$.minutos') AS minutos_antes,
       JSON_VALUE(valores_novos, '$.minutos')   AS minutos_depois
FROM seg.Auditoria;
ROLLBACK TRANSACTION;
GO

/* ---------- Copias de seguranca ----------
   Politica proposta (agendada no SQL Server Agent):
   - completa aos domingos 02:00;
   - diferencial de segunda a sabado 02:00;
   - log a cada 15 minutos -> perda maxima de 15 minutos (RPO).
   As copias sao levadas para outro local (nuvem ou servidor remoto). */
EXEC master.dbo.xp_create_subdir N'/var/opt/mssql/backup';
GO
BACKUP DATABASE SIGPR TO DISK = N'/var/opt/mssql/backup/SIGPR_completo.bak'
    WITH INIT, COMPRESSION, CHECKSUM, NAME = N'SIGPR - completo';
BACKUP DATABASE SIGPR TO DISK = N'/var/opt/mssql/backup/SIGPR_diferencial.bak'
    WITH DIFFERENTIAL, INIT, COMPRESSION, CHECKSUM, NAME = N'SIGPR - diferencial';
BACKUP LOG SIGPR TO DISK = N'/var/opt/mssql/backup/SIGPR_log.trn'
    WITH INIT, COMPRESSION, CHECKSUM, NAME = N'SIGPR - log';
GO
RESTORE VERIFYONLY FROM DISK = N'/var/opt/mssql/backup/SIGPR_completo.bak' WITH CHECKSUM;
GO
SELECT TOP (3) bs.name,
       CASE bs.type WHEN 'D' THEN 'completo' WHEN 'I' THEN 'diferencial' WHEN 'L' THEN 'log' END AS tipo,
       CAST(bs.backup_size / 1048576.0 AS DECIMAL(8,2)) AS tamanho_mb,
       CAST(bs.compressed_backup_size / 1048576.0 AS DECIMAL(8,2)) AS comprimido_mb,
       bs.backup_finish_date
FROM msdb.dbo.backupset AS bs
WHERE bs.database_name = 'SIGPR'
ORDER BY bs.backup_finish_date DESC;
GO

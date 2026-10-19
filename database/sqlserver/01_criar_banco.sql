/* ============================================================
   SIGPR - Fujiy Instrumentais
   01 - Banco de dados e esquemas
   SQL Server 2022
   ============================================================ */
USE master;
GO

IF DB_ID('SIGPR') IS NOT NULL
BEGIN
    ALTER DATABASE SIGPR SET SINGLE_USER WITH ROLLBACK IMMEDIATE;
    DROP DATABASE SIGPR;
END
GO

CREATE DATABASE SIGPR COLLATE Latin1_General_100_CI_AI_SC_UTF8;
GO

/* Modelo de recuperacao completo: permite backup de log e restauracao
   ate um ponto no tempo (ver 08_seguranca_backup.sql). */
ALTER DATABASE SIGPR SET RECOVERY FULL;
GO

USE SIGPR;
GO

/* Um esquema por area de negocio. Alem de organizar, o esquema e a
   unidade de permissao: o perfil de operador recebe acesso a prod e
   nao enxerga com. */
CREATE SCHEMA cad;     -- cadastros
GO
CREATE SCHEMA prod;    -- producao e rastreabilidade
GO
CREATE SCHEMA qual;    -- qualidade
GO
CREATE SCHEMA com;     -- comercial e expedicao
GO
CREATE SCHEMA seg;     -- auditoria
GO
CREATE SCHEMA stg;     -- area de carga dos arquivos do terminal
GO

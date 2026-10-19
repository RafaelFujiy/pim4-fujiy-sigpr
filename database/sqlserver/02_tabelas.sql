/* ============================================================
   SIGPR - Fujiy Instrumentais
   02 - Tabelas (modelo fisico em 3FN)
   ============================================================ */
USE SIGPR;
GO

/* ---------------- cad: cadastros ---------------- */

CREATE TABLE cad.Linha (
    id_linha      TINYINT      NOT NULL IDENTITY(1,1),
    nome          VARCHAR(20)  NOT NULL,
    CONSTRAINT PK_Linha PRIMARY KEY (id_linha),
    CONSTRAINT UQ_Linha_nome UNIQUE (nome)
);

CREATE TABLE cad.Familia (
    sigla         CHAR(3)      NOT NULL,
    nome          VARCHAR(40)  NOT NULL,
    id_linha      TINYINT      NOT NULL,
    CONSTRAINT PK_Familia PRIMARY KEY (sigla),
    CONSTRAINT FK_Familia_Linha FOREIGN KEY (id_linha) REFERENCES cad.Linha (id_linha)
);

CREATE TABLE cad.Produto (
    id_produto          INT          NOT NULL IDENTITY(1,1),
    codigo              VARCHAR(12)  NOT NULL,
    descricao           VARCHAR(80)  NOT NULL,
    sigla_familia       CHAR(3)      NOT NULL,
    aco                 VARCHAR(10)  NOT NULL,
    classe_risco        TINYINT      NOT NULL,
    notificacao_anvisa  VARCHAR(20)  NULL,
    preco_unitario      DECIMAL(10,2) NOT NULL,
    ativo               BIT          NOT NULL CONSTRAINT DF_Produto_ativo DEFAULT (1),
    CONSTRAINT PK_Produto PRIMARY KEY (id_produto),
    CONSTRAINT UQ_Produto_codigo UNIQUE (codigo),
    CONSTRAINT FK_Produto_Familia FOREIGN KEY (sigla_familia) REFERENCES cad.Familia (sigla),
    CONSTRAINT CK_Produto_aco CHECK (aco IN ('AISI420', 'AISI440', 'AISI304')),
    CONSTRAINT CK_Produto_classe CHECK (classe_risco BETWEEN 1 AND 4),
    CONSTRAINT CK_Produto_preco CHECK (preco_unitario > 0)
);

CREATE TABLE cad.Etapa (
    id_etapa               TINYINT      NOT NULL,
    nome                   VARCHAR(40)  NOT NULL,
    tempo_padrao_min_peca  DECIMAL(5,2) NOT NULL,
    CONSTRAINT PK_Etapa PRIMARY KEY (id_etapa),
    CONSTRAINT UQ_Etapa_nome UNIQUE (nome),
    CONSTRAINT CK_Etapa_id CHECK (id_etapa BETWEEN 1 AND 8)
);

CREATE TABLE cad.Operador (
    matricula      VARCHAR(8)   NOT NULL,
    nome           VARCHAR(60)  NOT NULL,
    data_admissao  DATE         NOT NULL,
    ativo          BIT          NOT NULL CONSTRAINT DF_Operador_ativo DEFAULT (1),
    CONSTRAINT PK_Operador PRIMARY KEY (matricula)
);

/* Relacionamento N:N - um operador pode estar habilitado em varias
   etapas e cada etapa tem varios operadores habilitados. */
CREATE TABLE cad.OperadorEtapa (
    matricula         VARCHAR(8)  NOT NULL,
    id_etapa          TINYINT     NOT NULL,
    data_habilitacao  DATE        NOT NULL,
    CONSTRAINT PK_OperadorEtapa PRIMARY KEY (matricula, id_etapa),
    CONSTRAINT FK_OperadorEtapa_Operador FOREIGN KEY (matricula) REFERENCES cad.Operador (matricula),
    CONSTRAINT FK_OperadorEtapa_Etapa FOREIGN KEY (id_etapa) REFERENCES cad.Etapa (id_etapa)
);

CREATE TABLE cad.Fornecedor (
    id_fornecedor  INT          NOT NULL IDENTITY(1,1),
    cnpj           CHAR(14)     NOT NULL,
    razao_social   VARCHAR(80)  NOT NULL,
    CONSTRAINT PK_Fornecedor PRIMARY KEY (id_fornecedor),
    CONSTRAINT UQ_Fornecedor_cnpj UNIQUE (cnpj),
    CONSTRAINT CK_Fornecedor_cnpj CHECK (cnpj NOT LIKE '%[^0-9]%')
);

CREATE TABLE cad.Cliente (
    id_cliente     INT          NOT NULL IDENTITY(1,1),
    cnpj           CHAR(14)     NOT NULL,
    razao_social   VARCHAR(80)  NOT NULL,
    tipo           VARCHAR(12)  NOT NULL,
    cidade         VARCHAR(60)  NOT NULL,
    uf             CHAR(2)      NOT NULL,
    email_contato  VARCHAR(80)  NULL,
    CONSTRAINT PK_Cliente PRIMARY KEY (id_cliente),
    CONSTRAINT UQ_Cliente_cnpj UNIQUE (cnpj),
    CONSTRAINT CK_Cliente_cnpj CHECK (cnpj NOT LIKE '%[^0-9]%'),
    CONSTRAINT CK_Cliente_tipo CHECK (tipo IN ('DISTRIBUIDOR', 'HOSPITAL', 'CLINICA'))
);

/* ---------------- com: comercial ---------------- */

CREATE TABLE com.Pedido (
    id_pedido      INT          NOT NULL IDENTITY(1,1),
    id_cliente     INT          NOT NULL,
    data_pedido    DATE         NOT NULL,
    pedido_cliente VARCHAR(20)  NULL,
    CONSTRAINT PK_Pedido PRIMARY KEY (id_pedido),
    CONSTRAINT FK_Pedido_Cliente FOREIGN KEY (id_cliente) REFERENCES cad.Cliente (id_cliente)
);

CREATE TABLE com.ItemPedido (
    id_item         INT           NOT NULL IDENTITY(1,1),
    id_pedido       INT           NOT NULL,
    id_produto      INT           NOT NULL,
    quantidade      INT           NOT NULL,
    preco_unitario  DECIMAL(10,2) NOT NULL,
    data_entrega    DATE          NOT NULL,
    CONSTRAINT PK_ItemPedido PRIMARY KEY (id_item),
    CONSTRAINT FK_ItemPedido_Pedido FOREIGN KEY (id_pedido) REFERENCES com.Pedido (id_pedido)
        ON DELETE CASCADE,
    CONSTRAINT FK_ItemPedido_Produto FOREIGN KEY (id_produto) REFERENCES cad.Produto (id_produto),
    CONSTRAINT CK_ItemPedido_qtd CHECK (quantidade > 0),
    CONSTRAINT CK_ItemPedido_preco CHECK (preco_unitario > 0)
);

/* ---------------- prod: producao ---------------- */

CREATE TABLE prod.LoteMateriaPrima (
    codigo            VARCHAR(16)  NOT NULL,
    id_fornecedor     INT          NOT NULL,
    aco               VARCHAR(10)  NOT NULL,
    corrida           VARCHAR(16)  NOT NULL,
    peso_kg           DECIMAL(9,2) NOT NULL,
    saldo_kg          DECIMAL(9,2) NOT NULL,
    data_recebimento  DATE         NOT NULL,
    CONSTRAINT PK_LoteMateriaPrima PRIMARY KEY (codigo),
    CONSTRAINT FK_LoteMP_Fornecedor FOREIGN KEY (id_fornecedor) REFERENCES cad.Fornecedor (id_fornecedor),
    CONSTRAINT CK_LoteMP_aco CHECK (aco IN ('AISI420', 'AISI440', 'AISI304')),
    CONSTRAINT CK_LoteMP_peso CHECK (peso_kg > 0),
    CONSTRAINT CK_LoteMP_saldo CHECK (saldo_kg >= 0 AND saldo_kg <= peso_kg)
);

CREATE TABLE prod.OrdemProducao (
    numero          INT          NOT NULL,
    id_produto      INT          NOT NULL,
    id_item_pedido  INT          NULL,
    quantidade      INT          NOT NULL,
    data_abertura   DATE         NOT NULL,
    data_entrega    DATE         NOT NULL,
    situacao        VARCHAR(12)  NOT NULL CONSTRAINT DF_OP_situacao DEFAULT ('ABERTA'),
    etapa_atual     TINYINT      NOT NULL CONSTRAINT DF_OP_etapa DEFAULT (0),
    qtd_boa         INT          NOT NULL,
    qtd_refugo      INT          NOT NULL CONSTRAINT DF_OP_refugo DEFAULT (0),
    lote            VARCHAR(12)  NOT NULL,
    CONSTRAINT PK_OrdemProducao PRIMARY KEY (numero),
    CONSTRAINT UQ_OrdemProducao_lote UNIQUE (lote),
    CONSTRAINT FK_OP_Produto FOREIGN KEY (id_produto)
        REFERENCES cad.Produto (id_produto),
    CONSTRAINT FK_OP_ItemPedido FOREIGN KEY (id_item_pedido)
        REFERENCES com.ItemPedido (id_item),
    CONSTRAINT CK_OP_quantidade CHECK (quantidade > 0),
    CONSTRAINT CK_OP_datas CHECK (data_entrega >= data_abertura),
    CONSTRAINT CK_OP_situacao CHECK (situacao IN
        ('ABERTA', 'EM_PRODUCAO', 'EM_INSPECAO', 'APROVADA', 'REPROVADA', 'EXPEDIDA')),
    CONSTRAINT CK_OP_etapa CHECK (etapa_atual BETWEEN 0 AND 8),
    CONSTRAINT CK_OP_pecas CHECK (qtd_boa >= 0 AND qtd_refugo >= 0
                                  AND qtd_boa + qtd_refugo = quantidade)
);

/* N:N entre OP e lote de materia-prima: e o elo da rastreabilidade */
CREATE TABLE prod.ConsumoMateriaPrima (
    numero_op       INT          NOT NULL,
    codigo_lote_mp  VARCHAR(16)  NOT NULL,
    peso_kg         DECIMAL(9,2) NOT NULL,
    CONSTRAINT PK_ConsumoMP PRIMARY KEY (numero_op, codigo_lote_mp),
    CONSTRAINT FK_ConsumoMP_OP FOREIGN KEY (numero_op) REFERENCES prod.OrdemProducao (numero),
    CONSTRAINT FK_ConsumoMP_Lote FOREIGN KEY (codigo_lote_mp) REFERENCES prod.LoteMateriaPrima (codigo),
    CONSTRAINT CK_ConsumoMP_peso CHECK (peso_kg > 0)
);

CREATE TABLE prod.Apontamento (
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
    CONSTRAINT PK_Apontamento PRIMARY KEY (id_apontamento),
    CONSTRAINT UQ_Apontamento_op_etapa UNIQUE (numero_op, id_etapa),
    CONSTRAINT FK_Apontamento_OP FOREIGN KEY (numero_op) REFERENCES prod.OrdemProducao (numero),
    CONSTRAINT FK_Apontamento_Etapa FOREIGN KEY (id_etapa) REFERENCES cad.Etapa (id_etapa),
    CONSTRAINT FK_Apontamento_Operador FOREIGN KEY (matricula) REFERENCES cad.Operador (matricula),
    CONSTRAINT CK_Apontamento_periodo CHECK (fim > inicio),
    CONSTRAINT CK_Apontamento_minutos CHECK (minutos > 0),
    CONSTRAINT CK_Apontamento_pecas CHECK (qtd_boa >= 0 AND qtd_refugo >= 0),
    CONSTRAINT CK_Apontamento_motivo CHECK (qtd_refugo = 0 OR motivo_refugo IS NOT NULL)
);

/* ---------------- qual: qualidade ---------------- */

CREATE TABLE qual.Inspecao (
    id_inspecao         INT         NOT NULL IDENTITY(1,1),
    numero_op           INT         NOT NULL,
    data_inspecao       DATE        NOT NULL,
    tamanho_lote        INT         NOT NULL,
    letra_codigo        CHAR(1)     NOT NULL,
    amostra             INT         NOT NULL,
    aceitacao           INT         NOT NULL,
    rejeicao            INT         NOT NULL,
    defeitos            INT         NOT NULL,
    resultado           VARCHAR(9)  NOT NULL,
    matricula_inspetor  VARCHAR(8)  NOT NULL,
    CONSTRAINT PK_Inspecao PRIMARY KEY (id_inspecao),
    CONSTRAINT FK_Inspecao_OP FOREIGN KEY (numero_op) REFERENCES prod.OrdemProducao (numero),
    CONSTRAINT FK_Inspecao_Operador FOREIGN KEY (matricula_inspetor) REFERENCES cad.Operador (matricula),
    CONSTRAINT CK_Inspecao_amostra CHECK (amostra > 0 AND amostra <= tamanho_lote),
    CONSTRAINT CK_Inspecao_defeitos CHECK (defeitos BETWEEN 0 AND amostra),
    CONSTRAINT CK_Inspecao_criterio CHECK (rejeicao = aceitacao + 1),
    CONSTRAINT CK_Inspecao_resultado CHECK (resultado IN ('APROVADO', 'REPROVADO'))
);

CREATE TABLE qual.NaoConformidade (
    id_nc          INT           NOT NULL IDENTITY(1,1),
    numero_op      INT           NOT NULL,
    id_inspecao    INT           NULL,
    data_abertura  DATE          NOT NULL,
    descricao      VARCHAR(200)  NOT NULL,
    causa          VARCHAR(200)  NULL,
    acao           VARCHAR(200)  NULL,
    situacao       VARCHAR(10)   NOT NULL CONSTRAINT DF_NC_situacao DEFAULT ('ABERTA'),
    CONSTRAINT PK_NaoConformidade PRIMARY KEY (id_nc),
    CONSTRAINT FK_NC_OP FOREIGN KEY (numero_op) REFERENCES prod.OrdemProducao (numero),
    CONSTRAINT FK_NC_Inspecao FOREIGN KEY (id_inspecao) REFERENCES qual.Inspecao (id_inspecao),
    CONSTRAINT CK_NC_situacao CHECK (situacao IN ('ABERTA', 'EM_ANALISE', 'ENCERRADA'))
);

/* ---------------- com: expedicao ---------------- */

CREATE TABLE com.Expedicao (
    nota_fiscal   INT   NOT NULL,
    id_cliente    INT   NOT NULL,
    data_emissao  DATE  NOT NULL,
    CONSTRAINT PK_Expedicao PRIMARY KEY (nota_fiscal),
    CONSTRAINT FK_Expedicao_Cliente FOREIGN KEY (id_cliente) REFERENCES cad.Cliente (id_cliente)
);

CREATE TABLE com.ItemExpedicao (
    nota_fiscal  INT  NOT NULL,
    numero_op    INT  NOT NULL,
    quantidade   INT  NOT NULL,
    CONSTRAINT PK_ItemExpedicao PRIMARY KEY (nota_fiscal, numero_op),
    CONSTRAINT FK_ItemExpedicao_Expedicao FOREIGN KEY (nota_fiscal) REFERENCES com.Expedicao (nota_fiscal),
    CONSTRAINT FK_ItemExpedicao_OP FOREIGN KEY (numero_op) REFERENCES prod.OrdemProducao (numero),
    CONSTRAINT CK_ItemExpedicao_qtd CHECK (quantidade > 0)
);

/* ---------------- seg: auditoria ---------------- */

CREATE TABLE seg.Auditoria (
    id_auditoria     BIGINT         NOT NULL IDENTITY(1,1),
    tabela           SYSNAME        NOT NULL,
    operacao         CHAR(1)        NOT NULL,
    chave            VARCHAR(60)    NOT NULL,
    usuario          SYSNAME        NOT NULL CONSTRAINT DF_Auditoria_usuario DEFAULT (SUSER_SNAME()),
    data_hora        DATETIME2(0)   NOT NULL CONSTRAINT DF_Auditoria_data DEFAULT (SYSDATETIME()),
    valores_antigos  NVARCHAR(MAX)  NULL,
    valores_novos    NVARCHAR(MAX)  NULL,
    CONSTRAINT PK_Auditoria PRIMARY KEY (id_auditoria),
    CONSTRAINT CK_Auditoria_operacao CHECK (operacao IN ('I', 'U', 'D'))
);
GO

/* ============================================================
   SIGPR - Fujiy Instrumentais
   03 - Dados de cadastro e pedidos (dados ficticios)
   ============================================================ */
USE SIGPR;
GO

INSERT INTO cad.Linha (nome) VALUES ('ODONTO'), ('CIRURGICO');

INSERT INTO cad.Familia (sigla, nome, id_linha)
SELECT f.sigla, f.nome, l.id_linha
FROM (VALUES
    ('CUR', 'Cureta periodontal', 'ODONTO'),
    ('SON', 'Sonda exploradora', 'ODONTO'),
    ('ESP', 'Espátula', 'ODONTO'),
    ('ESC', 'Escavador de dentina', 'ODONTO'),
    ('PIN', 'Pinça', 'CIRURGICO'),
    ('TES', 'Tesoura', 'CIRURGICO'),
    ('PAG', 'Porta-agulhas', 'CIRURGICO')
) AS f (sigla, nome, linha)
JOIN cad.Linha AS l ON l.nome = f.linha;

INSERT INTO cad.Produto (codigo, descricao, sigla_familia, aco, classe_risco, notificacao_anvisa, preco_unitario) VALUES
    ('OD-CUR-001', 'Cureta periodontal Gracey 5/6', 'CUR', 'AISI420', 1, 'FIC-000001', 64.90),
    ('OD-CUR-002', 'Cureta periodontal McCall 13/14', 'CUR', 'AISI420', 1, 'FIC-000002', 64.90),
    ('OD-SON-003', 'Sonda exploradora nº 5', 'SON', 'AISI420', 1, 'FIC-000003', 38.50),
    ('OD-ESP-004', 'Espátula para resina nº 1', 'ESP', 'AISI420', 1, 'FIC-000004', 41.20),
    ('OD-ESC-005', 'Escavador de dentina nº 17', 'ESC', 'AISI420', 1, 'FIC-000005', 45.80),
    ('CI-PIN-006', 'Pinça anatômica dente de rato 14 cm', 'PIN', 'AISI420', 1, 'FIC-000006', 52.30),
    ('CI-PIN-007', 'Pinça hemostática Kelly reta 14 cm', 'PIN', 'AISI420', 1, 'FIC-000007', 56.10),
    ('CI-TES-008', 'Tesoura Metzenbaum curva 15 cm', 'TES', 'AISI440', 1, 'FIC-000008', 89.90),
    ('CI-TES-009', 'Tesoura íris reta 11,5 cm', 'TES', 'AISI440', 1, 'FIC-000009', 74.50),
    ('CI-PAG-010', 'Porta-agulhas Mayo-Hegar 14 cm', 'PAG', 'AISI420', 1, 'FIC-000010', 98.40);

INSERT INTO cad.Etapa (id_etapa, nome, tempo_padrao_min_peca) VALUES
    (1, 'Corte da barra', 0.40),
    (2, 'Forjamento', 0.90),
    (3, 'Usinagem', 1.60),
    (4, 'Tratamento térmico', 0.70),
    (5, 'Retífica e afiação', 2.10),
    (6, 'Polimento e passivação', 1.20),
    (7, 'Gravação a laser', 0.25),
    (8, 'Inspeção final e embalagem', 0.50);

INSERT INTO cad.Operador (matricula, nome, data_admissao) VALUES
    ('1001', 'Carlos Eduardo Lima', '2015-03-02'),
    ('1002', 'Márcio Souza', '2018-08-13'),
    ('1003', 'Ana Paula Ribeiro', '2019-02-04'),
    ('1004', 'Renato Alves', '2021-06-07'),
    ('1005', 'Juliana Prado', '2016-11-21'),
    ('1006', 'Sérgio Matos', '2012-05-14'),
    ('1007', 'Patrícia Nunes', '2020-01-20'),
    ('1008', 'Fábio Teixeira', '2017-09-11'),
    ('1009', 'Luciana Freitas', '2014-04-07'),
    ('1010', 'Roberta Campos', '2013-10-01'),
    ('1011', 'Daniel Moreira', '2022-03-14'),
    ('1012', 'Tiago Barbosa', '2011-07-18');

INSERT INTO cad.OperadorEtapa (matricula, id_etapa, data_habilitacao) VALUES
    ('1001', 1, '2023-01-16'),
    ('1001', 2, '2023-01-16'),
    ('1002', 1, '2023-01-16'),
    ('1002', 2, '2023-01-16'),
    ('1003', 3, '2023-01-16'),
    ('1004', 3, '2023-01-16'),
    ('1005', 4, '2023-01-16'),
    ('1006', 5, '2023-01-16'),
    ('1007', 5, '2023-01-16'),
    ('1008', 6, '2023-01-16'),
    ('1009', 6, '2023-01-16'),
    ('1009', 7, '2023-01-16'),
    ('1010', 8, '2023-01-16'),
    ('1011', 8, '2022-09-01'),
    ('1012', 1, '2023-01-16'),
    ('1012', 2, '2023-01-16'),
    ('1012', 3, '2023-01-16'),
    ('1012', 4, '2023-01-16'),
    ('1012', 5, '2023-01-16'),
    ('1012', 6, '2023-01-16'),
    ('1012', 7, '2023-01-16');

INSERT INTO cad.Fornecedor (cnpj, razao_social) VALUES
    ('43218706000137', 'Aços Especiais Paulista Ltda.'),
    ('07594412000146', 'Inoxbarra Distribuidora de Aços Ltda.'),
    ('61835097000147', 'Metalúrgica Rio Pardo Comércio de Aços Ltda.');

INSERT INTO cad.Cliente (cnpj, razao_social, tipo, cidade, uf, email_contato) VALUES
    ('34547855000159', 'Ipê Roxo Distribuidora Odontológica Ltda.', 'DISTRIBUIDOR', 'Ribeirão Preto', 'SP', 'compras@iperoxo.example'),
    ('94602776000125', 'Horizonte Azul Produtos Cirúrgicos Ltda.', 'DISTRIBUIDOR', 'Belo Horizonte', 'MG', 'suprimentos@horizonteazul.example'),
    ('20989580000107', 'Dental Três Colinas Comércio Ltda.', 'DISTRIBUIDOR', 'Franca', 'SP', 'compras@trescolinas.example'),
    ('91641545000132', 'Medcentro Pardo Suprimentos Hospitalares Ltda.', 'HOSPITAL', 'Uberaba', 'MG', 'almoxarifado@medcentropardo.example'),
    ('42932437000103', 'Rede Odonto Mantiqueira Ltda.', 'CLINICA', 'Poços de Caldas', 'MG', 'compras@odontomantiqueira.example'),
    ('68507731000161', 'Cirúrgica Vale do Sapucaí Ltda.', 'DISTRIBUIDOR', 'Pouso Alegre', 'MG', 'pedidos@valedosapucai.example');
GO

/* Pedidos de venda. Na carga de exemplo cada OP atende ao item de
   pedido de mesmo numero (id_item = numero da OP). */
SET IDENTITY_INSERT com.Pedido ON;
INSERT INTO com.Pedido (id_pedido, id_cliente, data_pedido, pedido_cliente) VALUES
    (1, 3, '2026-06-29', 'PC-4101'),
    (2, 4, '2026-07-01', 'PC-4102'),
    (3, 4, '2026-07-04', 'PC-4103'),
    (4, 3, '2026-07-07', 'PC-4104'),
    (5, 2, '2026-07-08', 'PC-4105'),
    (6, 4, '2026-07-11', 'PC-4106'),
    (7, 5, '2026-07-14', 'PC-4107'),
    (8, 4, '2026-07-18', 'PC-4108'),
    (9, 2, '2026-07-19', 'PC-4109'),
    (10, 4, '2026-07-20', 'PC-4110'),
    (11, 3, '2026-07-23', 'PC-4111'),
    (12, 2, '2026-07-25', 'PC-4112'),
    (13, 5, '2026-07-28', 'PC-4113'),
    (14, 5, '2026-07-30', 'PC-4114'),
    (15, 4, '2026-08-02', 'PC-4115'),
    (16, 2, '2026-08-05', 'PC-4116'),
    (17, 1, '2026-08-08', 'PC-4117'),
    (18, 5, '2026-08-10', 'PC-4118'),
    (19, 6, '2026-08-13', 'PC-4119'),
    (20, 3, '2026-08-15', 'PC-4120'),
    (21, 1, '2026-08-16', 'PC-4121'),
    (22, 6, '2026-08-19', 'PC-4122'),
    (23, 4, '2026-08-22', 'PC-4123'),
    (24, 1, '2026-08-25', 'PC-4124'),
    (25, 5, '2026-08-27', 'PC-4125'),
    (26, 4, '2026-08-29', 'PC-4126'),
    (27, 1, '2026-09-01', 'PC-4127'),
    (28, 3, '2026-09-03', 'PC-4128'),
    (29, 2, '2026-09-05', 'PC-4129'),
    (30, 2, '2026-09-08', 'PC-4130'),
    (31, 1, '2026-09-12', 'PC-4131'),
    (32, 6, '2026-09-12', 'PC-4132'),
    (33, 2, '2026-09-16', 'PC-4133'),
    (34, 1, '2026-09-17', 'PC-4134'),
    (35, 4, '2026-09-20', 'PC-4135'),
    (36, 2, '2026-09-23', 'PC-4136'),
    (37, 3, '2026-09-24', 'PC-4137'),
    (38, 4, '2026-09-28', 'PC-4138'),
    (39, 2, '2026-10-01', 'PC-4139'),
    (40, 3, '2026-10-03', 'PC-4140'),
    (41, 5, '2026-10-06', 'PC-4141'),
    (42, 2, '2026-10-07', 'PC-4142'),
    (43, 4, '2026-10-10', 'PC-4143'),
    (44, 5, '2026-10-11', 'PC-4144'),
    (45, 2, '2026-10-14', 'PC-4145'),
    (46, 4, '2026-10-14', 'PC-4146'),
    (47, 6, '2026-10-15', 'PC-4147'),
    (48, 1, '2026-10-15', 'PC-4148'),
    (49, 4, '2026-10-17', 'PC-4149'),
    (50, 5, '2026-10-17', 'PC-4150'),
    (51, 2, '2026-10-17', 'PC-4151');
SET IDENTITY_INSERT com.Pedido OFF;

SET IDENTITY_INSERT com.ItemPedido ON;
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (1, 1, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 200, 38.50, '2026-07-22');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (2, 2, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-009'), 250, 74.50, '2026-07-25');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (3, 3, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 450, 52.30, '2026-07-28');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (4, 4, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESP-004'), 600, 41.20, '2026-07-26');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (5, 5, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 350, 98.40, '2026-08-01');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (6, 6, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 250, 89.90, '2026-07-26');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (7, 7, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 250, 38.50, '2026-08-06');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (8, 8, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 150, 98.40, '2026-08-07');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (9, 9, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 150, 89.90, '2026-08-07');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (10, 10, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 300, 52.30, '2026-08-04');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (11, 11, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 250, 64.90, '2026-08-12');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (12, 12, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 300, 98.40, '2026-08-15');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (13, 13, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESC-005'), 450, 45.80, '2026-08-13');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (14, 14, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 200, 38.50, '2026-08-15');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (15, 15, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 450, 98.40, '2026-08-16');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (16, 16, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 250, 89.90, '2026-08-29');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (17, 17, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 400, 38.50, '2026-08-31');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (18, 18, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 350, 64.90, '2026-08-26');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (19, 19, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-007'), 350, 56.10, '2026-09-03');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (20, 20, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESC-005'), 600, 45.80, '2026-09-03');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (21, 21, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 200, 64.90, '2026-09-09');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (22, 22, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 550, 89.90, '2026-09-12');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (23, 23, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-007'), 300, 56.10, '2026-09-04');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (24, 24, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 150, 38.50, '2026-09-07');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (25, 25, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-001'), 600, 64.90, '2026-09-11');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (26, 26, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 300, 52.30, '2026-09-16');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (27, 27, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESC-005'), 300, 45.80, '2026-09-21');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (28, 28, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-001'), 450, 64.90, '2026-09-22');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (29, 29, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-007'), 400, 56.10, '2026-09-27');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (30, 30, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 500, 52.30, '2026-09-21');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (31, 31, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 550, 64.90, '2026-10-01');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (32, 32, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 350, 98.40, '2026-09-26');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (33, 33, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-007'), 500, 56.10, '2026-10-04');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (34, 34, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 500, 64.90, '2026-10-04');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (35, 35, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 150, 98.40, '2026-10-12');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (36, 36, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 200, 89.90, '2026-10-10');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (37, 37, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESC-005'), 250, 45.80, '2026-10-16');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (38, 38, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 600, 98.40, '2026-10-13');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (39, 39, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-007'), 150, 56.10, '2026-10-16');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (40, 40, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-ESP-004'), 600, 41.20, '2026-10-19');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (41, 41, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 150, 64.90, '2026-10-27');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (42, 42, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 600, 98.40, '2026-10-22');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (43, 43, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 300, 52.30, '2026-10-28');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (44, 44, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 400, 64.90, '2026-10-31');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (45, 45, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 550, 98.40, '2026-11-05');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (46, 46, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-008'), 500, 89.90, '2026-11-07');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (47, 47, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-TES-009'), 300, 74.50, '2026-11-13');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (48, 48, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-CUR-002'), 450, 64.90, '2026-10-30');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (49, 49, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PAG-010'), 200, 98.40, '2026-10-28');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (50, 50, (SELECT id_produto FROM cad.Produto WHERE codigo = 'OD-SON-003'), 500, 38.50, '2026-11-20');
INSERT INTO com.ItemPedido (id_item, id_pedido, id_produto, quantidade, preco_unitario, data_entrega) VALUES
    (51, 51, (SELECT id_produto FROM cad.Produto WHERE codigo = 'CI-PIN-006'), 350, 52.30, '2026-10-26');
SET IDENTITY_INSERT com.ItemPedido OFF;
GO

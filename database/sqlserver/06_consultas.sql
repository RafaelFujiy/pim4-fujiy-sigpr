/* ============================================================
   SIGPR - Fujiy Instrumentais
   06 - Consultas de apoio a decisao (PCP, Qualidade, Diretoria)
   ============================================================ */
USE SIGPR;
GO

/* C1 - Recall: clientes que receberam pecas feitas com a barra
   suspeita MP-2608-003 (rastreabilidade para frente). */
SELECT r.lote_mp, r.lote_produto, r.produto, r.situacao,
       r.nota_fiscal, r.data_expedicao, r.cliente, r.cnpj_cliente, r.pecas_expedidas
FROM prod.vw_Rastreabilidade AS r
WHERE r.lote_mp = 'MP-2608-003'
ORDER BY r.lote_produto, r.nota_fiscal;
GO

/* C2 - Gargalo: minutos por peca em cada etapa, comparados ao tempo
   padrao, e capacidade diaria (960 minutos em dois turnos). */
SELECT e.id_etapa,
       e.nome AS etapa,
       CAST(SUM(a.minutos) * 1.0 / SUM(a.qtd_boa + a.qtd_refugo) AS DECIMAL(6,2)) AS min_por_peca,
       e.tempo_padrao_min_peca AS padrao,
       CAST(960.0 / (SUM(a.minutos) * 1.0 / SUM(a.qtd_boa + a.qtd_refugo)) AS INT) AS capacidade_dia
FROM prod.Apontamento AS a
JOIN cad.Etapa AS e ON e.id_etapa = a.id_etapa
GROUP BY e.id_etapa, e.nome, e.tempo_padrao_min_peca
ORDER BY min_por_peca DESC;
GO

/* C3 - Refugo por etapa e mes a partir da view indexada (sem varrer
   prod.Apontamento) e participacao de cada etapa no refugo do mes. */
SELECT v.ano, v.mes, e.nome AS etapa, v.refugo,
       CAST(100.0 * v.refugo / v.pecas_processadas AS DECIMAL(5,2)) AS perc_refugo,
       CAST(100.0 * v.refugo / SUM(v.refugo) OVER (PARTITION BY v.ano, v.mes) AS DECIMAL(5,1))
           AS perc_do_refugo_do_mes
FROM prod.vw_RefugoEtapaMes AS v WITH (NOEXPAND)
JOIN cad.Etapa AS e ON e.id_etapa = v.id_etapa
WHERE v.ano = 2026 AND v.mes = 10
ORDER BY v.refugo DESC;
GO

/* C4 - Entregas no prazo (OTD) por cliente. A CTE acha a data da
   ultima nota de cada OP; depois compara com a data prometida. */
WITH entrega_op AS (
    SELECT ie.numero_op, MAX(e.data_emissao) AS ultima_nf,
           MAX(e.id_cliente) AS id_cliente
    FROM com.ItemExpedicao AS ie
    JOIN com.Expedicao AS e ON e.nota_fiscal = ie.nota_fiscal
    GROUP BY ie.numero_op
)
SELECT c.razao_social,
       COUNT(*) AS ops_entregues,
       SUM(CASE WHEN eo.ultima_nf <= op.data_entrega THEN 1 ELSE 0 END) AS no_prazo,
       CAST(100.0 * SUM(CASE WHEN eo.ultima_nf <= op.data_entrega THEN 1 ELSE 0 END)
            / COUNT(*) AS DECIMAL(5,1)) AS otd_perc,
       MAX(CASE WHEN eo.ultima_nf > op.data_entrega
                THEN DATEDIFF(DAY, op.data_entrega, eo.ultima_nf) ELSE 0 END)
           AS maior_atraso_dias
FROM entrega_op AS eo
JOIN prod.OrdemProducao AS op ON op.numero = eo.numero_op
JOIN cad.Cliente AS c ON c.id_cliente = eo.id_cliente
WHERE op.situacao = 'EXPEDIDA'
GROUP BY c.razao_social
ORDER BY otd_perc;
GO

/* C5 - Ranking de produtividade (pecas por hora) dos operadores em
   cada etapa, com DENSE_RANK por etapa. */
SELECT e.nome AS etapa, o.nome AS operador,
       SUM(a.qtd_boa + a.qtd_refugo) AS pecas,
       CAST(60.0 * SUM(a.qtd_boa + a.qtd_refugo) / SUM(a.minutos) AS DECIMAL(6,1)) AS pecas_hora,
       DENSE_RANK() OVER (PARTITION BY a.id_etapa
                          ORDER BY 1.0 * SUM(a.qtd_boa + a.qtd_refugo) / SUM(a.minutos) DESC) AS posicao
FROM prod.Apontamento AS a
JOIN cad.Etapa AS e    ON e.id_etapa = a.id_etapa
JOIN cad.Operador AS o ON o.matricula = a.matricula
WHERE a.id_etapa IN (2, 5)
GROUP BY a.id_etapa, e.nome, o.nome
ORDER BY a.id_etapa, posicao;
GO

/* C6 - Subconsulta correlacionada: OPs com refugo 25% acima da media
   do proprio produto (candidatas a analise de causa). */
SELECT op.numero, op.lote, p.codigo AS produto, op.quantidade, op.qtd_refugo,
       CAST(100.0 * op.qtd_refugo / op.quantidade AS DECIMAL(5,2)) AS perc_refugo
FROM prod.OrdemProducao AS op
JOIN cad.Produto AS p ON p.id_produto = op.id_produto
WHERE op.etapa_atual >= 7
  AND 1.0 * op.qtd_refugo / op.quantidade >
      (SELECT 1.25 * AVG(1.0 * o2.qtd_refugo / o2.quantidade)
       FROM prod.OrdemProducao AS o2
       WHERE o2.id_produto = op.id_produto AND o2.etapa_atual >= 7)
ORDER BY perc_refugo DESC;
GO

/* C7 - Faturamento expedido por mes e acumulado no ano (SUM OVER). */
SELECT YEAR(e.data_emissao) AS ano, MONTH(e.data_emissao) AS mes,
       SUM(ie.quantidade * ip.preco_unitario) AS faturamento,
       SUM(SUM(ie.quantidade * ip.preco_unitario))
           OVER (PARTITION BY YEAR(e.data_emissao) ORDER BY MONTH(e.data_emissao)) AS acumulado
FROM com.ItemExpedicao AS ie
JOIN com.Expedicao AS e        ON e.nota_fiscal = ie.nota_fiscal
JOIN prod.OrdemProducao AS op  ON op.numero = ie.numero_op
JOIN com.ItemPedido AS ip      ON ip.id_item = op.id_item_pedido
GROUP BY YEAR(e.data_emissao), MONTH(e.data_emissao)
ORDER BY ano, mes;
GO

/* C8 - Lotes de materia-prima com saldo e quantas OPs ja usaram cada
   um (LEFT JOIN: aparece mesmo o lote ainda nao usado). */
SELECT mp.codigo, mp.aco, f.razao_social AS fornecedor, mp.saldo_kg,
       COUNT(c.numero_op) AS ops_que_usaram,
       COALESCE(SUM(c.peso_kg), 0) AS consumido_kg
FROM prod.LoteMateriaPrima AS mp
JOIN cad.Fornecedor AS f ON f.id_fornecedor = mp.id_fornecedor
LEFT JOIN prod.ConsumoMateriaPrima AS c ON c.codigo_lote_mp = mp.codigo
GROUP BY mp.codigo, mp.aco, f.razao_social, mp.saldo_kg
ORDER BY mp.codigo;
GO

/* C9 - Produtos do catalogo sem nenhuma OP em outubro (EXCEPT). */
SELECT codigo FROM cad.Produto
EXCEPT
SELECT p.codigo
FROM prod.OrdemProducao AS op
JOIN cad.Produto AS p ON p.id_produto = op.id_produto
WHERE op.data_abertura >= '2026-10-01';
GO

/* C10 - Plano de amostragem para lotes de tamanhos diferentes
   (funcao com valor de tabela aplicada com CROSS APPLY). */
SELECT t.tamanho_lote, pa.letra, pa.amostra, pa.aceitacao, pa.rejeicao
FROM (VALUES (80), (120), (196), (480), (900), (2500)) AS t (tamanho_lote)
CROSS APPLY qual.fn_PlanoAmostragem(t.tamanho_lote) AS pa;
GO

// SIGPR - Fujiy Instrumentais
// 03 - Consultas e agregacoes sobre os laudos importados do terminal
// Antes: mongoimport --uri "mongodb://localhost/sigpr?replicaSet=rs0"
//        --collection laudos_inspecao --file /carga/laudos.jsonl
// Uso: docker exec sigpr-mongo1 mongosh --quiet /scripts/03_consultas_laudos.js

db = db.getSiblingDB("sigpr");

print("\n== Q1 - laudos importados por resultado");
db.laudos_inspecao.aggregate([
  { $group: { _id: "$resultado", laudos: { $sum: 1 } } },
  { $sort: { _id: 1 } }
]).forEach(printjson);

print("\n== Q2 - laudo de um lote (consulta por indice)");
printjson(db.laudos_inspecao.findOne(
  { lote: "L2608-0014" },
  { _id: 0, lote: 1, "produto.codigo": 1, data: 1, plano: 1, defeitos: 1, resultado: 1, medicoes: 1 }
));

print("\n== Q3 - dureza por familia: media, desvio e extremos");
db.laudos_inspecao.aggregate([
  { $unwind: "$medicoes" },
  { $match: { "medicoes.caracteristica": "dureza_hrc" } },
  { $group: {
      _id: "$produto.familia",
      laudos: { $sum: 1 },
      media: { $avg: "$medicoes.valor" },
      desvio: { $stdDevPop: "$medicoes.valor" },
      minima: { $min: "$medicoes.valor" },
      maxima: { $max: "$medicoes.valor" } } },
  { $project: { laudos: 1, media: { $round: ["$media", 2] }, desvio: { $round: ["$desvio", 2] },
                minima: 1, maxima: 1 } },
  { $sort: { _id: 1 } }
]).forEach(printjson);

print("\n== Q4 - medicoes fora da especificacao");
db.laudos_inspecao.aggregate([
  { $unwind: "$medicoes" },
  { $match: { "medicoes.conforme": false } },
  { $project: { _id: 0, lote: 1, familia: "$produto.familia", caracteristica: "$medicoes.caracteristica",
                valor: "$medicoes.valor", minimo: "$medicoes.minimo", maximo: "$medicoes.maximo", resultado: 1 } }
]).forEach(printjson);

print("\n== Q5 - plano de execucao da busca por lote");
const plano = db.laudos_inspecao.find({ lote: "L2608-0014" }).explain("executionStats");
printjson({
  estagio: plano.queryPlanner.winningPlan.inputStage
    ? plano.queryPlanner.winningPlan.inputStage.stage
    : plano.queryPlanner.winningPlan.stage,
  indice: plano.queryPlanner.winningPlan.inputStage
    ? plano.queryPlanner.winningPlan.inputStage.indexName : null,
  documentosExaminados: plano.executionStats.totalDocsExamined,
  documentosRetornados: plano.executionStats.nReturned,
  tempoMs: plano.executionStats.executionTimeMillis
});

print("\n== Q6 - validacao de esquema recusa laudo sem lote valido");
try {
  db.laudos_inspecao.insertOne({ lote: "SEM-LOTE", numero_op: 99, produto: { codigo: "X", familia: "CUR" },
                                 data: new Date(), plano: {}, defeitos: 0, resultado: "TALVEZ", medicoes: [] });
} catch (e) {
  print(`Erro ${e.code}: ${e.message}`);
}

// SIGPR - Fujiy Instrumentais
// 05 - Replicacao: estado dos nos, escrita confirmada pela maioria e
// leitura em secundario.
// Uso: docker exec sigpr-mongo1 mongosh --quiet
//        "mongodb://mongo1,mongo2,mongo3/sigpr?replicaSet=rs0" /scripts/05_replicacao.js

print("== R1 - membros do replica set");
const estado = rs.status();
const primario = estado.members.find((m) => m.stateStr === "PRIMARY");
estado.members.forEach((m) => {
  const atraso = primario ? (primario.optimeDate - m.optimeDate) / 1000 : null;
  print(`${m.name.padEnd(14)} ${m.stateStr.padEnd(10)} atraso de replicacao: ${atraso} s`);
});

print("\n== R2 - escrita com confirmacao da maioria (2 de 3 nos)");
const resultado = db.getSiblingDB("sigpr").eventos_teste.insertOne(
  { tipo: "teste_replicacao", em: new Date() },
  { writeConcern: { w: "majority", wtimeout: 5000 } }
);
printjson({ confirmado: resultado.acknowledged });

print("\n== R3 - leitura em no secundario");
db.getMongo().setReadPref("secondary");
const leitura = db.getSiblingDB("sigpr").laudos_inspecao.find({}, { lote: 1 }).limit(1).toArray();
printjson({ lido_de: db.getMongo().getReadPrefMode(), laudos_lidos: leitura.length });
db.getMongo().setReadPref("primary");
db.getSiblingDB("sigpr").eventos_teste.drop();

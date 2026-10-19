// SIGPR - Fujiy Instrumentais
// 01 - Inicia o replica set rs0 com tres nos.
// Uso: docker exec sigpr-mongo1 mongosh --quiet /scripts/01_replica_set.js
//
// O no mongo1 tem prioridade maior e tende a ser o primario. Se ele
// cair, os outros dois ainda formam maioria (2 de 3) e elegem um novo.

const configuracao = {
  _id: "rs0",
  members: [
    { _id: 0, host: "mongo1:27017", priority: 2 },
    { _id: 1, host: "mongo2:27017", priority: 1 },
    { _id: 2, host: "mongo3:27017", priority: 1 }
  ]
};

try {
  rs.status();
  print("Replica set ja estava iniciado.");
} catch (e) {
  rs.initiate(configuracao);
  print("Replica set iniciado.");
}

let espera = 0;
while (!db.hello().isWritablePrimary && espera < 60) {
  sleep(1000);
  espera++;
}

rs.status().members.forEach((m) => print(`${m.name.padEnd(14)} ${m.stateStr}`));

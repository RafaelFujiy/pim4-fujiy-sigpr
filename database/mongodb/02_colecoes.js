// SIGPR - Fujiy Instrumentais
// 02 - Colecoes, validacao de esquema e indices
// Uso: docker exec sigpr-mongo1 mongosh --quiet /scripts/02_colecoes.js

db = db.getSiblingDB("sigpr");
db.laudos_inspecao.drop();
db.telemetria_fornos.drop();

// Laudos: o esquema e flexivel nas medicoes (cada familia de produto
// mede coisas diferentes), mas os campos que identificam o laudo sao
// obrigatorios e conferidos pelo proprio banco.
db.createCollection("laudos_inspecao", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["lote", "numero_op", "produto", "data", "plano", "defeitos", "resultado", "medicoes"],
      properties: {
        lote:      { bsonType: "string", pattern: "^L[0-9]{4}-[0-9]{4}$" },
        numero_op: { bsonType: ["int", "long"], minimum: 1 },
        produto: {
          bsonType: "object",
          required: ["codigo", "familia"],
          properties: {
            codigo:  { bsonType: "string" },
            familia: { enum: ["CUR", "SON", "ESP", "ESC", "PIN", "TES", "PAG"] }
          }
        },
        data:      { bsonType: "date" },
        defeitos:  { bsonType: ["int", "long"], minimum: 0 },
        resultado: { enum: ["APROVADO", "REPROVADO"] },
        medicoes: {
          bsonType: "array",
          minItems: 1,
          items: {
            bsonType: "object",
            required: ["caracteristica", "valor"],
            properties: {
              caracteristica: { bsonType: "string" },
              valor:          { bsonType: ["double", "int"] }
            }
          }
        }
      }
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

// Telemetria dos fornos: colecao de series temporais. O MongoDB agrupa
// as leituras do mesmo forno em blocos por intervalo de tempo e apaga
// sozinho o que passar de 365 dias (expireAfterSeconds).
db.createCollection("telemetria_fornos", {
  timeseries: { timeField: "ts", metaField: "meta", granularity: "minutes" },
  expireAfterSeconds: 365 * 24 * 60 * 60
});

db.laudos_inspecao.createIndex({ lote: 1 }, { unique: true });
db.laudos_inspecao.createIndex({ "produto.familia": 1, data: -1 });
db.laudos_inspecao.createIndex({ "medicoes.caracteristica": 1 });
db.telemetria_fornos.createIndex({ "meta.forno": 1, ts: 1 });

printjson(db.getCollectionInfos({}, { nameOnly: true }).map((c) => c.name));

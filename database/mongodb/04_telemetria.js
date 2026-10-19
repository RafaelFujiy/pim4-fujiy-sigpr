// SIGPR - Fujiy Instrumentais
// 04 - Telemetria dos fornos de tratamento termico (serie temporal)
// Uso: docker exec sigpr-mongo1 mongosh --quiet /scripts/04_telemetria.js
//
// Os controladores dos fornos FT-01 e FT-02 enviam uma leitura a cada
// 5 minutos. Aqui as leituras de 01/07 a 18/10/2026 sao simuladas a
// partir das janelas de tratamento termico (etapa 4) exportadas pelo
// terminal, com patamar de tempera em 1030 C (faixa interna 1020-1040 C).

const fs = require("fs");
db = db.getSiblingDB("sigpr");
db.telemetria_fornos.deleteMany({});

// Janelas da etapa 4 lidas do arquivo de carga do terminal em C
const linhas = fs.readFileSync("/carga/carga_apontamentos.csv", "utf8").trim().split("\n").slice(1);
const janelas = linhas
  .map((l) => l.split(";"))
  .filter((c) => c[1] === "4")
  .map((c) => ({
    op: Number(c[0]),
    forno: Number(c[0]) % 2 === 0 ? "FT-01" : "FT-02",
    inicio: new Date(c[3].replace(" ", "T") + ":00-03:00"),
    fim: new Date(c[4].replace(" ", "T") + ":00-03:00")
  }));

// Gerador pseudoaleatorio com semente fixa: o resultado se repete
let semente = 4242;
function ruido(amplitude) {
  semente = (semente * 1103515245 + 12345) % 2147483648;
  return ((semente / 2147483648) * 2 - 1) * amplitude;
}

const PASSO = 5 * 60 * 1000;
const inicioPeriodo = new Date("2026-07-01T00:00:00-03:00").getTime();
const fimPeriodo = new Date("2026-10-19T00:00:00-03:00").getTime();
let lote = [];
let total = 0;

for (const forno of ["FT-01", "FT-02"]) {
  const doForno = janelas.filter((j) => j.forno === forno);
  for (let t = inicioPeriodo; t < fimPeriodo; t += PASSO) {
    const janela = doForno.find((j) => t >= j.inicio.getTime() && t < j.fim.getTime());
    let temperatura, fase;
    const leitura = { ts: new Date(t), meta: { forno: forno } };
    if (janela) {
      const minutos = (t - janela.inicio.getTime()) / 60000;
      // na OP 14 o forno nao sustentou o patamar (falha simulada)
      const patamar = janela.op === 14 ? 996 : 1030;
      if (minutos < 30) {
        fase = "aquecimento";
        temperatura = 300 + (patamar - 300) * (minutos / 30) + ruido(5);
      } else {
        fase = "patamar";
        temperatura = patamar + ruido(4);
      }
      leitura.meta.op = janela.op;
    } else {
      fase = "espera";
      temperatura = 280 + ruido(10);
    }
    leitura.temperatura_c = Math.round(temperatura * 10) / 10;
    leitura.setpoint_c = janela ? 1030 : 280;
    leitura.fase = fase;
    lote.push(leitura);
    if (lote.length === 5000) {
      db.telemetria_fornos.insertMany(lote);
      total += lote.length;
      lote = [];
    }
  }
}
if (lote.length > 0) {
  db.telemetria_fornos.insertMany(lote);
  total += lote.length;
}
print(`Leituras gravadas: ${total} (${janelas.length} ciclos de tempera)`);

print("\n== T1 - leituras e blocos internos (buckets) da serie temporal");
printjson({
  leituras: db.telemetria_fornos.countDocuments(),
  blocos: db.getCollection("system.buckets.telemetria_fornos").countDocuments()
});

print("\n== T2 - ciclos com patamar fora da faixa 1020-1040 C");
db.telemetria_fornos.aggregate([
  { $match: { fase: "patamar" } },
  { $group: {
      _id: { op: "$meta.op", forno: "$meta.forno" },
      leituras: { $sum: 1 },
      media: { $avg: "$temperatura_c" },
      minima: { $min: "$temperatura_c" },
      abaixo: { $sum: { $cond: [{ $lt: ["$temperatura_c", 1020] }, 1, 0] } } } },
  { $match: { abaixo: { $gt: 0 } } },
  { $project: { _id: 0, op: "$_id.op", forno: "$_id.forno", leituras: 1, abaixo: 1,
                media: { $round: ["$media", 1] }, minima: 1 } }
]).forEach(printjson);

print("\n== T3 - mesma barra de aco (MP-2608-003): OPs 14, 17 e 20 no forno");
db.telemetria_fornos.aggregate([
  { $match: { "meta.op": { $in: [14, 17, 20] }, fase: "patamar" } },
  { $group: { _id: "$meta.op", forno: { $first: "$meta.forno" },
              media: { $avg: "$temperatura_c" }, minima: { $min: "$temperatura_c" } } },
  { $project: { forno: 1, media: { $round: ["$media", 1] }, minima: 1 } },
  { $sort: { _id: 1 } }
]).forEach(printjson);

print("\n== T4 - media por hora no ciclo da OP 14 (fuso de Brasilia)");
db.telemetria_fornos.aggregate([
  { $match: { "meta.op": 14 } },
  { $group: { _id: { $dateTrunc: { date: "$ts", unit: "hour", timezone: "America/Sao_Paulo" } },
              media: { $avg: "$temperatura_c" }, leituras: { $sum: 1 } } },
  { $sort: { _id: 1 } },
  { $project: { _id: 0, hora: { $dateToString: { date: "$_id", format: "%d/%m %H:%M",
                                                  timezone: "America/Sao_Paulo" } },
                media: { $round: ["$media", 1] }, leituras: 1 } }
]).forEach(printjson);

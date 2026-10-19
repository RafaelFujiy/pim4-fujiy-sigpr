# SIGPR - Sistema Integrado de Gestão da Produção e Rastreabilidade

Projeto Integrado Multidisciplinar IV do curso superior de tecnologia em Análise e Desenvolvimento de Sistemas (UNIP EaD).
Autor: Rafael Fujiy Ceregati.

A Fujiy Instrumentais Ltda. é uma empresa fictícia de Ribeirão Preto (SP) que fabrica instrumentais odontológicos e cirúrgicos em aço inoxidável. O SIGPR substitui a ficha de ordem de produção em papel por um terminal de apontamento no chão de fábrica. Com ele, a inspeção final segue a ABNT NBR 5426, e qualquer lote pode ser rastreado até a barra de aço que lhe deu origem e até os clientes que o receberam.

## Estrutura

```
c/                      terminal de apontamento em C (C99)
  include/  src/        módulos: entrada, arquivos, produção, qualidade, rastreio, relatórios
  tests/                testes automatizados (87 verificações)
  dados/                arquivos CSV de trabalho (dados simulados de jul. a out. de 2026)
  exportar/             arquivos gerados para o SQL Server e o MongoDB
database/sqlserver/     scripts T-SQL 01 a 08, carregar.sh e instalar.sh
database/mongodb/       scripts mongosh 01 a 05
docs/diagramas/         diagramas UML, BPMN e entidade-relacionamento
infra/                  regras de firewall do servidor de banco (iptables)
docker-compose.yml      SQL Server 2022 e MongoDB 7 (replica set de três nós)
```

## Terminal em C

Linux ou contêiner com GCC:

```
cd c
make sigpr      # compila
make teste      # testes
make memoria    # testes com AddressSanitizer
./sigpr
```

No Windows, com o GCC do MinGW (o mesmo do Dev-C++ e do Code::Blocks) no PATH, execute `compilar.bat` dentro da pasta `c`.

O programa lê e grava os arquivos da pasta `dados`. A opção 8 do menu gera em `exportar` os arquivos de carga do SQL Server (`carga_*.csv`) e os laudos de inspeção para o MongoDB (`laudos.jsonl`).

## Bancos de dados

```
cp .env.example .env          # defina a senha do usuário sa
docker compose up -d
docker exec sigpr-sqlserver bash /scripts/instalar.sh
docker exec sigpr-mongo1 mongosh --quiet /scripts/01_replica_set.js
docker exec sigpr-mongo1 mongosh --quiet /scripts/02_colecoes.js
docker exec sigpr-mongo1 mongoimport --uri "mongodb://localhost/sigpr?replicaSet=rs0" --collection laudos_inspecao --file /carga/laudos.jsonl
docker exec sigpr-mongo1 mongosh --quiet "mongodb://localhost/sigpr?replicaSet=rs0" /scripts/03_consultas_laudos.js
docker exec sigpr-mongo1 mongosh --quiet "mongodb://localhost/sigpr?replicaSet=rs0" /scripts/04_telemetria.js
docker exec sigpr-mongo1 mongosh --quiet "mongodb://mongo1,mongo2,mongo3/sigpr?replicaSet=rs0" /scripts/05_replicacao.js
```

O `instalar.sh` executa os scripts na ordem: criação do banco, tabelas, cadastros, objetos (views, funções, procedures e triggers), carga dos arquivos do terminal, consultas, teste de índices e perfis de acesso com backup.

## Rede e firewall

A rede proposta para a fábrica separa servidores (VLAN 10, 10.10.10.0/24), chão de fábrica (VLAN 20, 10.10.20.0/24) e escritório (VLAN 30, 10.10.30.0/24). As estações só abrem conexões de saída: o `bcp` usa a porta TCP 1433 do SQL Server e o `mongoimport`, a 27017 do MongoDB.

No servidor de banco (Linux com Docker), `infra/firewall_servidor.sh` nega a entrada por padrão, libera o SSH só para o escritório e aceita as portas 1433 e 27017 apenas das VLANs 20 e 30. As regras dos bancos ficam na cadeia `DOCKER-USER`, porque o Docker redireciona as portas publicadas antes da cadeia `INPUT`. Ajuste a variável `LAN` para a interface do servidor e rode como root:

```
sudo sh infra/firewall_servidor.sh
```

O `docker-compose.yml` é o ambiente de laboratório e roda o MongoDB sem autenticação. Em produção, os nós do replica set devem se autenticar entre si (certificados X.509) com o controle de acesso por papéis ativo.

## Dados

Todos os dados são fictícios, e os CNPJs foram gerados apenas para passar na validação dos dígitos verificadores. As especificações de dureza e dimensões são internas da empresa fictícia e não reproduzem norma técnica.

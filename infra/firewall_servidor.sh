#!/bin/sh
# Firewall do servidor de banco do SIGPR (Linux com Docker), proposto para a
# rede da fabrica:
#   VLAN 10 - servidores       10.10.10.0/24 (este servidor: 10.10.10.5)
#   VLAN 20 - chao de fabrica  10.10.20.0/24 (estacoes de apontamento)
#   VLAN 30 - escritorio       10.10.30.0/24 (PCP, qualidade e gestao)
# As estacoes so abrem conexoes de saida: no fim do turno o bcp usa a porta
# 1433 do SQL Server e o mongoimport a 27017 do MongoDB.
# As portas publicadas pelo Docker sofrem DNAT antes da cadeia INPUT; por isso
# as regras dos bancos ficam na cadeia DOCKER-USER, que o Docker processa antes
# das proprias regras. Rodar como root, depois que o Docker subir.
set -eu
LAN=eth0
FABRICA=10.10.20.0/24
ESCRITORIO=10.10.30.0/24

# 1) O proprio servidor: entrada negada por padrao, SSH so a partir do escritorio
iptables -F INPUT
iptables -A INPUT -i lo -j ACCEPT
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -i "$LAN" -p tcp --dport 22 -s "$ESCRITORIO" -j ACCEPT
iptables -P INPUT DROP

# 2) Containers: SQL Server (1433) e MongoDB (27017) so para fabrica e
#    escritorio. Depois do DNAT a porta vista e a do container, que no
#    docker-compose.yml e igual a publicada.
iptables -F DOCKER-USER
iptables -A DOCKER-USER -m conntrack --ctstate ESTABLISHED,RELATED -j RETURN
for REDE in "$FABRICA" "$ESCRITORIO"; do
  for PORTA in 1433 27017; do
    iptables -A DOCKER-USER -i "$LAN" -s "$REDE" -p tcp --dport "$PORTA" -j RETURN
  done
done
iptables -A DOCKER-USER -i "$LAN" -j DROP
iptables -A DOCKER-USER -j RETURN

iptables -L INPUT -n --line-numbers
iptables -L DOCKER-USER -n --line-numbers
# Para manter as regras apos reiniciar (Debian/Ubuntu): netfilter-persistent save

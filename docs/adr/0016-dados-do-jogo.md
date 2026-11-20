---
id: 0016
titulo: Dados do jogo
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0016: Dados do jogo

Fonte: [D-16 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-16--dados-do-jogo).

## Contexto

O cliente precisa de dados visuais (modelos, nomes, ícones) e de alguns dados para exibir regras (a janela de atributos). As regras de verdade são do servidor.

## Decisão

- **De onde vêm:**
  - os dados **visuais e de interface** (modelos, nomes, tamanhos no inventário, ícones, textos, minimapa) vêm dos arquivos originais do usuário, pelo `muimport`;
  - as **regras** (dano, drop, requisitos) são do servidor;
  - os dados que o cliente precisa para **exibir** regras são conferidos contra as definições do OpenMU, e qualquer divergência vira aviso no importador.
- **Formato:** JSON legível no conteúdo e no cache. Um binário compacto só se a medição de carga pedir.
- **Regra de ouro:** vai para dados o que alguém teria motivo para ajustar; o resto fica em código. Cada conteúdo movido para dados é comparado, entrada por entrada, com a fonte original.
- **Efeitos:** o comportamento é código do módulo MU, escrito a partir de specs. O catálogo em dados só guarda parâmetros.

## Consequências

- Nenhuma tabela de dados da Webzen entra no repositório (INV-17).

## Alternativas consideradas

- **Tudo em dados:** comportamento que ninguém teria motivo para ajustar viraria configuração sem dono.

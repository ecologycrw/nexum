---
id: 0009
titulo: Render forward com passes fixos e Classic em espaço gama
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0009: Render forward com passes fixos e Classic em espaço gama

Fonte: [D-09 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-09--render-forward-passes-fixos-classic-em-espaço-gama).

## Contexto

O conteúdo do original é cheio de aditivos e transparências, e a iluminação é por vértice, a partir de uma grade, em espaço gama. A paridade visual exige respeitar isso.

## Decisão

- **Forward rendering**, com uma **lista fixa de passes** por quadro. Sem render graph até que exista um motivo medido, por exemplo mais de 10 passes com dependências dinâmicas.
- **Dois modos de aparência:**
  - **Classic:** espaço gama, iluminação por vértice e grade de luz, materiais clássicos. É o modo da paridade P2.
  - **Enhanced:** acrescenta sombra do sol, AO, bloom e LUT de cor, sem compromisso de paridade (P3).
- **Materiais são pilhas de passes.** A engine oferece o mecanismo; o módulo MU decide qual pilha cada item ou nível usa.
- **Ordem de desenho:** opacos agrupados por pipeline e textura; transparentes na ordem em que foram enviados.
- **Skinning na GPU**, com uma paleta de ossos por personagem compartilhada por todas as partes.
- **Instancing** para objetos estáticos repetidos, e **MSAA** de 2, 4 ou 8 amostras.
- **A regra do desenho:** o desenho só lê o estado do mundo e escreve apenas em estruturas próprias do quadro. Uma checagem no build de desenvolvimento acusa qualquer escrita (INV-04).

## Consequências

- O guia visual define as cenas e as tolerâncias da paridade ([VISUAL.md](../VISUAL.md)).
- A checagem de escrita durante o desenho entra no M3.

## Alternativas consideradas

- **Deferred (GBuffer):** ruim para aditivos e transparências, e pesado para o visual clássico.
- **Forward+ ou clustered:** desnecessário com a grade de luz.

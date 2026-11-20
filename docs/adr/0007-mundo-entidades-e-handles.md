---
id: 0007
titulo: Mundo com entidades, componentes e handles
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0007: Mundo com entidades, componentes e handles

Fonte: [D-07 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-07--mundo-entidades-componentes-e-handles).

## Contexto

O original usa arrays fixos e globais, e um objeto desconhecido cai numa posição reserva do array. O servidor reaproveita os ids dos objetos na hora, então um ponteiro ou um índice cru pode passar a apontar para outro objeto.

## Decisão

- **Entidade** é um handle geracional: índice e geração. Um handle velho nunca aponta para um objeto novo.
- **Componentes** são structs simples, sem herança. **Sistemas** são funções chamadas numa ordem fixa dentro do tick.
- **Armazenamento:** EnTT (MIT), escondido atrás de uma fachada `World` da engine, para poder ser trocado. ECS é a ferramenta, não o objetivo.
- **Mapa de ids da rede:** um dicionário de (época, id do servidor) para a entidade, no módulo MU. Um id desconhecido gera um log e é ignorado.
- **Estado para interpolar:** os componentes que se movem têm "anterior" e "atual" ([ADR 0005](0005-tick-fixo-de-25-hz.md)).

## Consequências

- Identificadores são tipos fortes (`EntityId`, `ServerObjectId`, `MapEpoch`), nunca `int` cru.
- O EnTT só aparece dentro de `Engine/World` (INV-10).

## Alternativas consideradas

- **Registro próprio com sparse sets:** mais didático, mais trabalho.
- **Hierarquia de classes:** o modelo do original, que se quer evitar.

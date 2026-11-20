---
id: 0019
titulo: Convenções do mundo e do código
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0019: Convenções do mundo e do código

Fonte: [D-19 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-19--convenções).

## Contexto

Eixos, unidades e ângulos errados espelham ou distorcem o mundo, e esses erros são caros de achar depois. Nomes de namespace precisam deixar a linha vermelha visível.

## Decisão

| Tema | Convenção |
|---|---|
| Eixos do mundo | X e Y no chão, **Z para cima** |
| Lateralidade | *lacuna*: confirmada no M1 com uma cena de teste importada. Não chutar |
| Unidades | as do MU: **100 unidades = 1 célula** |
| Ângulos | graus nos dados e nas ferramentas; radianos no código; o importador converte os ângulos de Euler para quaternions uma vez |
| Espaço de recorte e texturas | as convenções do SDL GPU; textura com origem no canto superior esquerdo |
| Tempo | simulação em ticks inteiros; relógio de parede em segundos (`double`) |
| Namespaces | `nexum::` na engine; `nexum::gridrpg` no kit; `mu::` no módulo MU |
| Identificadores | tipos fortes (`EntityId`, `ServerObjectId`, `MapEpoch`), nunca `int` cru |
| Texto | UTF-8 em tudo; conversão só na borda com o sistema operacional |
| Caminhos | sempre pelo VFS, com barra normal |

## Consequências

- A checagem `terms` procura o namespace `mu` dentro de `Engine/` e `Kit/` (INV-03).
- Os detalhes de estilo do código ficam em [CONVENTIONS.md](../CONVENTIONS.md).

## Alternativas consideradas

- **Metros como unidade e Y para cima:** exigiria converter tudo o que vem do original, e cada conversão é uma chance de erro de escala.

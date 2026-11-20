---
id: 0015
titulo: Aleatoriedade determinística
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0015: Aleatoriedade determinística

Fonte: [D-15 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-15--aleatoriedade-determinística).

## Contexto

Sorteios do sistema tornam os testes e os replays irreproduzíveis, e um sorteio feito durante o desenho faria o mesmo tick sair diferente a cada quadro.

## Decisão

- **Um gerador próprio**, rápido e determinístico (da família PCG ou xoshiro), com **fluxos separados**:
  - **simulação:** usado nos ticks;
  - **desenho:** semeado a cada quadro a partir do número do tick, para o mesmo tick desenhado duas vezes sair igual;
  - **UI e ferramentas.**
- **O `rand()` do sistema é proibido** em todo o código. A checagem `cpp-rules` procura `rand` e `srand` (INV-07).
- Os testes de paridade podem **fixar a semente**.
- **Não reproduzir a sequência exata do original.** A distribuição importa; a sequência exata só importaria para comparar quadro a quadro, e os testes de imagem usam cenas com os efeitos aleatórios desligados ou congelados.

## Consequências

- O gerador entra no M0, junto com o laço de ticks.

## Alternativas consideradas

- **Reproduzir o gerador do sistema do original:** só faria sentido para comparar quadro a quadro, o que os testes de imagem não fazem.

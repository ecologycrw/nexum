---
id: 0012
titulo: Áudio com paridade primeiro
status: substituida
data: 2026-11-20
substitui:
substituida-por: 0031
---

# ADR 0012: Áudio com paridade primeiro

Fonte: [D-12 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-12--áudio-paridade-primeiro).

## Contexto

No original, os efeitos sonoros tocam sempre em 2D, o jogador tem dois volumes e só os sons de objetos do mapa caem com a distância. Áudio 3D desde o início mudaria a experiência original.

## Decisão

- **miniaudio** (domínio público ou MIT-0), só dentro de `Engine/Audio`.
- **Modo Classic (paridade):**
  - efeitos em 2D;
  - dois volumes, música e efeitos;
  - sons de objetos do mapa com queda linear (1,0 até 300 unidades; 0,25 a partir de 1500);
  - passos só do herói, escolhidos pelo mundo e pela textura da camada 1 do chão.
- **Modo Enhanced:** som posicional 3D e barramentos extras (ambiente, UI), configuráveis.
- **Eventos de som em dados:** qual som toca em qual momento.

## Consequências

- O áudio de paridade entra no M7; o 3D, no M14.

## Alternativas consideradas

- **Áudio 3D com quatro barramentos desde a base:** muda a experiência original e não é paridade.

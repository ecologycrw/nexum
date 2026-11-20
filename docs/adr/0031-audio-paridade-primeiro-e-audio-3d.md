---
id: 0031
titulo: Áudio com paridade primeiro e o Áudio 3D como opção à parte
status: aceita
data: 2026-11-20
substitui: 0012
substituida-por:
---

# ADR 0031: Áudio com paridade primeiro e o Áudio 3D como opção à parte

Fonte: [D-12 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-12--áudio-paridade-primeiro). Substitui o [ADR 0012](0012-audio-paridade-primeiro.md): o áudio de paridade é o mesmo, e o som 3D deixou de fazer parte do modo Enhanced.

## Contexto

No original, os efeitos sonoros tocam sempre em 2D, o jogador tem dois volumes e só os sons de objetos do mapa caem com a distância. Áudio 3D desde o início mudaria a experiência original.

O Enhanced do MuMain, que é a referência do modo Enhanced do Nexum, é só visual e não mexe no som. O áudio 3D é uma ideia do Nexum.

## Decisão

- **miniaudio** (domínio público ou MIT-0), só dentro de `Engine/Audio`.
- **Modo Classic (paridade):**
  - efeitos em 2D;
  - dois volumes, música e efeitos;
  - sons de objetos do mapa com queda linear (1,0 até 300 unidades; 0,25 a partir de 1500);
  - passos só do herói, escolhidos pelo mundo e pela textura da camada 1 do chão.
- **Opção "Áudio 3D"**, proposta do Nexum: som posicional 3D e barramentos extras (ambiente, UI), configuráveis. Ela fica separada do Enhanced visual, para o jogador escolher uma coisa sem a outra.
- **Eventos de som em dados:** qual som toca em qual momento.

## Consequências

- O áudio de paridade entra no M7; a opção "Áudio 3D", no M14.
- A seção `audio` das configurações guarda os dois volumes, a opção "Áudio 3D" e os barramentos extras dela.

## Alternativas consideradas

- **Áudio 3D com quatro barramentos desde a base:** muda a experiência original e não é paridade.
- **Áudio 3D dentro do Enhanced** (a versão anterior da spec): misturava visual com som, e o Enhanced de referência não mexe no som.

---
id: 0011
titulo: Interface com RmlUi e base 640x480
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0011: Interface com RmlUi e base 640x480

Fonte: [D-11 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-11--interface-rmlui-com-base-640480).

## Contexto

A paridade de layout exige a mesma base e a mesma regra de escala do original. Uma interface temática precisa de uma biblioteca de UI de verdade, não de uma ferramenta de depuração.

## Decisão

- **RmlUi** (MIT) para toda a UI 2D, com a versão fixada. Se precisar de patch, fica num submódulo.
- **Layout de referência 640×480**, com a regra de escala do original: um clamp com teto, multiplicado pela preferência do jogador (75% a 150%). Um "layout moderno" pode vir depois, como tema, sem mudar a regra.
- **Temas por pasta**, com tokens; *data binding* do RmlUi para ligar janelas e estado.
- **Uma regra única** decide se a entrada vai para a UI ou para o mundo, e qual janela fica com o teclado.
- **Cursor nativo**, desenhado depois da UI, com os estados vindos de dados.
- **Texto:** um único conjunto de fontes, servindo ao RmlUi e ao texto desenhado no mundo, que sai em lote pela engine.
- **IME** pelo SDL, ao focar um campo.
- **Hot-reload** de `.rml` e `.rcss` no build de desenvolvimento.
- **Tradução:** tabelas de textos por idioma, com reserva em inglês.

## Consequências

- O RmlUi só aparece dentro de `Engine/Ui` (INV-10). As janelas do MU ficam em `Games/MuOnline/Ui`.
- Entra no M6.

## Alternativas consideradas

- **Base 1920×1080:** quebra a paridade de layout.
- **Dear ImGui para o jogo:** não serve para UI temática.
- **UI própria:** trabalho demais.

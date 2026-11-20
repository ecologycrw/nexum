---
id: 0013
titulo: Ferramentas de autoria em ImGui dentro da engine
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0013: Ferramentas de autoria em ImGui dentro da engine

Fonte: [D-13 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-13--ferramentas-autoria-em-imgui-dentro-da-engine).

## Contexto

Um editor externo em Qt com plugins em DLL desde o início traria obrigações de licença (LGPL), integração com o SDL GPU diferente por sistema e plugins C++ com ABI frágil. O cliente moderno estudado pela central mostra que um editor em ImGui, dentro do jogo, dá conta de janelas acopláveis, desfazer e refazer e editores de mapa, item, monstro, efeito e som.

## Decisão

- **Modo de autoria (Authoring)** dentro do próprio executável de desenvolvimento, com Dear ImGui (ramo *docking*).
- **Ferramentas, em ordem de prioridade:**
  1. inspetor de entidades e componentes;
  2. visualizador de modelos e animações;
  3. visualizador de mapa com a sobreposição de atributos;
  4. inspetor de pacotes, ao vivo e a partir de gravações;
  5. console de comandos;
  6. editor de mapa, que grava os formatos da engine e exporta a grade de atributos no formato do servidor;
  7. editores de dados (itens, efeitos, sons).
- **Qt e plugins em DLL: adiados.** Só voltam se a ferramenta ultrapassar o ImGui e a API pública da engine estiver estável. Se voltarem, plugins usam uma API em C.

## Consequências

- O jogador nunca recebe o modo de autoria: ele é removido na compilação do build de jogador, junto com o socket de automação e o inspetor de pacotes (INV-19). A opção `NEXUM_AUTHORING` do CMake já existe.
- O ImGui só aparece dentro de `Engine/Debug` (INV-10).

## Alternativas consideradas

- **Editor externo em Qt 6, com plugins em DLL, desde o início:** os custos descritos no contexto.

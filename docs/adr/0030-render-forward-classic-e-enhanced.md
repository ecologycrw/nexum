---
id: 0030
titulo: Render forward com passes fixos, Classic em espaço gama e o Enhanced do MuMain como referência
status: aceita
data: 2026-11-20
substitui: 0009
substituida-por:
---

# ADR 0030: Render forward com passes fixos, Classic em espaço gama e o Enhanced do MuMain como referência

Fonte: [D-09 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-09--render-forward-passes-fixos-classic-em-espaço-gama) e [IV.9.7](../referencia/especificacao-engine-propria-caminho-b.md#iv97-enhanced-look). Substitui o [ADR 0009](0009-render-forward-e-classic.md): o Classic e o forward continuam iguais, e mudaram a definição do Enhanced e o MSAA do Classic.

## Contexto

O conteúdo do original é cheio de aditivos e transparências, e a iluminação é por vértice, a partir de uma grade, em espaço gama. A paridade visual exige respeitar isso.

O modo Enhanced não existe no cliente original: ele foi criado pelo MuMain. A spec revista o descreve a partir da doc oficial do MuMain como um pacote de 7 recursos, com valores em dados, ambientes por mapa e uma volta automática quando um passe não pode ser desenhado. No MuMain, o Classic desenha sem antialiasing.

## Decisão

- **Forward rendering**, com uma **lista fixa de passes** por quadro. Sem render graph até que exista um motivo medido, por exemplo mais de 10 passes com dependências dinâmicas.
- **Dois modos de aparência:**
  - **Classic:** espaço gama, iluminação por vértice e grade de luz, materiais clássicos. É o modo da paridade P2.
  - **Enhanced:** tem como referência os 7 recursos do MuMain, sem compromisso de paridade (P3). São eles: filtragem de textura; iluminação aprimorada (luz por pixel nos modelos, contorno de luz e brilho nas armaduras de metal); luzes dinâmicas por pixel; luz do sol (mapa de sombra, nuvens e tom do céu, que precisa das luzes dinâmicas); light mood (a luz própria de cada mapa, mais escura, só no Enhanced); pós-processamento (bloom só nos efeitos aditivos, LUT de cor por mapa e AO); e antialiasing.
- **O Enhanced é feito em sala limpa:** a partir da descrição da spec, sem copiar nomes, chaves ou formatos do MuMain. As fórmulas e os valores entram numa spec de render da central antes do M14.
- **Os valores do Enhanced ficam em dados**, por mapa e por ambiente (externo, fechado, subaquático...), com a preferência do jogador por cima.
- **Cada recurso tem a sua própria chave**, inclusive o AO separado do bloom.
- **Volta automática por passe:** quando um passe opcional não pode ser desenhado, o jogo cai para uma versão mais simples sem perder geometria.
- **Materiais são pilhas de passes.** A engine oferece o mecanismo; o módulo MU decide qual pilha cada item ou nível usa.
- **Ordem de desenho:** opacos agrupados por pipeline e textura; transparentes na ordem em que foram enviados.
- **Skinning na GPU**, com uma paleta de ossos por personagem compartilhada por todas as partes.
- **Instancing** para objetos estáticos repetidos.
- **MSAA** de 2, 4 ou 8 amostras no Enhanced. No Classic, ele vem desligado por padrão, como no MuMain, e o jogador pode ligar.
- **A regra do desenho:** o desenho só lê o estado do mundo e escreve apenas em estruturas próprias do quadro. Uma checagem no build de desenvolvimento acusa qualquer escrita (INV-04).

## Consequências

- O guia visual define as cenas e as tolerâncias da paridade ([VISUAL.md](../VISUAL.md)). As cenas de paridade usam o Classic sem MSAA, nos dois programas.
- A checagem de escrita durante o desenho entra no M3.
- O Enhanced completo é o marco M14. No Windows, ele roda sobre o Vulkan enquanto o defeito do D3D12 não for corrigido ([ADR 0029](0029-sdl3-e-sdl-gpu.md)).
- O som não faz parte do Enhanced: o áudio 3D é uma opção à parte ([ADR 0031](0031-audio-paridade-primeiro-e-audio-3d.md)).

## Alternativas consideradas

- **Deferred (GBuffer):** ruim para aditivos e transparências, e pesado para o visual clássico.
- **Forward+ ou clustered:** desnecessário com a grade de luz.
- **Enhanced só com sombra do sol, AO, bloom e LUT** (a versão anterior da spec): deixava de fora 4 dos 7 recursos que o MuMain já prova.

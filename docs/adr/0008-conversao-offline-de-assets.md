---
id: 0008
titulo: Assets convertidos offline para formatos próprios
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0008: Assets convertidos offline para formatos próprios

Fonte: [D-08 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-08--assets-conversão-offline-para-formatos-próprios).

## Contexto

Ler os arquivos originais dentro do jogo deixaria a lógica de decifrar os arquivos de outra empresa no runtime, obrigaria a engine a carregar formatos ineficientes e amarraria as ferramentas a eles.

## Decisão

- A ferramenta **`muimport`**, em `Tools/`, lê a pasta de dados da instalação do usuário e grava um **cache convertido**, na máquina dele, nos formatos da engine:

| Original | Formato da engine |
|---|---|
| modelos | modelo próprio: malhas, esqueleto, clipes de animação e referências de material, num binário versionado e em blocos |
| texturas | KTX2, com compressão BCn no desktop e mipmaps |
| arquivos de mapa (terreno, atributos, objetos, altura, luz) | mapa: heightfield, camadas de tiles, atributos, objetos, luz base |
| tabelas de dados (itens, monstros, minimapa, textos) | JSON da engine |
| sons e músicas | arquivos de áudio (formato de entrada confirmado no M1) |

- O jogo lê **só** os formatos da engine, por um **VFS** com montagens (cache convertido, conteúdo próprio, mods) e resolução de nomes sem diferenciar maiúsculas.
- O cache é indexado pelo hash do arquivo de entrada mais a versão do importador. Reimportar só refaz o que mudou.
- O `muimport` também **exporta glTF** de qualquer modelo convertido, para conferir no Blender. O caminho inverso (glTF para o modelo da engine) é a porta para conteúdo novo e mods.

## Consequências

- O usuário faz dois passos: importar uma vez e depois jogar.
- O importador vira uma peça crítica, com testes contra arquivos reais (locais, fora do repositório) e fuzzing.
- A engine fica neutra de verdade. Os nomes dos formatos originais só aparecem em `Tools/`, e a checagem `terms` confere (INV-08).

## Alternativas consideradas

- **Leitores dos formatos originais dentro do runtime:** a proposta da versão original da spec.
- **Híbrido, com leitores no runtime só no desenvolvimento:** dois caminhos para manter.

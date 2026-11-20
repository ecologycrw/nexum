---
id: 0022
titulo: Organização do repositório em Engine, Kit, Games e Tools
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0022: Organização do repositório em Engine, Kit, Games e Tools

Fonte: [III.2 da spec](../referencia/especificacao-engine-propria-caminho-b.md#iii2-repositório-e-módulos).

## Contexto

A spec define a árvore do repositório e a convenção `Public/` e `Private/`. O ambiente espelhado do Lemuria também tem ferramentas internas em Python, que lá ficam em `tools/`. No Windows, `tools/` e o `Tools/` da spec seriam a mesma pasta.

## Decisão

- **As camadas ficam na raiz:** `Engine/`, `Kit/`, `Games/` e `Tools/`. Pastas com inicial maiúscula são do produto; pastas em minúsculas são da infraestrutura (`cmake/`, `scripts/`, `tests/`, `docs/`, `specs/`, `templates/`).
- **Todo módulo tem o mesmo formato:** `Public/` (a API), `Private/` (a implementação), `Tests/` e `CMakeLists.txt`. O CMake só expõe `Public/`.
- **Um alvo por módulo da engine** (`Engine/Core`, `Engine/Platform`...). **O kit e o módulo MU são um alvo cada**, e as áreas da spec (`Grid`, `Movement`, `Protocol`, `Session`...) ficam em subpastas de `Public/` e `Private/`. O executável do jogo fica em `Games/MuOnline/App`.
- **As ferramentas internas em Python ficam em `scripts/`** (`scripts/dev.py`, `scripts/checks/`), e não em `tools/`, para não colidir com `Tools/` no Windows.
- **Uma pasta só nasce na fatia que precisa dela.** A tabela de pastas futuras fica em [ARCHITECTURE.md](../ARCHITECTURE.md#estrutura-do-repositório).
- A checagem `structure` confere o formato dos módulos e recusa as pastas `src`, `include` e `source` na raiz.

## Consequências

- O comando único do projeto é `py scripts/dev.py`, e não `py tools/dev.py` como no Lemuria.

## Alternativas consideradas

- **Renomear o `Tools/` da spec:** a árvore da spec é a referência; mudar a infraestrutura custa menos.
- **Um alvo por área do kit e do módulo MU:** muitos alvos pequenos antes da necessidade (regra de três).

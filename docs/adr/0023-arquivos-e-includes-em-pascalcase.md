---
id: 0023
titulo: Arquivos C++ e pastas de include em PascalCase
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0023: Arquivos C++ e pastas de include em PascalCase

## Contexto

Os nomes de tipos e funções são `PascalCase` ([ADR 0003](0003-linguagem-build-e-dependencias.md)). Arquivos e includes com outro padrão fariam o leitor alternar entre dois estilos. No Lemuria, a mesma decisão evitou arquivos com nomes que não batiam com o tipo principal.

## Decisão

- Cada arquivo C++ tem o nome do tipo principal, em `PascalCase`, com `.hpp` ou `.cpp`: `Version.hpp`, `Version.cpp`.
- Arquivos de teste terminam em `Test`: `VersionTest.cpp`.
- As pastas de include espelham o namespace: `Nexum/Core/` para `nexum::core`, `Nexum/GridRpg/` para `nexum::gridrpg` e `Mu/` para `mu::`.
- A checagem `structure` recusa um arquivo C++ fora do padrão e um `.cpp` de `Tests/` que não termine em `Test.cpp`.

## Consequências

- Os includes ficam no formato `#include "Nexum/Core/Result.hpp"`.

## Alternativas consideradas

- **Arquivos em `snake_case`:** comum em C++, mas destoa dos nomes dos tipos.

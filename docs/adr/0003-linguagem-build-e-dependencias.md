---
id: 0003
titulo: Linguagem, build e dependências
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0003: Linguagem, build e dependências

Fonte: [D-03 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-03--linguagem-build-e-dependências).

## Contexto

O projeto precisa compilar igual em três sistemas e três compiladores, com as mesmas versões de bibliotecas para todos, e precisa de uma regra única de estilo.

## Decisão

- **Linguagem:** C++20, sem extensões de compilador e sem módulos do C++20. Do C++23, só o que os três compiladores suportarem bem, por ADR.
- **Compiladores:** MSVC no Windows, Clang no Linux e no macOS, e GCC no Linux, pela CI.
- **Build:** CMake 3.28 ou mais novo, com `CMakePresets.json` e Ninja. Presets de desenvolvimento, com sanitizers e de release (o build de jogador).
- **Nomes dos alvos:** `nexum_<módulo>`, com o alias `Nexum::<Módulo>` (`Nexum::Core`, `Nexum::Platform`); o kit é `Nexum::GridRpg`; o módulo MU é `MuOnline`, e o executável é `MuClient`.
- **Dependências:** vcpkg em modo manifesto, com a linha de base fixada. Uma biblioteca que precisar de patch entra como submódulo, fixado num commit.
- **Estilo:** clang-format e clang-tidy, com a regra escrita em [CONVENTIONS.md](../CONVENTIONS.md): tipos e funções em `PascalCase`, variáveis em `camelCase`, membros privados com `m_`.
- **Erros:** `Result<T>` onde a falha é esperada; `assert` e log para bugs; exceções só nas ferramentas offline, nunca atravessando um módulo do runtime.
- **Ajuste na adoção:** a spec sugere o `std::expected` do C++23. O Nexum usa o `tl::expected`, que tem a mesma interface e funciona em C++20. A troca é uma linha, quando o modo C++23 for adotado nos três compiladores ([TD-0004](../TECH_DEBT.md)).

## Consequências

- O M0 começa com o build, os presets e a CI antes de qualquer gráfico. Parte disso veio na preparação.
- Uma regra de estilo só, conferida por máquina.

## Alternativas consideradas

- **`FetchContent` do CMake:** mais simples, mas recompila tudo e fixa versões pior.
- **Conan:** bom, mas é mais uma ferramenta para aprender.
- **Compilar já em C++23:** o suporte do MSVC ao modo C++23 ainda é parcial.

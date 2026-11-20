---
id: 0026
titulo: Documentação como memória do projeto
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0026: Documentação como memória do projeto

## Contexto

Cada sessão da IA começa sem lembrar das anteriores. O que não estiver escrito se perde, e o dono precisa entender o projeto sem ler código.

## Decisão

- **Idiomas:** código, comentários e commits em inglês; specs, planos, relatórios, ADRs e docs do projeto em português simples.
- **`AGENTS.md` é a fonte única das regras** para qualquer IA. O `CLAUDE.md` só acrescenta o que é do Claude Code.
- **Quatro arquivos por fatia** em `specs/NNNN-nome/`: `spec.md`, `plan.md`, `tasks.md` (a memória entre sessões) e `report.md`.
- **Três tipos de documento:** vivos (atualizados no mesmo PR do código), históricos (nunca editados; um ADR novo substitui o antigo) e de ambiente (as travas, só por proposta aprovada).
- **Diagramas em Mermaid**, com no máximo cerca de 15 elementos.
- **Front matter** nas specs, nos ADRs e no STATUS, conferido pela checagem `docs`.
- **A cópia da spec de referência** fica em `docs/referencia/`, sem edição, e é uma trava.

## Consequências

- O `tasks.md` sempre atualizado permite que qualquer sessão continue de onde a outra parou.
- O sistema de documentos foi espelhado do Lemuria, para o dono reconhecer o mesmo fluxo nos dois projetos.

## Alternativas consideradas

- **Documentação só no código:** o dono não programa e não conseguiria acompanhar.

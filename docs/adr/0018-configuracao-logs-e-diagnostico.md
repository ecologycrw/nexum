---
id: 0018
titulo: Configuração, logs e diagnóstico
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0018: Configuração, logs e diagnóstico

Fonte: [D-18 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-18--configuração-logs-e-diagnóstico).

## Contexto

Configuração, logs e relatórios de falha precisam de um lugar próprio na máquina do usuário, com nomes que não confundam o Nexum com produtos de terceiros.

## Decisão

- **Pasta do usuário** pelo SDL, com nomes nossos de organização e aplicativo: `Nexum` e `MuClient`.
- **Configuração** em JSON com `"schema_version"` e migrações automáticas entre versões. Vídeo, áudio, UI, controles e opções do ajudante ficam em seções separadas.
- **Logs** por categoria (render, rede, recursos, jogo), com níveis. O log de pacotes **mascara credenciais** (INV-18).
- **Profiler** Tracy (BSD) no build de desenvolvimento, com zonas nos ticks, no desenho e nos jobs. O Tracy só aparece dentro de `Engine/Debug`, e os outros módulos usam as macros da engine.
- **Relatório de falha:** minidump no Windows e pilha de chamadas nos outros sistemas, gravados na pasta do usuário.

## Consequências

- Log, asserts e Tracy entram no M0; configuração e relatório de falha, no M7.

## Alternativas consideradas

- **Pasta com o nome do jogo original ou do servidor:** confundiria o Nexum com produtos de terceiros.

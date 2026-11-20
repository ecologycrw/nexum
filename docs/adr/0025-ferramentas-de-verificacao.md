---
id: 0025
titulo: Ferramentas de verificação
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0025: Ferramentas de verificação

## Contexto

O dono do produto não revisa código linha por linha, e o C++ permite erros de memória que outras linguagens impedem. As regras precisam ser garantidas por máquina, não depender de a IA lembrar delas. Princípio herdado do Lemuria: **toda regra que uma máquina consegue verificar vira teste, análise, hook ou CI, não parágrafo.**

## Decisão

| Necessidade | Ferramenta |
|---|---|
| Comando único de verificação | `scripts/dev.py`, em Python 3, só com a biblioteca padrão |
| Testes | doctest ([ADR 0014](0014-qualidade-e-paridade-medida.md)) |
| Avisos do compilador | tratados como erro, no MSVC, no Clang e no GCC |
| Erros de memória | AddressSanitizer no MSVC; AddressSanitizer e UndefinedBehaviorSanitizer no Clang |
| Práticas perigosas, nomes e complexidade | clang-tidy |
| Formatação | clang-format e `.editorconfig` |
| Camadas, bibliotecas atrás do módulo dono, linha vermelha, regras de C++ | `scripts/checks`, em Python 3, com testes próprios |
| Docs, specs, estrutura do repositório e arquivos originais proibidos | `scripts/checks` |
| Engine e Kit sem o módulo MU | um "outro jogo" compilado só com a Engine e o Kit |
| Antes de cada commit e push | hooks do Git em `sh` |
| Antes de cada comando da IA, no início e no fim da sessão | hooks do Claude Code em PowerShell 5.1 |
| Verificação independente | GitHub Actions em Windows, Linux (Clang e GCC) e macOS, com Debug e Release |
| Diagramas | `@mermaid-js/mermaid-cli` na CI |

**Toda trava precisa provar que funciona:** um exemplo com a violação precisa ser recusado, nos testes das checagens.

## Consequências

- Uma regra quebrada aparece como build vermelho, teste falhando ou commit bloqueado, com a explicação.
- As travas em Python rodam mesmo quando o build do C++ está quebrado.
- Os testes de mutação do Lemuria ficaram de fora: a spec não os pede. Podem voltar por proposta.
- Fuzzing, vetores de protocolo, replay e comparação de imagens entram nos marcos que trazem o que eles testam.

## Alternativas consideradas

- **Regras só em texto:** a IA pode esquecer ou ignorar.
- **Ferramentas internas em C++:** dependeriam de um build já feito, e uma trava que não roda deixaria a ação passar.
- **Catch2, como no Lemuria:** funciona bem, mas a spec escolheu o doctest; a etiqueta `[NNNN-CA-xx]` vai no nome do teste.

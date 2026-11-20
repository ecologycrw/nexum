---
id: 0021
titulo: Repositório público com licença MIT
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0021: Repositório público com licença MIT

## Contexto

O Nexum deve poder ser publicado, e o código que ele reaproveita (o OpenMU) usa a licença MIT. O dono escolheu a mesma licença do Lemuria, o outro projeto dele.

## Decisão

- O código do Nexum usa a licença **MIT** ([LICENSE](../../LICENSE)).
- O repositório foi pensado para ser **público**: nada pessoal, nenhum caminho do computador do dono, nenhum segredo, nenhum asset da Webzen.
- Código de outros projetos só entra com licença compatível com a MIT e com o aviso da licença junto do arquivo. O OpenMU é a fonte principal.
- Bibliotecas com licença GPL não entram no jogo distribuído. Toda biblioteca fica registrada em [THIRD_PARTY.md](../THIRD_PARTY.md), e a checagem `structure` confere.

## Consequências

- Os caminhos do computador do dono, como o da central de conhecimento, ficam em arquivos fora do Git (`.claude/central.local.json`).

## Alternativas consideradas

- **Decidir a licença depois:** deixaria o repositório sem permissão de uso clara.

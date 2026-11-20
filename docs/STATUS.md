---
fase: preparacao
spec-atual:
branch: preparacao/ambiente
atualizado-em: 2026-11-20
---

# Status

## Agora

**Preparação pronta, esperando a revisão do dono.** A branch `preparacao/ambiente` tem a cópia da spec do Caminho B, os documentos e os 28 ADRs, o esqueleto das três camadas (Engine, Kit e módulo MU) que compila e passa nos testes, as checagens automáticas, os hooks do Git, a CI e o ambiente de IA. O repositório ainda é só local: a CI nunca rodou ([TD-0001](TECH_DEBT.md)).

## Próxima ação

1. O dono liga os hooks do Git, uma vez: `git config core.hooksPath .githooks`.
2. O dono revisa a Preparação. Ao aprovar, ou pede o merge local (`git merge --ff-only`), ou publica o repositório seguindo [docs/setup/GITHUB.md](setup/GITHUB.md) e aprova o PR #1.
3. Numa sessão nova, aberta só no Nexum: `/planejar` da fatia 0 (o resto do M0).

## Bloqueios

Nenhum.

## Histórico

| Data | O que aconteceu |
|---|---|
| 2026-11-20 | Plano da preparação aprovado pelo dono, com Git só local, leitura do OpenMU liberada, MuMain bloqueado e licença MIT |
| 2026-11-20 | Spec do Caminho B copiada da central para `docs/referencia/`, e as decisões D-01 a D-20 adotadas como ADRs |
| 2026-11-20 | Preparação concluída na branch `preparacao/ambiente`: documentos, esqueleto, checagens, hooks do Git, CI e ambiente de IA |

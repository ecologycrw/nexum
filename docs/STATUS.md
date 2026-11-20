---
fase: preparacao
spec-atual:
branch: preparacao/publicacao
atualizado-em: 2026-11-20
---

# Status

## Agora

**Preparação publicada no GitHub, no PR #2, esperando a CI e a revisão do dono.** O repositório `ecologycrw/nexum` está público, com a `main` protegida pela regra do GitHub. A preparação entrou num único commit assinado pela conta de máquina **provisória** `lemuria-ci-dev`, porque o GitHub restringiu a `nexum-ci-dev` logo depois da criação ([TD-0009](TECH_DEBT.md)). A branch `preparacao/publicacao` tem a cópia da spec do Caminho B, os documentos e os 28 ADRs, o esqueleto das três camadas (Engine, Kit e módulo MU) que compila e passa nos testes, as checagens automáticas, os hooks do Git, a CI e o ambiente de IA. A primeira rodada da CI testa Linux, GCC e macOS pela primeira vez ([TD-0001](TECH_DEBT.md), [TD-0002](TECH_DEBT.md)).

## Próxima ação

1. A IA acompanha a CI do PR #2 e corrige o que falhar nos sistemas que ainda não tinham sido testados.
2. Com a CI verde, o dono revisa o PR #2 e faz o squash merge pelo navegador. O PR #1, da conta restrita, é fechado pelo dono sem merge.
3. Em paralelo, o dono pede ao suporte do GitHub a revisão da conta `nexum-ci-dev`. Quando ela for liberada, a conta provisória sai ([TD-0009](TECH_DEBT.md)).
4. Numa sessão nova, aberta só no Nexum: `/planejar` da fatia 0 (o resto do M0).

## Bloqueios

Nenhum. A restrição da `nexum-ci-dev` foi contornada com a conta provisória ([TD-0009](TECH_DEBT.md)).

## Histórico

| Data | O que aconteceu |
|---|---|
| 2026-11-20 | Plano da preparação aprovado pelo dono, com Git só local, leitura do OpenMU liberada, MuMain bloqueado e licença MIT |
| 2026-11-20 | Spec do Caminho B copiada da central para `docs/referencia/`, e as decisões D-01 a D-20 adotadas como ADRs |
| 2026-11-20 | Preparação concluída na branch `preparacao/ambiente`: documentos, esqueleto, checagens, hooks do Git, CI e ambiente de IA |
| 2026-11-20 | Conta de máquina `nexum-ci-dev`, chave de assinatura e hooks do Git configurados pelo dono; os commits anteriores, do modo local, ficam sem assinatura |
| 2026-11-20 | Repositório publicado (`ecologycrw/nexum`), `main` enviada com a regra desligada por um instante e protegida de novo, e PR #1 aberto |
| 2026-11-20 | A `nexum-ci-dev` aparece restrita pelo GitHub: perfil e PR #1 escondidos do público, nenhuma execução da CI, commits assinados sem verificação e a conta não aparece na busca de colaboradores. Os 4 commits do modo local, sem assinatura, também impediriam o merge do PR #1 |
| 2026-11-20 | O dono adotou a `lemuria-ci-dev` como conta de máquina provisória. A preparação foi para a branch `preparacao/publicacao` num único commit assinado, sem reescrever o histórico da `preparacao/ambiente`, e o PR #2 substitui o #1 |

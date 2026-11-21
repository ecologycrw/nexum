---
fase: em-andamento
spec-atual: 0000
branch: fatia/0000-fundacao
atualizado-em: 2026-11-21
---

# Status

## Agora

**Fatia 0 (Fundação, marco M0) fechada, aguardando o teste do dono.** O [report](../specs/0000-fundacao/report.md) tem a prova de cada critério, a sala limpa e o passo a passo do teste. A [spec](../specs/0000-fundacao/spec.md), o [plano](../specs/0000-fundacao/plan.md) e a [lista de tarefas](../specs/0000-fundacao/tasks.md) estão na branch `fatia/0000-fundacao`, e o [PR #5](https://github.com/ecologycrw/nexum/pull/5) está pronto para o teste do dono. Cada módulo novo ganhou a sua página em [docs/modules](modules/README.md). A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)).

## Próxima ação

1. O dono testa a fatia 0 pelo passo a passo do report (seção "O que você deve testar") e conta o que viu. O CA-11 é essa verificação manual.
2. `/retrospectiva` da fatia 0, na mesma branch, antes da aprovação.
3. O dono aprova e faz o merge do PR #5.
4. Numa sessão nova, ligar de novo o aviso automático da CI no PR #5, se o dono quiser, porque ele vale só para a sessão que o ligou.
5. Em paralelo, o dono pede ao suporte do GitHub a revisão da conta `nexum-ci-dev`. Quando ela for liberada, a conta provisória sai e o PR #1, escondido junto com a conta, é fechado sem merge.

## Bloqueios

Nenhum.

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
| 2026-11-20 | CI do PR #2 verde nos 10 checks, pela primeira vez em Linux (Clang e GCC) e macOS; o dono aprovou e fez o squash merge na `main` |
| 2026-11-20 | Propostas P-0001 (separar os jogos em repositórios próprios) e P-0002 (conferir se a descrição da central basta) registradas a pedido do dono; a P-0002 foi adotada logo em seguida |
| 2026-11-20 | A central reescreveu o Enhanced da spec a partir da doc oficial do MuMain. A pedido do dono, a cópia em `docs/referencia/` foi trocada inteira, os ADRs 0029 a 0031 substituíram os 0004, 0009 e 0012, e o VISUAL, a VISÃO, o ROADMAP, o glossário e o TD-0005 foram atualizados |
| 2026-11-20 | O dono fez o merge do PR #3 (proposta P-0002) e do PR #4 (a spec com o Enhanced reescrito) |
| 2026-11-20 | Spec da fatia 0 (Fundação) aprovada pelo dono, com o Tracy, as mudanças nas travas e a lista de requisitos do SDL GPU na VISÃO. O SDL 3.4.20 ainda tem o defeito do D3D12 (SDL #16369, correção prevista para o 3.6.0), e o Vulkan segue como padrão no Windows |
| 2026-11-21 | Fatia 0: as 17 tarefas concluídas e a revisão independente feita, sem achados críticos, com 2 importantes e 9 menores. As correções vão até o commit `3444b67` |
| 2026-11-21 | Fatia 0 fechada: report com a prova dos 13 critérios, as páginas dos módulos em `docs/modules`, e o PR #5 pronto para o teste do dono |

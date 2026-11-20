---
fase: em-andamento
spec-atual: 0000
branch: fatia/0000-fundacao
atualizado-em: 2026-11-20
---

# Status

## Agora

**Fatia 0 (Fundação, marco M0) em andamento.** A [spec](../specs/0000-fundacao/spec.md), o [plano](../specs/0000-fundacao/plan.md) e a [lista de tarefas](../specs/0000-fundacao/tasks.md) estão na branch `fatia/0000-fundacao`. O dono aprovou também o Tracy, as mudanças nas travas listadas no plano e a lista de requisitos do SDL GPU na VISÃO. A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)).

## Próxima ação

1. `/implementar` da fatia 0, seguindo o [tasks.md](../specs/0000-fundacao/tasks.md). Ele começa pela sondagem: provar que as máquinas da CI usam a placa de vídeo nos 3 sistemas. Se um sistema falhar, a fatia para e as opções vão para o dono.
2. Em paralelo, o dono pede ao suporte do GitHub a revisão da conta `nexum-ci-dev`. Quando ela for liberada, a conta provisória sai e o PR #1, escondido junto com a conta, é fechado sem merge.

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

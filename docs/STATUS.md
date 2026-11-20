---
fase: entre-fatias
spec-atual:
branch:
atualizado-em: 2026-11-20
---

# Status

## Agora

**Preparação concluída e na `main`.** O repositório `ecologycrw/nexum` está público, com a `main` protegida, e a CI do PR #2 passou nos 10 checks em Windows, Linux (Clang e GCC) e macOS. A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)). Nenhuma fatia está em andamento.

## Próxima ação

1. O dono revisa e faz o merge do PR da atualização da spec (o Enhanced reescrito). Ele muda o que o M0 confere antes: o defeito do D3D12 e o Vulkan como padrão no Windows.
2. Numa sessão nova, aberta só no Nexum: `/planejar` da fatia 0 (o resto do M0).
3. Em paralelo, o dono pede ao suporte do GitHub a revisão da conta `nexum-ci-dev`. Quando ela for liberada, a conta provisória sai e o PR #1, escondido junto com a conta, é fechado sem merge.

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

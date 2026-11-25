---
fase: entre-fatias
spec-atual:
branch: fatia/0000-fundacao
atualizado-em: 2026-11-25
---

# Status

## Agora

**Fatia 0 (Fundação, marco M0) concluída, esperando o merge do dono.** O dono testou, e a retrospectiva foi feita. O [report](../specs/0000-fundacao/report.md) tem a prova de cada critério, e o [PR #5](https://github.com/ecologycrw/nexum/pull/5) está pronto para a aprovação.

A retrospectiva mudou a ordem do roteiro ([ADR 0032](adr/0032-rede-antes-do-importador.md)): a próxima fatia é a **parte 1 do M2, a rede sem mapa**, antes do M1. O que o marco manda investigar antes já está na central: a spec de protocolo do cliente 106.3 e a spec dos atributos de terreno, ambas de 2026-11-22. A cópia da spec de referência foi trocada pela versão nova da central (P-0011). A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)).

## Próxima ação

1. O dono aprova e faz o squash merge do PR #5 pelo navegador.
2. `/planejar` da parte 1 do M2, numa branch nova a partir da `main`. Ele lê as specs novas da central e decide com o dono três pontos que a conferência das specs levantou:
   - a imagem do OpenMU para os testes (`0.10.1`) e a impressão digital dela;
   - se as capturas reais do laboratório da central entram no repositório como vetores;
   - o número de série próprio no login, sem usar o do MuMain.
3. Depois do merge, apagar o contêiner e o volume `nexum-linux-ci` do Docker ([tasks.md](../specs/0000-fundacao/tasks.md), "Notas para a próxima sessão").
4. Em paralelo, sem pressa: levar à central o bloco 3 (as pontas soltas da IV.12 e do Apêndice G, e a data do front matter da spec, que ficou em 2026-11-20).
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
| 2026-11-21 | Teste do dono: tudo certo. A janela preta por uns 0,7 s no início virou a proposta P-0010 |
| 2026-11-22 | A central escreveu a spec de protocolo do cliente 106.3 e a spec dos atributos de terreno, a pedido do dono, e corrigiu 3 trechos da spec de referência |
| 2026-11-25 | Retrospectiva da fatia 0: o M2 foi dividido, e a parte 1 vem antes do M1 (ADR 0032); a cópia da spec foi trocada (P-0011); as propostas P-0004, P-0005, P-0010 e P-0011 foram aceitas |

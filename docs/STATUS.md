---
fase: em-andamento
spec-atual: 0001
branch: fatia/0001-rede-ate-o-mundo
atualizado-em: 2026-11-26
---

# Status

## Agora

**Fatia 1 em andamento: a rede até o mundo** ([spec](../specs/0001-rede-ate-o-mundo/spec.md)), na branch `fatia/0001-rede-ate-o-mundo`. É a primeira das duas fatias da parte 1 do M2, que vem antes do M1 ([ADR 0032](adr/0032-rede-antes-do-importador.md)). O `MuClient` vai entrar num OpenMU local sem modificações e ficar no mundo; a fatia 2 traz a gravação `.mucap` e o replay. O andamento das tarefas fica no `tasks.md` da fatia, criado pelo `/implementar`.

A fatia 0 (Fundação, marco M0) e as três faxinas da documentação (PRs #7, #9 e #10) estão na `main`. A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)).

## Próxima ação

1. `/fechar` da fatia 1, numa sessão nova aberta pelo dono: as 18 tarefas e a revisão estão feitas. Depois vem o teste do dono, com o OpenMU de teste no Docker. O [PR #11](https://github.com/ecologycrw/nexum/pull/11) está em rascunho, só para a CI rodar no Linux e no macOS; o `/fechar` o tira do rascunho.
2. Em paralelo, sem pressa: levar à central o bloco 3 (as pontas soltas da IV.12 e do Apêndice G, e a data do front matter da spec, que ficou em 2026-11-20).
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
| 2026-11-20 | O dono fez o merge do PR #3 (proposta P-0002) e do PR #4 (a spec com o Enhanced reescrito) |
| 2026-11-20 | Spec da fatia 0 (Fundação) aprovada pelo dono, com o Tracy, as mudanças nas travas e a lista de requisitos do SDL GPU na VISÃO. O SDL 3.4.20 ainda tem o defeito do D3D12 (SDL #16369, correção prevista para o 3.6.0), e o Vulkan segue como padrão no Windows |
| 2026-11-21 | Fatia 0: as 17 tarefas concluídas e a revisão independente feita, sem achados críticos, com 2 importantes e 9 menores. As correções vão até o commit `3444b67` |
| 2026-11-21 | Fatia 0 fechada: report com a prova dos 13 critérios, as páginas dos módulos em `docs/modules`, e o PR #5 pronto para o teste do dono |
| 2026-11-21 | Teste do dono: tudo certo. A janela preta por uns 0,7 s no início virou a proposta P-0010 |
| 2026-11-22 | A central escreveu a spec de protocolo do cliente 106.3 e a spec dos atributos de terreno, a pedido do dono, e corrigiu 3 trechos da spec de referência |
| 2026-11-25 | Retrospectiva da fatia 0: o M2 foi dividido, e a parte 1 vem antes do M1 (ADR 0032); a cópia da spec foi trocada (P-0011); as propostas P-0004, P-0005, P-0010 e P-0011 foram aceitas |
| 2026-11-25 | O dono fez o squash merge do PR #5: a fatia 0 está na `main`. O contêiner e o volume `nexum-linux-ci` do Docker foram apagados no mesmo dia |
| 2026-11-25 | Faxina da documentação a partir da revisão trazida pelo dono: ADR 0033 (o dono de cada biblioteca externa), os itens provisórios TD-0022 a TD-0024, as skills sem o modo local, a checagem `cpp-rules` aceitando o `#include <new>` e o filtro morto da etiqueta `gpu` removido dos presets |
| 2026-11-25 | O dono fez o squash merge do PR #7 (a faxina da documentação) |
| 2026-11-25 | Consolidação do ambiente da IA (P-0012): a saída curta do `verify`, o `tasks.md` com tarefas curtas, as specs longas lidas por seção, cada regra com um único dono e ADR novo só para decisões próprias e desvios da spec. O dono fez o squash merge do PR #9 |
| 2026-11-25 | Consolidação dos achados de uma auditoria externa trazida pelo dono (P-0013): o clang-tidy passou a recusar variáveis globais que podem mudar, a retrospectiva e a consolidação revisam os destinos quando a ordem dos marcos muda, o dono lê o resumo de segurança antes de cada merge e entrou o TD-0025. A P-0014 ficou aberta para a parte 1 do M2 |
| 2026-11-25 | O dono fez o squash merge do PR #10 (P-0013) |
| 2026-11-25 | Spec da fatia 1 (A rede até o mundo, M2 parte 1) aprovada pelo dono, com a parte 1 do M2 em duas fatias, o OpenMU 0.10.1 fixado pela impressão digital, só os vetores escritos nos documentos da central, o número de série `NEXUM-CLIENT-001`, o TD-0015 dentro da fatia, o Asio e o ADR 0034 (o `packetgen` em Python) |
| 2026-11-26 | Fatia 1: tarefas T01 a T10 feitas. O PR #11 foi aberto em rascunho, a pedido do dono, para a CI rodar no Linux e no macOS. A primeira CI achou uma conversão de sinal num teste de fuzzing e a análise estática sem os cabeçalhos gerados (o `tidy` do `scripts/dev.py` passou a compilar antes, com o "sim" do dono). Depois das duas correções, os 10 checks passaram |
| 2026-11-26 | Fatia 1: as 18 tarefas feitas. O `MuClient` entrou no OpenMU 0.10.1 local, sem modificações, ficou 3 minutos no mundo com 9 pings e recusou a senha errada, sem nenhum erro no log do servidor. A documentação da fatia foi atualizada, e o TD-0015, o TD-0020 e o TD-0023 saíram |
| 2026-11-26 | Revisão independente da fatia 1: nenhum achado crítico, 1 importante e 12 menores. O importante (o teste do checksum) e cinco menores foram corrigidos; quatro menores entraram no TD-0028 e no TD-0032 ou viraram o TD-0036 e o TD-0037; três ficam para o `/fechar`. As correções vão até o commit `18b8420` |

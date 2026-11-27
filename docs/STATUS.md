---
fase: em-andamento
spec-atual: 0002
branch: fatia/0002-gravacao-e-replay
atualizado-em: 2026-11-27
---

# Status

## Agora

**Fatia 2 em andamento: a gravação e o replay** ([spec](../specs/0002-gravacao-e-replay/spec.md) e [plano](../specs/0002-gravacao-e-replay/plan.md)), na branch `fatia/0002-gravacao-e-replay`. O `MuClient` vai gravar a sessão no mundo num arquivo `.mucap` e reproduzi-la sem servidor, conferindo o estado do mundo nos pontos de controle. A fatia fecha a parte 1 do M2 ([ADR 0032](adr/0032-rede-antes-do-importador.md)).

O dono aprovou a spec em 2026-11-26, com as 6 decisões: o adaptador de servidor (P-0018, que vira o ADR 0035), o formato `.mucap` versão 1, a gravação e o replay só no build de desenvolvimento e dentro do módulo MU, a gravação na memória até o fim (16 MiB), a gravação de referência do OpenMU local e a trava dos `NOLINT` (P-0015). Valem as duas regras da retrospectiva da fatia 1: o PR da fatia abre em rascunho logo depois da lista de tarefas (P-0019), e todo teste de recusa confere o motivo exato, com a sabotagem onde há risco real de falso positivo (P-0020).

A fatia 1 (a rede até o mundo) está na `main` desde o squash merge do [PR #11](https://github.com/ecologycrw/nexum/pull/11), assim como a fatia 0 (Fundação, marco M0) e as três faxinas da documentação (PRs #7, #9 e #10). A conta de máquina é, por enquanto, a `lemuria-ci-dev` ([TD-0009](TECH_DEBT.md)).

## Próxima ação

1. `/revisar` da fatia 2: as 16 tarefas do [`tasks.md`](../specs/0002-gravacao-e-replay/tasks.md) estão feitas, com o [PR #12](https://github.com/ecologycrw/nexum/pull/12) em rascunho desde a lista de tarefas.
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
| 2026-11-26 | Fatia 1 fechada: report com a prova dos 15 critérios, a contagem dos vetores (100% em cada grupo), a checklist de sala limpa e a medição de 30 minutos no mundo com a janela minimizada. O PR #11 saiu do rascunho para o teste do dono |
| 2026-11-26 | Teste do dono da fatia 1: tudo passou, e a CI ficou verde nos 10 checks |
| 2026-11-26 | Retrospectiva da fatia 1: a próxima é a fatia 2 (a gravação `.mucap` e o replay). A escada de erros trouxe a P-0019 (o PR em rascunho desde a lista de tarefas) e a P-0020 (o teste de recusa confere o motivo, e os testes de risco são sabotados), aplicadas. A P-0015 e a P-0018 foram aceitas para a fatia 2, e a P-0017 para o M7. A P-0014 fechou com a medição do custo da fatia |
| 2026-11-26 | O dono fez o squash merge do PR #11: a fatia 1 está na `main` |
| 2026-11-26 | Spec da fatia 2 (A gravação e o replay, M2 parte 1) aprovada pelo dono, com o adaptador de servidor (P-0018, ADR 0035), o formato `.mucap` versão 1, a gravação e o replay só no build de desenvolvimento e no módulo MU, a gravação na memória até o fim (16 MiB), a gravação de referência do OpenMU local e a trava dos `NOLINT` (P-0015) |
| 2026-11-27 | Fatia 2: as 16 tarefas feitas. O `MuClient` gravou 60 s no OpenMU 0.10.1 local, com a `test1Dk` entrando na visão e saindo, e o replay da gravação, com o servidor desligado, conferiu cada tick e ponto de controle e mostrou as mesmas linhas do mundo. A gravação de referência de 20 s está no repositório e é reproduzida nos 3 sistemas. Saiu o TD-0037; entraram o TD-0038 e o TD-0039 |

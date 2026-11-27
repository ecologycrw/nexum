# Tarefas da fatia 2

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui. Por isso ele é curto. O detalhe de cada tarefa vai para a mensagem do commit, e as provas, para o report.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

Cada tarefa tem a primeira linha no formato abaixo, que o hook do início da sessão lê, e no máximo 2 linhas depois dela: o resultado em uma frase e o commit.

- [x] T01: O ADR 0035: o contrato do adaptador de servidor, as duas implementações, a época nos eventos, os comandos na parte 2 do M2, e a gravação e o replay no módulo MU, só no build de desenvolvimento (CA-01, CA-13). Verificação: checagem `docs` no `verify`.
  - `docs/adr/0035-adaptador-de-servidor-gravacao-e-replay.md`, aceito, com os 7 pontos e os dois desvios (as credenciais fora do contrato e o lugar fora de `Tools/`); o índice dos ADRs atualizado. Commit: este.
- [x] T02: Os `NOLINT` (P-0015, trava `scripts/checks/`): a `cpp-rules` exige a regra e o motivo na linha de cima, o `pr_summary.py` lista cada `NOLINT` novo, e o profiler ganha a linha de motivo (CA-11). Verificação: testes das checagens e a `cpp-rules` no `verify`.
  - A `cpp-rules` recusa o `NOLINT` sem cada regra (ou com `*`) e sem o motivo na linha de cima; o resumo lista cada linha nova com `NOLINT` fora dos `.md`; os 3 de hoje passam; 8 testes novos, com a sabotagem. Commit: este.
- [x] T03: O FNV-1a de 64 bits (`Engine/Core`), com os valores de teste publicados do algoritmo (CA-06). Verificação: `verify`.
  - `Nexum/Core/Fnv1a.hpp` (`Fnv1a64`, só no cabeçalho e `constexpr`): bytes, texto e números em little-endian; os 3 valores do documento do IETF passam; 5 casos novos. Commit: este.
- [ ] T04: `ReadFileBytes`, com o limite conferido antes de ler, e `WriteFileBytes` (`Engine/Platform`, pelo SDL) (CA-03, CA-04). Verificação: `verify`.
- [ ] T05: O adaptador de servidor (`Mu/Online/ServerAdapter.hpp`), o `LiveServer` com as peças de hoje e o `OnlineClient` só pelo contrato; os testes da fatia 1 montam o `LiveServer`, e um adaptador de teste prova a ordem (CA-01). Verificação: `verify`.
- [ ] T06: A época nos eventos (TD-0037): a fila guarda a época, a tradução recebe a da mensagem, e o tick descarta e conta os velhos, com a sabotagem (CA-02). Verificação: `verify`.
- [ ] T07: O formato `.mucap` (`Mu/Recording/Recording.hpp`): a escrita num espaço reservado e a leitura com cada recusa e o motivo exato, os bytes fixos de uma gravação pequena e a sabotagem do tamanho e da ordem (CA-03). Verificação: `verify`.
- [ ] T08: A impressão do estado, versão 1 (`Mu/Recording/StateDigest.hpp`): a ordem que não importa e cada campo que importa (CA-06). Verificação: `verify`.
- [ ] T09: O gravador (`Mu/Recording/Recorder.hpp`): os registros, os pontos de controle a cada 5 s, o espaço cheio e a conta e a senha ausentes do arquivo (CA-04). Verificação: `verify`.
- [ ] T10: O replay (`Mu/Online/ReplayServer.hpp`): o ritmo, a parada em cada marca de tick, as diferenças com a mensagem certa, o fim e a repetição, com gravações montadas e um relógio de teste (CA-05). Verificação: `verify`.
- [ ] T11: O alvo de fuzzing `muonline_recording`, que lê e reproduz, com as sementes pequenas e inválidas (CA-09). Verificação: `muonline_recording_seeds` no `verify`.
- [ ] T12: As opções `--record`, `--server-version` e `--replay` do `MuClient`, só no build de desenvolvimento, com as recusas e os testes de execução (CA-04, CA-05, CA-08). Verificação: `verify`, com o build de jogador.
- [ ] T13: O quadro sem alocação gravando e reproduzindo, com pontos de controle dentro dos 1.000 quadros (CA-10). Verificação: `muonline_frame_allocation_tests`.
- [ ] T14: A gravação de referência do OpenMU 0.10.1 local (`openmu-two-players.mucap`), sem conta e sem senha, o `.gitattributes`, os dois testes de replay e a semente do fuzzing (CA-07, CA-09). Verificação: `verify` e a conferência do conteúdo do arquivo.
- [ ] T15: O teste de integração `openmu_muclient_records_and_replays`, que grava 20 s e reproduz (CA-12). Verificação: o teste contra o servidor local.
- [ ] T16: A documentação: ARCHITECTURE, a página nova `mu-recording` com o formato byte a byte, `mu-online`, `mu-world`, `platform`, `core`, `docs/setup/DEV.md` e TECH_DEBT (sai o TD-0037) (CA-13). Verificação: checagens `docs` e `structure` no `verify`.

## Etapas

Depois das tarefas, cada etapa do ciclo marca a sua linha quando termina, com a data e, na revisão, o commit. O hook do início da sessão lê esta lista para dizer a etapa e o próximo passo (`docs/WORKFLOW.md`).

- [ ] Revisão (`/revisar`)
- [ ] Fechamento (`/fechar`)
- [ ] Teste do dono
- [ ] Retrospectiva (`/retrospectiva`)

## Decisões do dono

Cada escolha que o dono fez durante a fatia, com as palavras dele e a tarefa: o que mudou e por quê. Nenhuma decisão do dono fica só no meio do texto de uma tarefa.

- **2026-11-26, antes da lista de tarefas:** "Nesta sessão". O `/implementar` começou na mesma sessão do `/situacao`.
- **2026-11-26, antes da T01:** "Nesta sessão" e "Sim, ligue". A T01 foi feita nesta sessão, e o aviso automático da CI (Auto-fix) do PR #12 foi ligado nesta sessão.
- **2026-11-26, antes da T02:** "Nesta sessão". A T02 foi feita nesta sessão. A trava `scripts/checks/` mudou pela decisão 6 da spec, aprovada pelo dono.
- **2026-11-27, antes da T03:** "Nesta sessão". A T03 foi feita nesta sessão.
- **2026-11-27, entre a T03 e a T04:** "Em outra sessão (eu abro)". A T04 começa numa sessão nova, aberta pelo dono; lá, o aviso automático da CI (Auto-fix) do PR #12 precisa ser ligado de novo.

## Bloqueios

*(nenhum)*

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0.

## Revisão

Achados do revisor e o que foi feito com cada um, uma linha por achado. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

Contexto que a próxima sessão precisa saber e que não está em outro lugar.

Os desvios do plano e as travas alteradas não ficam aqui: vão direto para o report.

- **Onde parou (2026-11-27):** a T01, a T02 e a T03 estão feitas e enviadas, com o `verify` verde em cada uma. A próxima é a **T04**, numa sessão nova aberta pelo dono: no começo dela, pergunte se a T04 começa ali e ofereça ligar o Auto-fix do PR #12 de novo.
- **Desenho da T04, para quem começa:** o plano pede `ReadFileBytes(caminho, limite)` e `WriteFileBytes(caminho, bytes)` em `Nexum/Platform/FileBytes.hpp`, as duas pelo SDL. O `Engine/Platform/Private/Png.cpp` já abre arquivos pelo SDL (`SDL_IOFromFile`, com o caminho em UTF-8 pelo `ToUtf8` e o `Stream`, um `unique_ptr` que fecha com `SDL_CloseIO`), os dois no espaço sem nome do arquivo: a T04 pode levá-los para um cabeçalho de `Private/` e usar dos dois lados. A leitura confere o tamanho (`SDL_GetIOSize`) antes de ler e recusa acima do limite sem ler nada (o CA-03 pede 64 MiB para o `.mucap`). Os testes de recusa conferem o motivo exato, e a conferência do limite leva a sabotagem (é um leitor de dados de fora). A escrita é usada pelo `MuClient` fora do laço dos quadros (INV-06).
- **PR em rascunho:** o [PR #12](https://github.com/ecologycrw/nexum/pull/12) foi aberto em 2026-11-26, logo depois da lista de tarefas (P-0019). O aviso automático da CI (Auto-fix) vale só para a sessão que o ligou: numa sessão nova, ofereça ligar de novo. O `/fechar` atualiza a descrição e tira do rascunho; nunca abra um segundo PR.
- **As regras da retrospectiva da fatia 1 valem aqui:** o PR fica em rascunho desde a lista de tarefas (P-0019), e todo teste de recusa confere o motivo exato, com a sabotagem onde há risco real de falso positivo, registrada no corpo do commit (P-0020, `docs/CONVENTIONS.md`, "Testes").
- **O resumo de segurança do PR #12** lista 42 linhas novas com `NOLINT` (conferido com `py scripts/checks/run.py pr-summary --base origin/main` depois da T02): todas são do código e dos testes da própria checagem, e nenhuma é uma exceção nova no C++. O report do `/fechar` diz isso.
- **O FNV-1a (T03)** é só um cabeçalho (`Nexum/Core/Fnv1a.hpp`). A análise estática só o lê quando um `.cpp` fora dos testes o incluir: a impressão do estado (T08) é a primeira.
- **Armadilhas desta máquina,** herdadas da fatia 1 ([tasks da fatia 1](../0001-rede-ate-o-mundo/tasks.md#notas-para-a-próxima-sessão)): o `ColumnLimit` é 110, então rode `py scripts/dev.py format --fix` antes do build; o `guard-bash` bloqueia um comando que cita uma trava e também grava um arquivo; o MSVC não acusa as conversões de sinal que o Clang e o GCC acusam; o `file(WRITE)` do CMake grava `\r\n` no Windows; não rode um build enquanto o `verify` roda em segundo plano.
- **O servidor de teste** sobe com `docker compose -f tests/integration/openmu/compose.yaml up -d`, nos processadores 0 a 5, e fica pronto em poucos segundos. O servidor do laboratório da central usa as mesmas portas: só um fica ligado por vez. Os testes `openmu` leem `NEXUM_OPENMU_ADDRESS`.

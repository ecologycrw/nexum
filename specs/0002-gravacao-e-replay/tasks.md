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
- [x] T04: `ReadFileBytes`, com o limite conferido antes de ler, e `WriteFileBytes` (`Engine/Platform`, pelo SDL) (CA-03, CA-04). Verificação: `verify`.
  - `Nexum/Platform/FileBytes.hpp`: o arquivo acima do limite é recusado sem pedir memória para o conteúdo (programa de teste próprio, com o contador de memória); o `ToUtf8` e o `Stream` do PNG foram para `Private/SdlFile.hpp`; 10 casos novos, com 3 sabotagens. Commit: este.
- [x] T05: O adaptador de servidor (`Mu/Online/ServerAdapter.hpp`), o `LiveServer` com as peças de hoje e o `OnlineClient` só pelo contrato; os testes da fatia 1 montam o `LiveServer`, e um adaptador de teste prova a ordem (CA-01). Verificação: `verify`.
  - O contrato com os 4 estados e o motivo; o `LiveServer` com a sessão e a tradução; o `OnlineClient` só com a fila, a visão e o tempo no mundo, e o `ApplyEvents(now)` avisa o servidor. 3 casos novos e 2 conferências a mais nos da fatia 1, com 3 sabotagens; os 2 testes `openmu` passaram no OpenMU local. Commit: este.
- [x] T06: A época nos eventos (TD-0037): a fila guarda a época, a tradução recebe a da mensagem, e o tick descarta e conta os velhos, com a sabotagem (CA-02). Verificação: `verify`.
  - A fila guarda cada evento com a época (`Mu/Session/SessionEpoch.hpp`, agora com a ordem), a tradução a recebe da mensagem, e o tick descarta, conta e registra uma vez os velhos; o servidor ouve quantos o tick tirou. 5 casos novos, com 2 sabotagens. Commit: este.
- [x] T07: O formato `.mucap` (`Mu/Recording/Recording.hpp`): a escrita num espaço reservado e a leitura com cada recusa e o motivo exato, os bytes fixos de uma gravação pequena e a sabotagem do tamanho e da ordem (CA-03). Verificação: `verify`.
  - O `RecordingWriter` grava num espaço reservado, sempre com lugar para o fim; o `ParseRecording` confere tudo na ordem do plano e recusa com o motivo exato; o cursor lê os registros sem pedir memória. 15 casos novos, com 3 sabotagens. Commit: este.
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
- **2026-11-27, na sessão nova, antes da T04:** "Sim, com Auto-fix". A T04 começou nesta sessão, e o aviso automático da CI (Auto-fix) do PR #12 foi ligado de novo aqui.
- **2026-11-27, entre a T04 e a T05:** "Nesta sessão". A T05 foi feita nesta sessão.
- **2026-11-27, entre a T05 e a T06:** "Em outra sessão". A T06 começa numa sessão nova, aberta pelo dono; lá, o aviso automático da CI (Auto-fix) do PR #12 precisa ser ligado de novo.
- **2026-11-27, na sessão nova, antes da T06:** "Sim, com Auto-fix". A T06 foi feita nesta sessão, e o aviso automático da CI (Auto-fix) do PR #12 foi ligado de novo aqui.
- **2026-11-27, entre a T06 e a T07:** "Nesta sessão". A T07 foi feita nesta sessão.

## Bloqueios

*(nenhum)*

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0.

## Revisão

Achados do revisor e o que foi feito com cada um, uma linha por achado. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

Contexto que a próxima sessão precisa saber e que não está em outro lugar.

Os desvios do plano e as travas alteradas não ficam aqui: vão direto para o report.

- **Onde parou (2026-11-27):** a T01 a T07 estão feitas e enviadas, com o `verify` verde em cada uma. A próxima é a **T08** (a impressão do estado). O Auto-fix do PR #12 foi ligado na sessão da T06 e da T07; numa sessão nova, ofereça ligar de novo e lembre o dono de desligar o da sessão anterior, para as duas não corrigirem a mesma falha.
- **O formato (T07), para a T09, a T10 e a T12:** o `RecordingWriter::Create(cabeçalho, espaço)` reserva o espaço uma vez e só se move (uma cópia perderia o espaço). O `Write(registro)` devolve falso quando não coube, e daí em diante o escritor fica cheio e só aceita o fim, que sempre cabe; devolve falso também depois do fim e para uma mensagem fora de 3 a 65.535 bytes. O `ParseRecording(bytes)` e o `ReadRecording(caminho)` devolvem a `Recording`, que guarda os bytes e dá um cursor (`Records()`) e as contagens (`Messages()`, `Ticks()`, `Checkpoints()`, `End()`). Os motivos da leitura não citam o caminho: o `MuClient` da T12 junta o caminho. O `MuOnline` passou a ter o `Nexum::Platform` como dependência privada. A página `mu-recording` fica para a T16.
- **A época (T06), para a T09 e a T10:** o `EventsApplied(view, count, now)` recebe quantos eventos o tick tirou da fila, aplicados ou descartados, como o registro 4 (tick) do `.mucap`, e é chamado mesmo quando todos foram descartados. O `OnlineClient::DroppedEvents()` conta os descartados desde o começo. O TD-0037 continua no TECH_DEBT e nas páginas `mu-world` e `mu-online`, e sai na T16; a `mu-world` e a `mu-translate` já descrevem a fila e a tradução com a época.
- **O adaptador (T05), para a T10:** o contrato tem o `CurrentEpoch()`, e o `LiveServer` devolve a época da sessão. O `ServerStatus::reason` é um `string_view` que vale até a próxima chamada ao servidor. O estado "terminou" vira o `OnlineStatus::ServerEnded`, que o `FoundationApp` encerra com sucesso: é o fim do replay. A `mu-online.md`, a `mu-session.md` e a `mu-translate.md` ainda descrevem o `OnlineClient` da fatia 1 e ficam para a T16, que no plano não cita as duas últimas.
- **A leitura do arquivo (T04), para a T07:** `ReadFileBytes(caminho, limite)` recusa com a mensagem exata "the file <caminho> has <N> bytes, more than the limit of <L>", antes de pedir memória. A recusa dos 64 MiB do CA-03 usa esse limite, e o teste do formato pode conferir essa mensagem. O teste de memória fica em `nexum_platform_file_bytes_allocation_tests`.
- **Rode os programas de teste pelo `ctest --test-dir build/windows-dev`,** nunca pelo `.exe` a partir da pasta do projeto: os testes gravam as pastas de saída na pasta atual. E compile pelo `py scripts/dev.py build`: o `cmake --build` direto fica sem o ambiente do compilador da Microsoft.
- **PR em rascunho:** o [PR #12](https://github.com/ecologycrw/nexum/pull/12) foi aberto em 2026-11-26, logo depois da lista de tarefas (P-0019). O aviso automático da CI (Auto-fix) vale só para a sessão que o ligou: numa sessão nova, ofereça ligar de novo. O `/fechar` atualiza a descrição e tira do rascunho; nunca abra um segundo PR.
- **As regras da retrospectiva da fatia 1 valem aqui:** o PR fica em rascunho desde a lista de tarefas (P-0019), e todo teste de recusa confere o motivo exato, com a sabotagem onde há risco real de falso positivo, registrada no corpo do commit (P-0020, `docs/CONVENTIONS.md`, "Testes").
- **O resumo de segurança do PR #12** lista 42 linhas novas com `NOLINT` (conferido com `py scripts/checks/run.py pr-summary --base origin/main` depois da T02): todas são do código e dos testes da própria checagem, e nenhuma é uma exceção nova no C++. O report do `/fechar` diz isso.
- **O FNV-1a (T03)** é só um cabeçalho (`Nexum/Core/Fnv1a.hpp`). A análise estática só o lê quando um `.cpp` fora dos testes o incluir: a impressão do estado (T08) é a primeira.
- **Armadilhas desta máquina,** herdadas da fatia 1 ([tasks da fatia 1](../0001-rede-ate-o-mundo/tasks.md#notas-para-a-próxima-sessão)): o `ColumnLimit` é 110, então rode `py scripts/dev.py format --fix` antes do build; o `guard-bash` bloqueia um comando que cita uma trava e também grava um arquivo; o MSVC não acusa as conversões de sinal que o Clang e o GCC acusam, nem um campo que falta num inicializador com nomes (`.campo = ...`): dê todos os campos, como mostrou a CI da T05; o `file(WRITE)` do CMake grava `\r\n` no Windows; não rode um build enquanto o `verify` roda em segundo plano, e lembre que o hook do fim do turno também compila: se o turno acabar com o `verify` em segundo plano, os dois brigam pela pasta de build ("failed recompaction: Permission denied"), e o hook acusa uma falha que não é do código.
- **O servidor de teste** sobe com `docker compose -f tests/integration/openmu/compose.yaml up -d`, nos processadores 0 a 5, e fica pronto em poucos segundos. O servidor do laboratório da central usa as mesmas portas: só um fica ligado por vez. Os testes `openmu` leem `NEXUM_OPENMU_ADDRESS`.

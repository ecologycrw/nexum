# Tarefas da fatia 1

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui. Por isso ele é curto. O detalhe de cada tarefa vai para a mensagem do commit, e as provas, para o report.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

Cada tarefa tem a primeira linha no formato abaixo, que o hook do início da sessão lê, e no máximo 2 linhas depois dela: o resultado em uma frase e o commit.

- [x] T01: O Asio no `vcpkg.json` (trava), o `find_package` na raiz e o registro no THIRD_PARTY (CA-06, CA-15). Verificação: `verify` com o Asio instalado pelo vcpkg.
  - O Asio 1.38.2 entrou pelo vcpkg e é achado na raiz do CMake; `verify` verde nos 3 builds. Commit: este.
- [x] T02: Os XML do OpenMU em `third_party/openmu`, copiados pelo dono, com o aviso MIT e a base `b53c704c6`, registrados no THIRD_PARTY (CA-03, CA-15). Verificação: checagem `structure` e os arquivos iguais aos da central.
  - Os 7 arquivos e a licença, copiados pelo dono, têm as mesmas linhas da central; checagens `docs`, `structure` e `terms` aceitando. Commit: este.
- [x] T03: O `packetgen` (`Tools/PacketGen`, Python só com a biblioteca padrão), a função do CMake que gera o código no build (trava `cmake/`), os testes gerados e o ADR 0034 (CA-03). Verificação: testes do gerador e os testes gerados no `verify`.
  - O `packetgen` gera os 613 pacotes dos 4 XML (cabeçalhos na pasta de build) e um teste por pacote; o XML de teste próprio confere cada tipo de campo; `verify` verde (29, 30 e 24 testes). Commit: este.
- [x] T04: O dialeto 106.3 (`Mu/Protocol`): a tabela de variantes, um teste por linha da §5.2 e a configuração única com a versão `20404` e o número de série `NEXUM-CLIENT-001` (CA-03, CA-12). Verificação: `verify`.
  - `Mu/Protocol/Dialect.hpp`: um apelido para cada variante da §5.2, a identidade do cliente e a porta 44406; 22 testes (um por linha e a identidade). Commit: este.
- [x] T05: O enquadramento `C1` a `C4`, incremental, com os exemplos do plano (CA-01). Verificação: `verify`.
  - `FrameReader`: entrega as mensagens dos 4 tipos em pedaços de 1 byte até 65.535, sem cópia quando a mensagem chega inteira; 7 testes. Commit: este.
- [x] T06: A cifra: SimpleModulus com o contador de cada direção, Xor32 e Xor3, com os vetores da central (CA-02). Verificação: `verify`.
  - `Mu/Protocol/Cipher`: SimpleModulus com o contador de cada direção, Xor32 e Xor3; os vetores V1 a V9 e os 5 das capturas reais passam (13 testes). Commit: este.
- [x] T07: O item estendido e a aparência estendida, cada um num componente neutro (CA-04). Verificação: `verify`.
  - `ReadExtendedItem` e `ReadExtendedAppearance` viram `mu::items::Item` e `mu::characters::Appearance`; as 32 + 224 combinações, as bordas, a captura e os dados cortados passam (14 testes). Commit: este.
- [x] T08: Os pacotes de exemplo da central lidos e escritos de novo, com a contagem por grupo (CA-05). Verificação: `verify`.
  - 13 casos (169 conferências): os pacotes da sessão, os andares V1 a V6 com cada passo, o eco e o login igual ao da captura, a não ser pelo número de série (`WriteLogin`). Commit: este.
- [x] T09: A estrutura de fuzzing (`NEXUM_FUZZING` em `cmake/`, o preset no `CMakePresets.json` e o passo da CI, travas) e um alvo por leitor; o `LoadPng` confere o cabeçalho (TD-0020, TD-0023) (CA-10). Verificação: `verify` e as sementes nos testes.
  - 7 alvos com sementes (`*_seeds` nos testes); o `LoadPng` recusa pelo cabeçalho; a CI roda cada alvo por 60 s (só prova num PR); TD-0026. Commit: este.
- [x] T10: O transporte `Engine/Net`: a conexão TCP com o Asio, a thread de rede, as filas e a interface da transformação de bytes (CA-06). Verificação: `nexum_net_tests` e as checagens `architecture` e `terms`.
  - `Connection` (thread própria, fila de entrada e fila única de saída, Asio em passos de 5 ms), `MessageCodec` e o servidor de teste `LoopbackServer` numa biblioteca só de testes; 9 casos passam 15 vezes seguidas. Commit: este.
- [x] T11: A sessão (`Mu/Session`) de "Desconectado" a "No mundo", com os prazos, a época, os eventos guardados no carregamento e o log que mascara as credenciais (CA-07, CA-12). Verificação: `verify`.
  - `Session` (prazos de 10 s, época por conexão, mensagens guardadas até o `FinishLoading`), `ClientCodec`, `MessageQueue` e `FormatPacket` com as credenciais escondidas; 25 casos novos; TD-0027 e TD-0028. Commit: este.
- [x] T12: O ping a cada 20 s de relógio de parede, desde a entrada no Game Server (CA-08). Verificação: `verify`.
  - `PingClock` e `WritePing` (`Mu/Session/Ping`), chamados pelo `Session::AdvanceClock` a cada quadro, com as velocidades do `C3 F3 03` e do `C1 26 FF`; 7 casos novos. Commit: este.
- [x] T13: A tradução para eventos neutros e a visão do mundo (`Mu/Translate` e `Mu/World`) (CA-09). Verificação: `verify`.
  - `Translator` (9 pacotes viram eventos; os da §6.12 ignorados em silêncio; código desconhecido no log uma vez), `EventQueue`, `WorldView` e o `LogLine` do `Engine/Debug`; 30 casos novos; TD-0029 a TD-0031. Commit: este.
- [x] T14: O `MuClient` com as opções novas, a senha sem eco (`Engine/Platform`) e a sessão dentro do laço (CA-12). Verificação: `verify`.
  - `--connect`, `--account`, `--server`, `--character` e `--world-seconds`; `ReadHiddenLine` (sem eco, sem `--password`); `Mu/Online` (`NetworkTransport` e `OnlineClient`) nos passos 2 e 3 e no tick; 17 casos novos, 4 testes do `MuClient` e o da senha com 9 entradas; TD-0032 e TD-0033. Commit: este.
- [x] T15: A janela minimizada: o laço espera o próximo tick (TD-0015) (CA-13). Verificação: teste com relógio de teste no `verify`.
  - `TickClock::UntilNextTick`, `Window::IsVisible` e `WaitFor` (`SDL_DelayNS`); fora da tela, o quadro não vai para a placa e espera o próximo tick: minimizado, de 92% para 0,3% de um núcleo, com 25 ticks por segundo; 7 casos novos. Commit: este.
- [x] T16: O quadro sem alocação com mensagens de rede chegando por um transporte de teste (CA-11). Verificação: `muonline_frame_allocation_tests`.
  - `FrameTransport` leva o cliente ao mundo e entrega 5 mensagens do mundo em cada um dos 1.000 quadros; o `CheckPacket` e o `StartPacket` só montam texto num erro (eram 10.003 pedidos, agora 0). Commit: este.
- [ ] T17: O OpenMU 0.10.1 em `tests/integration/openmu` (Docker, fixado pela impressão digital) e o teste de integração de 3 minutos (CA-14). Verificação: o teste contra o servidor local.
- [ ] T18: A documentação: ARCHITECTURE, páginas dos módulos, THIRD_PARTY, ADR 0034 e TECH_DEBT (saem o TD-0015, o TD-0020 e o TD-0023) (CA-15). Verificação: checagens `docs` e `structure` no `verify`.

## Etapas

Depois das tarefas, cada etapa do ciclo marca a sua linha quando termina, com a data e, na revisão, o commit. O hook do início da sessão lê esta lista para dizer a etapa e o próximo passo (`docs/WORKFLOW.md`).

- [ ] Revisão (`/revisar`)
- [ ] Fechamento (`/fechar`)
- [ ] Teste do dono
- [ ] Retrospectiva (`/retrospectiva`)

## Decisões do dono

Cada escolha que o dono fez durante a fatia, com as palavras dele e a tarefa: o que mudou e por quê. Nenhuma decisão do dono fica só no meio do texto de uma tarefa.

- **2026-11-25, antes da T01:** "Sim, todas". O dono aprovou de uma vez as travas da fatia: o Asio no `vcpkg.json` (T01, com o download pelo vcpkg), a função do `packetgen` em `cmake/` (T03) e o fuzzing em `cmake/`, no `CMakePresets.json` e no `.github/` (T09). A checagem `structure` não muda. O dono já copiou os XML do OpenMU para `third_party/openmu` (T02).
- **2026-11-26, entre a T10 e a T11:** "Sim, abra o PR" e "Sim, ligue". O PR #11 foi aberto em rascunho para a CI rodar no Linux e no macOS, com o aviso automático da CI (Auto-fix) ligado nesta sessão.
- **2026-11-26, entre a T10 e a T11:** "Sim, pode mudar". O `tidy` do `scripts/dev.py` (trava) compila antes de analisar, porque a análise estática da CI não achava os cabeçalhos gerados.
- **2026-11-26, entre a T13 e a T14:** "Sim, registre a proposta" e, depois, "Faça aqui ainda nessa sessão a consolidação da proposta P-0016" e "Escolha a opção 1". O código gerado pesava mais de 1 GB por compilação e enchia a memória do build. O `packetgen` passou a gerar um cabeçalho por pacote, a trava `cmake/NexumPacketGen.cmake` mudou, e o dialeto ficou em quatro partes. O dono pediu também que nenhum cabeçalho junte todos os pacotes e que os testes gerados continuem cobrindo todos eles, com os tempos novos para comparar (report, "Desvios do plano").
- **2026-11-26, entre a T15 e a T16:** "Nesta sessão" e "Sim, ligue". A T16 foi feita na mesma sessão da T15, e o aviso automático da CI (Auto-fix) do PR #11 foi ligado de novo nesta sessão.
- **2026-11-26, entre a T16 e a T17:** "Em outra sessão (eu abro)". A T17 começa numa sessão nova, aberta pelo dono; lá, o aviso automático da CI precisa ser ligado de novo.

## Bloqueios

*(nenhum)*

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0.

## Revisão

Achados do revisor e o que foi feito com cada um, uma linha por achado. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

- **Onde parou (2026-11-26):** T01 a T16 feitas. A próxima é a T17 (o OpenMU 0.10.1 no Docker e o teste de integração de 3 minutos). Entre uma tarefa e a seguinte, pergunte ao dono num cartão se segue nesta sessão ou numa sessão nova, aberta por ele. A T17 baixa a imagem do OpenMU e pede o "sim" do dono antes do download.
- **Desenho da T16 (feito), para a T17:** o teste do quadro (`FrameAllocationTest.cpp`) tem o `FrameTransport`, que implementa o `Transport` com as mensagens na memória: os dois primeiros quadros levam o cliente ao mundo, e depois cada quadro entrega as mesmas 5 mensagens do mundo (`NpcCameIntoView`, `HeroWalked`, `HeroPlaced`, `CurrentStats` e `NpcLeftTheView`, em `SessionMessages.hpp`). Uma alocação nova no caminho das mensagens aparece ali, com a contagem.
- **Desenho da T15 (feito), para a T18:** o `FoundationApp::Iterate` lê o `Window::IsVisible` uma vez por quadro. Fora da tela, o quadro roda os ticks e monta o desenho, mas não chama o `Present`, e no fim espera o `TickClock::UntilNextTick` com o `platform::WaitFor`. A T18 tira o TD-0015 do TECH_DEBT e da lista de itens provisórios de `docs/modules/platform.md`. Para medir o processador com a janela minimizada, sem conexão, um script do PowerShell inicia o `MuClient`, minimiza a janela com o `ShowWindow` do `user32` e lê o `TotalProcessorTime` do processo; o build `windows-dev` é Debug e mede mais alto que o de jogador. Não rode um build enquanto o `verify` roda em segundo plano: a checagem do fim do turno compila na mesma pasta, e as duas colidem ("failed recompaction").
- **Desenho da T14 (feito), para a T15, a T16 e a T17:** o `main` lê a senha com `nexum::platform::ReadHiddenLine` antes de abrir a janela e monta as opções da sessão com `MakeSessionOptions` (`Mu/App/ClientOptions`). O `FoundationApp` guarda o `NetworkTransport` e o `OnlineClient` (`Mu/Online`) e liga o cliente à tela com `FoundationScreen::Attach`: o passo 2 do quadro chama `TakeNetwork`, o passo 3 chama `AdvanceClock` e cada tick começa com `ApplyEvents`. O `Transport` (`Mu/Online/Transport.hpp`) é a interface que a T16 implementa com um transporte de teste, como o `ScriptedTransport` do `OnlineClientTest.cpp`. As mensagens de servidor dos testes ficam em `Games/MuOnline/Tests/SessionMessages.hpp`. A última linha do log é o `OnlineClient::Summary` ("the session ends: ..."). Com `--world-seconds N`, o cliente encerra com sucesso depois de N segundos no mundo: a T17 usa `--world-seconds 180` e passa a senha pela entrada padrão, como os testes do `MuClient` em `Games/MuOnline/App/CMakeLists.txt`.
- **O GCC acusa o que o clang-tidy não acusa:** `std::size_t count; count += cond ? 1 : 0;` passou no clang-tidy com `-Wsign-conversion` e quebrou a CI no GCC (T14). Some com tipos sem sinal (`std::size_t{1}`) ou use `std::ranges::count_if`.
- **Armadilha do CMake no Windows:** o `file(WRITE)` grava cada `\n` como `\r\n`. Para gravar finais de linha exatos, use `file(CONFIGURE OUTPUT ... CONTENT ... NEWLINE_STYLE LF|CRLF)`, como em `Engine/Platform/Tests/CheckHiddenLine.cmake`.
- **Desenho da T13 (feito), para a T14 e a T16:** no passo 2 do quadro, o app chama `TakeWorldMessages` e passa cada mensagem ao `Translator::Translate(mensagem, fila)` (`Mu/Translate`), que põe os eventos neutros (`Mu/World/WorldEvent.hpp`) numa `EventQueue`; no tick, `fila.Drain` entrega cada evento ao `WorldView::Apply` (`Mu/World`). O `Translator` nasce com o id do herói (`Session::PlayerId()`, depois do `GameServerEntered`). A última linha do log pode contar `WorldView::Objects().size()`. A visão e a tradução montam o log com o `nexum::debug::LogLine` (`Engine/Debug`), sem pedir memória; e, desde a T16, o `CheckPacket` e o `StartPacket` só montam texto para descrever um erro. A T18 documenta o `LogLine` na página do Debug e cria as páginas do `Translate` e do `World`.
- **Os pacotes e o dialeto desde a P-0016:** cada pacote gerado é um cabeçalho (`Mu/Protocol/Packets/<Arquivo>/<Pacote>.hpp`), e ninguém inclui esses cabeçalhos fora do dialeto. O dialeto está em `Mu/Protocol/Dialect/` (`ConnectServer.hpp`, `Login.hpp`, `FirstEntry.hpp` e `World.hpp`); inclua só as partes que o arquivo usa. O `Mu/Protocol/Dialect.hpp` tem só a identidade do cliente e a porta. Nenhum cabeçalho pode voltar a juntar todos os pacotes (os testes do `packetgen` conferem os gerados).
- **Desenho da T11 e da T12 (feito), para a T13 e a T14:** a sessão (`Mu/Session/Session.hpp`) recebe cada evento com `Handle(época, evento, agora)` e manda pela `SessionLink` (`Open`, `Send` e `Close`), que o app implementa com uma `Connection` e um `ClientCodec` novos por conexão, guardando a época de cada uma. O app chama `AdvanceClock(agora)` a cada quadro (os prazos e o ping) e `FinishLoading()` quando a sessão está em "Carregando" com os dados do personagem (nesta fatia, logo em seguida). O tick tira as mensagens do mundo com `TakeWorldMessages`, primeiro o `C3 F3 03`, e a T13 as traduz.- **Os rascunhos da pasta de notas da sessão que começou a fatia** já entraram todos no repositório; não há mais nada lá para usar.
- **Desenho da T10 (feito):** a sessão não deve possuir a `Connection`: o app tira os eventos com `Poll` e os entrega à sessão, e a sessão manda por uma interface de saída; assim os testes da T11 alimentam a sessão sem rede. a thread de rede roda o Asio em passos de 5 ms (`run_for`) e olha a fila de saída entre os passos, porque acordá-la pelo Asio pediria memória a cada mensagem na thread principal. As filas são pares de buffers reservados ao abrir, trocados sob mutex. Os testes do `Engine/Net` não podem incluir o Asio (só `Engine/Net/Private` pode), por isso o servidor de teste (`LoopbackServer`) fica na API pública do `Engine/Net`.
- **Armadilhas desta máquina:** o `ColumnLimit` do `.clang-format` é 110; rode `py scripts/dev.py format --fix` antes do build. O `guard-bash` bloqueia um comando que cita `scripts/dev.py` (ou outra trava) e também grava um arquivo: faça em comandos separados. Para ver o resultado do `verify`, use `set -o pipefail` e `echo EXIT=$?`, porque o `tail` esconde o código de saída. O MSVC recusa `for (std::uint8_t x : {0x01, ...})` (conversão de `int`): use um `std::array<std::uint8_t, N>`. O MSVC não acusa as conversões de sinal que o Clang e o GCC acusam: antes de enviar, rode o `clang-tidy` do Visual Studio com `--checks=-*,clang-diagnostic-*,cert-dcl58-cpp` e `--extra-arg=-Wsign-conversion` (e `-Wconversion`, `-Wshadow`).
- **Código gerado:** fica em `build/windows-dev/Games/MuOnline/Generated/Mu/Protocol/Packets/`; os nomes dos campos são os do XML (por exemplo, `dialect::Login::Writer::SetUsername`). As travas alteradas e os desvios do plano vão para o `report.md`, que já existe como rascunho.
- **PR em rascunho:** o [PR #11](https://github.com/ecologycrw/nexum/pull/11) foi aberto em 2026-11-26, a pedido do dono, para a CI rodar no Linux e no macOS; depois de duas correções, os 10 checks passaram, com o fuzzing. O aviso automático da CI (Auto-fix) vale só para a sessão que o ligou: numa sessão nova, ofereça ligar de novo. O `/fechar` atualiza a descrição e tira do rascunho; nunca abra um segundo PR. A primeira CI achou uma conversão de sinal no `CipherFuzzTest.cpp` (corrigida) e a análise estática sem os cabeçalhos gerados: o `tidy` do `scripts/dev.py` (trava) só configurava, e o job da CI não compilava antes. Agora o `tidy` compila antes, com o "sim" do dono.
- A central fica no caminho de `.claude/central.local.json`. Só a leitura é liberada, pelas ferramentas de arquivo (o terminal não chega lá), e `Client/` é proibido. A spec de protocolo é `docs/specs/protocolo/protocolo-106-3-m2.md`, e os achados ficam em `knowledge/protocolo/`.
- A checagem `architecture` já reserva o Asio para o `Engine/Net` (`Private/` e `CMakeLists.txt`). A checagem `structure` só confere a tabela "Em uso" do THIRD_PARTY contra o `vcpkg.json`: os XML do OpenMU ficam registrados numa lista fora dessa tabela, como o Tracy, sem mudar a checagem.
- O servidor do laboratório da central (`nexum-lab-openmu`) usa as mesmas portas do servidor de teste: só um fica ligado por vez. Todo contêiner roda com `--cpuset-cpus=0-5`.

# Relatório da fatia 2: A gravação e o replay

## Resumo para o dono

O `MuClient` grava a sessão no mundo num arquivo `.mucap` (`--record`) e depois a reproduz sem nenhum servidor ligado (`--replay`). O replay passa pelas mesmas peças da sessão ao vivo e confere, a cada tick e a cada ponto de controle, se o mundo ficou igual ao gravado. No fim, o terminal diz se tudo bateu. Uma gravação de 20 segundos de um OpenMU de verdade está no repositório, e a CI a reproduz nos 3 sistemas a cada mudança. Assim, um erro na tradução dos pacotes aparece sem precisar de servidor.

Por baixo, a fatia trouxe o adaptador de servidor: o cliente online só fala com o servidor por um contrato, com duas implementações, o OpenMU ao vivo e o replay. Trouxe também a época dentro de cada evento (o TD-0037 saiu), o formato `.mucap` versão 1, a impressão do estado do mundo e a trava que confere os `NOLINT` novos. Com ela, a parte 1 do M2 está completa. A janela continua com o cartão de teste da fatia 0.

## Critérios de aceitação

As provas automáticas rodam em todo `py scripts/dev.py verify` e na CI. A última execução da CI antes do fechamento foi a [46067181075](https://github.com/ecologycrw/nexum/actions/runs/46067181075), do commit `56f8138`, com todo o código da fatia e as correções da revisão: verde nos 10 checks, com o fuzzing. O `verify` do fechamento, nesta sessão, está no fim desta seção.

| CA | Prova | Resultado |
|---|---|---|
| CA-01 | 3 testes com a etiqueta `[0002-CA-01]` em `Games/MuOnline/Tests/OnlineClientTest.cpp`: o servidor ao vivo fica "conectando" até o mundo e depois "no mundo", sem motivo; o cliente online aplica os eventos de qualquer servidor na ordem, só no tick (com um adaptador de teste, que não tem sessão nem rede); e o estado do cliente segue o do servidor nos quatro estados, com o motivo da falha. Os testes da fatia 1 do cliente online e do quadro sem alocação passaram a montar o `LiveServer`, com as mesmas conferências (só mudou de quem se pergunta o estado da sessão). O contrato está em `Mu/Online/ServerAdapter.hpp`, e a conta e a senha só entram no `LiveServer` | passou |
| CA-02 | 5 testes com a etiqueta `[0002-CA-02]`: em `TranslatorTest.cpp`, cada evento sai da tradução com a época da mensagem; em `WorldViewTest.cpp`, a fila guarda cada evento com a época, em ordem, e uma época menor é mais velha; em `OnlineClientTest.cpp`, o tick descarta e conta os eventos de uma época mais velha e registra isso uma vez, e muitos eventos velhos num tick dão uma única linha de log. A sabotagem (a conferência da época desligada) fez o teste do descarte falhar, como registra o commit `87fa0af` | passou |
| CA-03 | 17 testes com a etiqueta `[0002-CA-03]` em `Games/MuOnline/Tests/RecordingTest.cpp`: os bytes fixos de uma gravação pequena, escrita e lida de novo com cada campo; cada recusa da lista do CA-03 com o motivo exato (a marca, a versão, o texto longo ou cortado, o registro cortado, de tipo desconhecido ou de tamanho errado, o tempo voltando, a mensagem antes da entrada, o ponto de controle sem tick ou com a contagem errada, algo depois do fim e a falta do fim); e, desde a revisão, os textos do cabeçalho em UTF-8, sem caracteres de controle. O arquivo acima de 64 MiB é recusado pelo tamanho, e um de 64 MiB é conferido. 4 testes em `Engine/Platform/Tests` (2 em `FileBytesTest.cpp` e 2 em `FileBytesAllocationTest.cpp`): o arquivo acima do limite é recusado, com a mensagem exata, antes de pedir memória para o conteúdo | passou |
| CA-04 | 10 testes com a etiqueta `[0002-CA-04]` em `Games/MuOnline/Tests/RecorderTest.cpp`: o servidor ao vivo grava, desde a entrada no mundo, cada mensagem, tick e ponto de controle; nada antes da entrada; o ponto de controle no primeiro tick a partir de cada 5 s no mundo; a mudança de época; o fim com o seu ponto de controle uma vez só; o espaço cheio, com um aviso, a gravação válida e o fim "cheio"; e a gravação sem a conta, sem a senha e sem nada do que o cliente envia, com um caso de controle que mostra que a busca acha o login quando ele está lá. 3 testes em `Engine/Platform/Tests/FileBytesTest.cpp` (a escrita do arquivo inteiro e o erro com o caminho). Os testes de execução `muclient_refuses_a_recording_it_cannot_write` (o caminho ruim é recusado antes de conectar) e `muclient_records_a_session_that_never_reaches_the_world`, em `Games/MuOnline/App/CMakeLists.txt` | passou |
| CA-05 | 9 testes com a etiqueta `[0002-CA-05]` em `Games/MuOnline/Tests/ReplayServerTest.cpp`, com gravações montadas e um relógio de teste: o grupo entregue na hora do seu tick, que depois espera o tick; um quadro sem tick; um quadro com vários ticks; um tick sem eventos; a diferença na quantidade de eventos e no ponto de controle, cada uma com o tick ou o ponto de controle e os dois valores; a mudança de época entre a tradução e o tick; o fim; e uma sessão gravada ao vivo, reproduzida em 3 ritmos com o mesmo resultado. Os testes de execução `muclient_replays_a_recording_to_its_end`, `muclient_reports_the_difference_of_a_replay`, `muclient_replays_a_session_that_never_reached_the_world` e, desde a revisão, `muclient_fails_a_replay_stopped_before_its_end` | passou |
| CA-06 | 5 testes com a etiqueta `[0002-CA-06]` em `Engine/Core/Tests/Fnv1aTest.cpp`, com os 3 valores de teste publicados do FNV-1a de 64 bits; 9 em `Games/MuOnline/Tests/StateDigestTest.cpp`: os 2 valores fixos (a visão vazia e uma cena com todas as partes preenchidas, calculados à parte a partir da lista do plano), a ordem dos objetos e do inventário que não muda a impressão, e cada campo do herói, dos objetos, do inventário e das contagens que muda | passou |
| CA-07 | a gravação `Games/MuOnline/Tests/Recordings/openmu-two-players.mucap`: 20 s no OpenMU 0.10.1 local com a `test0Dk`, e a `test1Dk` aparece em (135, 125) e sai da visão; 1.426 bytes, 23 mensagens, 6 ticks e 2 pontos de controle, sem conta e sem senha. O teste da biblioteca `[0002-CA-07]` em `ReplayServerTest.cpp` a reproduz até o fim em vários ritmos e confere que a conta e a senha não aparecem; o teste `muclient_replays_the_reference_recording` a reproduz no ritmo gravado. Os dois passam nos 3 sistemas, na CI | passou |
| CA-08 | 5 testes com a etiqueta `[0002-CA-08]` em `Games/MuOnline/Tests/ClientOptionsTest.cpp`: o `--record` com o `--connect`, o `--server-version` só com o `--record` e até 64 bytes, o `--replay` sem as opções do caminho até o mundo, sem `--record` e sem `--capture`, e o build de jogador recusando as três. Os testes de execução `muclient_refuses_a_file_that_is_not_a_recording` (recusa antes da janela abrir, com o motivo da leitura) e os 3 `muclient_player_build_refuses_*`, um por opção, no build de jogador. O código que liga as opções (`Games/MuOnline/App/Private/RecordingTools.cpp`) só compila com `NEXUM_AUTHORING` | passou |
| CA-09 | o alvo `muonline_recording` (`Games/MuOnline/Tests/RecordingFuzzTest.cpp`), que lê e, se a gravação vale, a reproduz inteira. As 23 sementes de `FuzzSeeds/Recording` (10 válidas, a gravação de referência entre elas, e 13 recusadas) rodam no teste `muonline_recording_seeds`, nos 3 sistemas. O passo "Fuzzing" da CI, no Linux com Clang, na execução citada acima: o alvo rodou 61 s, com 519.104 entradas, sem nenhuma falha. O teste `[0002-CA-09]` de `ReplayServerTest.cpp` cobre o defeito que o fuzzing achou (o relógio além de 2^63 µs) | passou |
| CA-10 | 2 testes com a etiqueta `[0002-CA-10]` em `Games/MuOnline/Tests/FrameAllocationTest.cpp` (programa `muonline_frame_allocation_tests`). Gravando, depois de 3 quadros de aquecimento, 1.000 quadros com 5 mensagens do mundo em cada um gravaram 5.000 mensagens, 658 ticks e 12 pontos de controle sem pedir memória. Reproduzindo, 1.000 quadros conferiram 658 ticks e 5 pontos de controle sem pedir memória. A sabotagem (um texto de 100 letras no ponto de controle do gravador e no do replay) fez os dois falharem, como registra o commit `dd793d3`. O limite do [TD-0019](../../docs/TECH_DEBT.md) continua: a memória que o SDL pede por `malloc` não é contada | passou nos 3 sistemas |
| CA-11 | 8 testes novos em `scripts/checks/tests`: 5 em `test_cpp_rules_check.py` (o `NOLINT` com a regra e o motivo passa; sem cada regra, com `*` ou sem o motivo na linha de cima, é recusado com a mensagem exata; o fim de um bloco não pede motivo; a regra vale em todas as pastas, `Tools/` incluída) e 3 em `test_pr_summary.py` (cada `NOLINT` novo listado com o arquivo e a linha, e os dos documentos e dos arquivos apagados de fora). A checagem `cpp-rules` roda em todo `verify`, e os três `NOLINT` que já existiam passam. Os testes das checagens são em Python, e a checagem `docs` só procura as etiquetas nos `.cpp`; por isso, no fechamento, a IA fez também uma verificação manual: a `cpp-rules`, rodada sobre uma pasta à parte com um arquivo de teste, recusou o `// NOLINT` sem a regra ("line 1: a NOLINT must name in parentheses each check it turns off, without '*'") e o `NOLINTNEXTLINE(misc-check)` com código na linha de cima ("line 4: a NOLINT needs a comment with the reason on the line above"), e aceitou o que tinha o motivo; no repositório, deu `OK: cpp-rules`. Os 8 testes, rodados pelo nome, passaram | passou |
| CA-12 | o teste de integração `openmu_muclient_records_and_replays` (etiqueta `openmu`), contra o OpenMU 0.10.1 local. Na T15, passou em 43 s. No fechamento, com o código final, passou de novo em 45 s: 20 s gravados com a `test0` (19 mensagens, 6 ticks e 1 ponto de controle), o replay com o servidor fora do caminho, "every tick and checkpoint matched" e as mesmas 8 linhas do mundo. A medição manual de 60 s, de 2026-11-27, está em "A medição de 60 segundos", abaixo | passou na verificação da IA; espera o teste do dono |
| CA-13 | verificação manual dos documentos na T16 e no fechamento, e as checagens `docs` e `structure` em todo `verify`: a [ARCHITECTURE](../../docs/ARCHITECTURE.md); a página nova [mu-recording](../../docs/modules/mu-recording.md), com o formato `.mucap` byte a byte, e a `mu-online`, a `mu-world`, a `mu-session`, a `mu-translate`, a `platform` e a `core`; o [ADR 0035](../../docs/adr/0035-adaptador-de-servidor-gravacao-e-replay.md); o [guia de desenvolvimento](../../docs/setup/DEV.md), com o jeito de gravar, de reproduzir e de refazer a gravação de referência; e o [TECH_DEBT](../../docs/TECH_DEBT.md), sem o TD-0037 e com o TD-0038, o TD-0039 e o TD-0040, cada um com destino | confirmado |

### A medição de 60 segundos (CA-12)

Em 2026-11-27, com o OpenMU 0.10.1 local, a IA gravou 60 s com a `test0` (`--world-seconds 60 --record ... --server-version "OpenMU 0.10.1"`). Uns 6 s depois, um segundo `MuClient` entrou com a `test1` por 20 s. A gravação teve 23 mensagens, 6 ticks e 2 pontos de controle, 3 pings e 4 objetos na visão no fim, sem nenhuma linha de erro ou de aviso. A `test1Dk` apareceu em (135, 125) e saiu da visão. Depois, o servidor foi desligado (a porta 44406 fechada), e o replay terminou com 0: "every tick and checkpoint matched", "the replay checked 23 messages, 6 ticks and 2 checkpoints, objects in view: 4", e as 11 linhas `world:` iguais às da sessão.

Uma sessão parada tem poucos pontos de controle: a marca do tick só é gravada quando o tick tira eventos da fila, e o ponto de controle vem no primeiro desses ticks depois de cada 5 s (ver "Desvios do plano").

### O `verify` do fechamento

Rodado em 2026-11-27, no Windows, sobre o código final e os documentos do fechamento, com `py scripts/dev.py verify`: tudo passou.

| Passo | Resultado |
|---|---|
| Formatação | OK |
| Build e testes (MSVC) | 55 de 55 |
| Build e testes com sanitizers | 56 de 56 |
| Build de jogador e testes | 44 de 44 |
| Análise estática | 72 arquivos, nenhum problema |
| Checagens do repositório | OK: `docs`, `structure`, `architecture`, `cpp-rules` e `terms` |
| Testes das checagens e dos hooks | 181 e 33 testes em Python; `guard-bash` 128 de 128, `guard-central` 65 de 65, `session-start` 27 de 27 |

O Linux (Clang e GCC) e o macOS só são conferidos pela CI do PR.

## Paridade

A fatia não mede um nível novo de paridade. O replay é a ferramenta do nível P1 (comportamento), e aqui ele serve de teste de regressão do **P0 (protocolo)**: ele confere que o Nexum chega ao mesmo estado com as mesmas mensagens de um OpenMU de verdade. Resultado: a gravação de referência passa em todos os ticks e pontos de controle nos 3 sistemas (CA-07), e a gravação de 60 s também (CA-12). Ainda não se confere que esse estado é igual ao do cliente original. O P0 do M2 inteiro só fica atingido no fim da parte 2, com o andar.

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0, e a prova visual dele (`visual_proof_0000_test_card`) continua passando nos 3 sistemas. A imagem aprovada não mudou.

## Sala limpa

A fatia não reproduz nenhum comportamento do original: o formato `.mucap`, a gravação, o replay e o adaptador de servidor são do próprio Nexum (a V.2.2, a IV.18, a III.8 e o item 5 da D-10 da spec de referência). Mesmo assim, as tarefas que mexem no módulo MU tocam em dados do protocolo: a T05 e a T06 (o adaptador e a época, sobre a sessão e a tradução da fatia 1), a T07 a T13 (o formato, a impressão, o gravador, o replay, o fuzzing, o `MuClient` e o quadro) e a T14 e a T15 (a gravação de referência e a integração). As outras são infraestrutura ou documentação: o ADR (T01), a trava dos `NOLINT` (T02), o FNV-1a (T03), o arquivo inteiro (T04) e os documentos (T16). A checklist foi conferida para a T05 a T15:

- [x] Escrito a partir de specs e achados da central, listados no `plan.md`, e não do código do MuMain. O [plano](plan.md) registra "Fontes da central usadas: nenhuma", porque nada vem do original. A tradução que o replay usa é a da fatia 1, com as fontes no [plano da fatia 1](../0001-rede-ate-o-mundo/plan.md#fontes-da-central-usadas).
- [x] Nenhum código do MuMain aberto durante a escrita. O hook `guard-central` bloqueia a pasta `Client/` da central, e nenhuma tarefa abriu a central.
- [x] Nenhum nome de variável, comentário ou estrutura copiado do MuMain. Os nomes novos (`ServerAdapter`, `LiveServer`, `ReplayServer`, `Recorder`, `RecordingWriter`, `StateDigestOf`) são do Nexum.
- [x] Constantes numéricas com a fonte citada numa spec. Nenhum número vem do original. Os 16 MiB do espaço da gravação, os 64 MiB do arquivo, os 5 s dos pontos de controle e os 64 bytes dos textos do cabeçalho são escolhas da spec da fatia (decisões 2 e 4); a versão `20404` do dialeto é a da fatia 1 (§5.1 da spec de protocolo); as constantes do FNV-1a são as do algoritmo publicado, com os valores de teste do documento do IETF.
- [x] Código ou XML do OpenMU com o aviso da licença MIT. Nenhum código do OpenMU foi usado. A gravação de referência é o que o OpenMU 0.10.1 enviou: ela tem um `README.md` na pasta `Games/MuOnline/Tests/Recordings`, com a origem e a licença MIT, e uma linha no [THIRD_PARTY](../../docs/THIRD_PARTY.md), que vale também para a cópia da semente.
- [x] Nenhum asset da Webzen no commit. A gravação de referência tem só as mensagens de rede do OpenMU: os números de itens e de NPCs e os nomes dos personagens de teste. Não há modelos, texturas, sons, tabelas de dados nem imagens do jogo.
- [x] Lacunas levadas à central antes de implementar: não houve. O plano registra "Perguntas abertas para a central: nenhuma", e nenhuma descrição faltou durante as tarefas. O bloco 3 (as pontas soltas da spec de referência, no fim deste relatório) não travou nenhuma tarefa.

## O que você deve testar

No Windows, com o Docker Desktop aberto e um terminal na pasta do Nexum, na branch `fatia/0002-gravacao-e-replay`:

1. Compile o build de desenvolvimento. No fim, aparece `OK  build`:

   ```powershell
   py scripts/dev.py build
   ```

2. Suba o OpenMU de teste. É a mesma imagem da fatia 1, já no seu computador, então nada é baixado. Se o servidor do laboratório da central (`nexum-lab-openmu`) estiver ligado, desligue-o antes, porque os dois usam as mesmas portas. O servidor fica pronto em poucos segundos:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml up -d
   ```

3. Grave 60 segundos no mundo. Quando o terminal mostrar `Password of the account test0:`, digite `test0` e tecle Enter. A senha não aparece enquanto você digita:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0 --world-seconds 60 --record build\minha-sessao.mucap --server-version "OpenMU 0.10.1"
   ```

4. Opcional: logo depois de o primeiro terminal mostrar `session: in the world`, abra outro terminal e entre por 20 segundos com a conta `test1` (senha `test1`):

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test1 --world-seconds 20
   ```

   Se os dois personagens nascerem perto, o primeiro terminal mostra `world: the player test1Dk (...) appeared at (...)` e, quando o segundo sair, `world: the player test1Dk (...) left the view`. O lugar onde a `test0Dk` nasce muda de uma entrada para outra: se ela nascer longe, o outro jogador não entra na visão. Isso é do servidor, e não um erro.
5. Quando a gravação acabar, confira as duas últimas linhas do primeiro terminal:
   - `client: the session ends: 60 s in the world, pings sent: 3, objects in view: ...`;
   - `client: wrote the recording to build\minha-sessao.mucap: ... messages, ... ticks and ... checkpoints`.
6. Desligue o servidor. Daqui em diante, nada conversa com o OpenMU:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml down
   ```

7. Reproduza a sua gravação. O terminal mostra as mesmas linhas `world:` da gravação, no mesmo ritmo. A primeira linha diz o que vai ser reproduzido (`replay: replaying ... messages, ... ticks and ... checkpoints, recorded by the Nexum 0.0.0 with the server OpenMU 0.10.1`). Perto do fim aparece `replay: the replay reached the end of the recording, where the client ended it: every tick and checkpoint matched`, e a última linha é `client: the session ends: 60 s in the world, the replay checked ...`, com os mesmos números do passo 5:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay build\minha-sessao.mucap
   ```

8. Confira que o programa terminou com sucesso. O número que aparece é 0:

   ```powershell
   $LASTEXITCODE
   ```

9. Reproduza a gravação de referência, a mesma que a CI usa. Ela dura 20 segundos: a `test1Dk` aparece em (135, 125) e sai da visão, e a última linha é `client: the session ends: 20 s in the world, the replay checked 23 messages, 6 ticks and 2 checkpoints, objects in view: 6`:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay Games\MuOnline\Tests\Recordings\openmu-two-players.mucap
   ```

10. Opcional: rode o mesmo comando do passo 9 e feche a janela antes do fim. O terminal mostra `client: the replay stopped before the end of the recording, so it checked only a part of it`, e o `$LASTEXITCODE` dá 1. Um replay pela metade não conta como sucesso (achado M2 da revisão).
11. Teste um arquivo que não é uma gravação. O terminal mostra `client: could not read the recording README.md: the recording does not start with the mark of a .mucap`, e o programa encerra com erro, sem abrir a janela. O `$LASTEXITCODE` dá 1:

    ```powershell
    .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay README.md
    ```

12. No [PR #12](https://github.com/ecologycrw/nexum/pull/12), confira que os 10 checks estão verdes. Antes de aprovar, leia o "Resumo de segurança do PR" (a página "Summary" da execução da CI): ele lista as 4 travas de `scripts/checks/` e o ROADMAP, nenhum teste removido e 42 linhas novas com `NOLINT`, todas no código e nos testes da própria checagem (ver "Travas alteradas e testes removidos").

Depois, conte o que viu: se deu tudo certo, ou o que ficou diferente. Em seguida, a `/retrospectiva` roda na mesma branch, antes da sua aprovação e do merge.

## Desvios do plano

- **O replay entrega cada grupo inteiro, e não registro a registro** (T10, commit `6947336`). O item 9 do plano e o CA-05 dizem que o replay entrega os registros cujo tempo já passou. Assim, um tick do cliente podia cair entre duas mensagens de um mesmo tick gravado e pegar só parte dos eventos, e a contagem daria diferença sem erro nenhum. O replay entrega o grupo inteiro de um tick quando chega a hora dele e espera esse tick antes do grupo seguinte. Isso mantém o agrupamento por tick da sessão gravada em qualquer ritmo de quadros, que é o que o CA-05 pede (revisão, achado M11).
- **O ponto de controle vem no primeiro tick com eventos depois de cada 5 s** (T09). O CA-04 diz "um ponto de controle a cada 5 segundos". O ponto de controle precisa vir logo depois de uma marca de tick (CA-03), e a marca só é gravada quando o tick tira eventos da fila. Numa sessão parada, quase nenhum tick tem eventos: 60 s parados deram 2 pontos de controle, e não 12 (revisão, achado M11).
- **No replay, só os quadros com tick recebem mensagens** (T13, commit `dd793d3`). O CA-10 fala em "mensagens em cada quadro". Gravando, é assim. Reproduzindo, o replay entrega no máximo um grupo por quadro, que espera o tick, então um quadro sem tick não traz mensagens. O teste reproduz uma gravação com um grupo de 5 mensagens em cada quadro gravado (revisão, achado M11).
- **Um replay interrompido termina com erro** (revisão, achado M2, commit `aab0a69`). A spec diz que, sem diferença, o replay encerra com sucesso. Um replay que o `--frames` ou a janela fechada param antes do fim conferiu só uma parte da gravação. Agora ele termina com 1 e a linha "the replay stopped before the end of the recording, so it checked only a part of it". O replay inteiro continua terminando com 0.
- **Os textos do cabeçalho também são conferidos como UTF-8** (revisão, achado M3, commit `06d2b77`). A lista de recusas do CA-03 não pedia isso. A leitura e o escritor recusam um texto que não seja UTF-8 válido (RFC 3629) ou que tenha caracteres de controle, com o motivo exato e o byte (INV-12).
- **O relógio do replay aceita qualquer hora** (T11). O fuzzing achou um defeito antes de rodar na CI: um relógio além de 2^63 µs (uns 292 mil anos) não cabia no arredondamento do `std::chrono`, e a gravação aceita tempos até 2^64 - 1 µs. O replay trata qualquer relógio a partir de 9 × 10^12 s como depois do fim de toda gravação, e um relógio antes do começo, ou que não é um número, como 0. O teste `[0002-CA-09]` de `ReplayServerTest.cpp` cobre os três casos.
- **O fim da sessão aplica os eventos que esperam um tick, mesmo sem gravação** (T12). Para as últimas mensagens terem o seu tick na gravação, o `FoundationApp::End` aplica os eventos que ainda esperam um tick antes de terminar. Isso vale também sem `--record`, então o "objects in view" da última linha do `MuClient` passou a contar as últimas mensagens.
- **A época mora em `Mu/Session/SessionEpoch.hpp`, com a ordem** (T06). O `EventsApplied` recebe quantos eventos o tick tirou da fila, aplicados ou descartados, e é chamado mesmo quando todos foram descartados, como pede o registro 4 do formato.
- **Peças movidas sem mudar o que fazem:** o `ToUtf8` e o `Stream` do PNG foram para `Engine/Platform/Private/SdlFile.hpp`, para a leitura do arquivo inteiro usar os mesmos (T04); o transporte de teste foi para `Games/MuOnline/Tests/ScriptedTransport.hpp`, usado pelos testes do gravador e do replay (T09). O `MuOnline` passou a ter o `Nexum::Platform` como dependência privada, para ler e escrever a gravação (T07).
- **O `.gitattributes` não mudou** (T14). Ele já marcava o `.mucap` como binário. A gravação de referência tem 1.426 bytes, bem abaixo dos 100 KB do risco do plano.
- **A gravação de referência precisou de duas tentativas** (T14). Na primeira, a `test0Dk` nasceu em (149, 130), longe da `test1Dk` em (135, 125), e a `test1` nem entrou na visão. Na segunda, ela nasceu em (135, 131), e deu certo. O `docs/setup/DEV.md` diz como conferir isso ao refazer a gravação.
- **Documentos além da lista do plano** (T16): a `mu-session` e a `mu-translate` também foram atualizadas (o `LiveServer` segura a sessão e a tradução), e o glossário ganhou "adaptador de servidor" e "ponto de controle". O `Tools/CaptureReplay` saiu da ARCHITECTURE, como diz o ADR 0035. No fechamento, a frase "Como é agora" da ARCHITECTURE passou a citar a fatia 2, e o [CONVENTIONS](../../docs/CONVENTIONS.md) ganhou dois exemplos de "Código de referência": o contrato com duas implementações e a leitura de um formato próprio.
- **Nenhum ADR além do 0035.** As outras decisões da fatia seguem a spec de referência (D-10, V.2.2, IV.18 e III.8) ou são detalhes da implementação, registrados aqui e no [tasks.md](tasks.md).

## Revisão

O subagente `revisor`, com contexto limpo, revisou em 2026-11-27 o diff `main...HEAD` até o commit `a322d81`, pela lista obrigatória inteira. Não achou nenhum problema crítico nem importante. Achou 11 menores. Cinco foram corrigidos, um virou item provisório, um virou proposta, três ficam explicados aqui, e um foi resolvido no fechamento, com o "sim" do dono. As correções vão até o commit `a97dc54`. O detalhe de cada achado está na seção "Revisão" do [tasks.md](tasks.md).

1. **M1: a época avisada ao gravador só no passo 2, e não antes do tick.** Procede, mas não acontece hoje: a época só muda no passo 2, e nenhum teste consegue mudá-la em outro passo. Virou o [TD-0040](../../docs/TECH_DEBT.md) (destino M4), e a consequência do ADR 0035 diz isso.
2. **M2: um replay interrompido terminava com sucesso.** Procede. Corrigido, com o teste `muclient_fails_a_replay_stopped_before_its_end` e a sabotagem (ver "Desvios do plano").
3. **M3: os textos do cabeçalho sem a conferência do UTF-8.** Procede (INV-12). Corrigido na leitura e no escritor, com 2 casos novos e a sabotagem.
4. **M4: o gravador perde em silêncio uma mensagem fora de 3 a 65.535 bytes.** Procede só em tese. A sessão só entrega mensagens que o leitor de mensagens já conferiu, com pelo menos o cabeçalho e o código e no máximo 65.535 bytes. Por isso, essa mensagem nunca chega ao gravador. Sem mudança.
5. **M5: nada obriga a impressão do estado a mudar com um campo novo da visão, e o script do valor fixo da cena cheia não está no repositório.** Procede, para o M4. Virou a [P-0022](../../docs/PROPOSALS.md).
6. **M6: o teste de memória do `ReadFileBytes` não conferia o motivo exato da recusa.** Procede: um arquivo que não existe passaria no teste antigo. Corrigido, com a sabotagem.
7. **M7: código repetido (o plural em três lugares, o `CountOf` e o tempo de um registro em dois).** Procede em parte. O plural tem duas formas de propósito: a do `LogLine`, que não pede memória e serve ao quadro, e a de `std::string`, fora do quadro, em dois lugares (`ReplayServer` e `RecordingTools`). O `CountOf` e o tempo do registro estão em dois lugares cada, abaixo da [Regra de três](../../docs/CONVENTIONS.md#regra-de-três). Sem mudança.
8. **M8: a página `mu-recording` dizia que a área não abre arquivo.** Procede. Corrigido.
9. **M9: a gravação de referência sem o aviso da licença do OpenMU.** Procede (decisão 5 da spec). Entrou um `README.md` na pasta `Tests/Recordings`, com a origem, a licença MIT e a cópia da semente, e uma linha no THIRD_PARTY.
10. **M10: o ROADMAP citava o TD-0037, que já saiu.** Procede. No fechamento, o dono disse "Sim, pode mudar", e o ROADMAP diz agora que o TD-0037 foi resolvido na fatia e que a fatia 2 espera o teste do dono.
11. **M11: três desvios de redação, com o comportamento certo e documentado** (o ponto de controle a cada 5 s, a entrega por grupo e as mensagens em cada quadro no replay). Procedem. Estão explicados em "Desvios do plano".

## Travas alteradas e testes removidos

**Travas alteradas.** Pela comparação do `git diff --name-only main...HEAD` com a lista de travas do `AGENTS.md`, mudaram 4 arquivos, todos da trava `scripts/checks/`:

| Trava | O que mudou | Aprovação do dono |
|---|---|---|
| `scripts/checks/nexum_checks/cpp_rules_check.py` | a checagem `cpp-rules` recusa um `NOLINT`, `NOLINTNEXTLINE` ou `NOLINTBEGIN` que não diga entre parênteses cada regra que desliga (uma lista vazia ou `*` também são recusadas), ou que não tenha um comentário com o motivo na linha de cima, em todas as pastas que ela lê, `Tools/` e os testes incluídos (T02, P-0015) | decisão 6 da spec, aprovada em 2026-11-26 |
| `scripts/checks/nexum_checks/pr_summary.py` | o resumo de segurança do PR lista cada linha nova com `NOLINT` em qualquer arquivo fora dos documentos, com o arquivo e a linha (T02, P-0015) | decisão 6 da spec, aprovada em 2026-11-26 |
| `scripts/checks/tests/test_cpp_rules_check.py` | 5 testes novos da regra dos `NOLINT`, cada recusa com a mensagem exata (T02) | decisão 6 da spec, aprovada em 2026-11-26 |
| `scripts/checks/tests/test_pr_summary.py` | 3 testes novos da lista dos `NOLINT` (T02) | decisão 6 da spec, aprovada em 2026-11-26 |

O resumo de segurança do PR lista também o `docs/ROADMAP.md`, que é um documento do dono, e não uma trava. Ele mudou duas vezes: no `/planejar`, para pôr a fatia 2 em "Agora", com a aprovação da spec (2026-11-26), e no fechamento, com o "Sim, pode mudar" do dono (2026-11-27). O resumo lista ainda 42 linhas novas com `NOLINT`: todas são do código e dos testes da própria checagem `cpp-rules` e do resumo, que citam a palavra. Nenhuma é uma exceção nova no C++. O único `NOLINT` do C++ que mudou foi o segundo do profiler (`Engine/Debug/Private/Profiler.cpp`), que ganhou a linha de motivo que a regra nova exige.

**Testes removidos:** nenhum. Nenhum teste foi apagado nem enfraquecido, e o resumo de segurança conta 99 testes adicionados e 0 removidos. Os testes da fatia 1 que mudaram só passaram a montar o `LiveServer` ou a receber a época: as conferências do estado da sessão passaram do cliente online para o servidor ao vivo (por exemplo, `client.State()` virou `server.State()`), a tradução recebe a época (`Translate(kEpoch, ...)`), e a fila entrega a época com cada evento, que alguns testes conferem. Nenhuma conferência saiu.

## Itens provisórios

**Criados** em [TECH_DEBT.md](../../docs/TECH_DEBT.md), cada um com o destino:

| Item | O que é | Destino |
|---|---|---|
| TD-0038 | o adaptador de servidor e a gravação `.mucap` não têm os comandos do jogador, que a V.2.2 da spec de referência pede na gravação; o replay confere só o que vem do servidor | M2, parte 2 |
| TD-0039 | a gravação do `MuClient` fica na memória, num espaço de 16 MiB, e o arquivo só é escrito quando o cliente termina; um travamento perde a gravação, e uma sessão longa e cheia de gente pode encher o espaço | M4 |
| TD-0040 | o `LiveServer` avisa o gravador da época só no começo do passo 2 do quadro, e não antes da marca do tick (revisão, achado M1) | M4 |

**Resolvidos**, saíram do TECH_DEBT na T16:

| Item | Como foi resolvido |
|---|---|
| TD-0037 | cada evento neutro leva a época da sua mensagem, como pede a D-10 (item 5). A fila (`Mu/World/EventQueue`) guarda cada evento com a época, a tradução a recebe da mensagem, e o tick descarta e conta os eventos de uma época mais velha que a do adaptador, registrando o descarte uma vez por tick (T06, CA-02) |

Nenhum item mudou de destino.

## Erros e lições

Ocorrências da escada do `docs/WORKFLOW.md`, para a `/retrospectiva` decidir o degrau:

1. **O MSVC deixou passar um campo que faltava num inicializador com nomes, e a CI quebrou no Linux** (T05, commit `b39f9dc`; corrigido no `8413196`). O estado "no mundo" do servidor ao vivo dava o estado sem o motivo. O Clang e o GCC recusaram (`-Wmissing-field-initializers`, um erro com `-Werror`) nos jobs `build-test (linux-clang)`, `build-test (linux-gcc)` e `release (linux)`; o `verify` no Windows tinha passado. É o mesmo tipo das conversões de sinal da fatia 1, que viraram a P-0019 (o PR em rascunho desde a lista de tarefas). A defesa funcionou: a CI do PR em rascunho pegou o erro no push seguinte, antes da revisão. A nota das armadilhas do `tasks.md` ganhou o caso.
2. **Um commit sem a linha `Co-Authored-By`** (T10, commit `6947336`). O `CLAUDE.md` pede a linha em toda mensagem de commit. Reescrever o histórico é proibido, então o commit fica assim. Desde então, a linha vai dentro do arquivo da mensagem do `git commit -F`, e o `git log -1` é conferido antes do push. Primeira ocorrência.
3. **Um teste de recusa sem o motivo exato, logo depois da regra da P-0020** (T04, revisão, achado M6, corrigido no `fb1d5ea`). O teste de memória do `ReadFileBytes` conferia só que a leitura falhou, e um arquivo que não existe passaria nele. É o mesmo erro que originou a P-0020 na fatia 1, agora com a regra já valendo.
4. **Dois defeitos que só a revisão viu:** o replay interrompido terminando com sucesso (M2) e os textos do cabeçalho sem a conferência do UTF-8 (M3, INV-12). Os dois foram corrigidos com a sabotagem. Primeira ocorrência de cada um.

Lições técnicas, sem erro da IA, que ficam para as próximas fatias:

- o fuzzing achou um defeito real antes de rodar na CI: um relógio além de 2^63 µs não cabia no `std::chrono` (T11);
- no Windows, o `abort` de um programa de sementes abre a janela "Debug Error!" do MSVC, e o teste fica parado até o limite de 300 s do CTest ([P-0021](../../docs/PROPOSALS.md));
- o lugar onde a `test0Dk` nasce no OpenMU muda de uma entrada para outra: ao gravar uma cena com dois jogadores, confira no log se o outro entrou na visão (T14);
- a marca do tick só é gravada quando o tick tira eventos da fila, então uma sessão parada tem poucos pontos de controle (T15);
- com o teste que grava e reproduz, os testes `openmu` levam uns 5 minutos (T15);
- a checagem `docs` só procura as etiquetas `[NNNN-CA-xx]` nos arquivos `.cpp`. Um critério provado por testes em Python, como o CA-11, precisa de uma verificação manual no report, e a checagem recusou o fechamento até ela entrar. Mudar isso mexe na trava `scripts/checks/`, então fica para a `/retrospectiva` decidir.

## Propostas geradas

- **P-0021:** a falha de uma semente de fuzzing no Windows abre uma janela em vez de falhar na hora. Vinda da sabotagem da T11; aberta.
- **P-0022:** a impressão do estado não pode esquecer um campo novo da visão do mundo, e o script do valor fixo da cena cheia vai para o repositório. Vinda da revisão (achado M5); aberta, para o M4.

A [P-0015](../../docs/PROPOSALS.md) (a trava dos `NOLINT`) e a [P-0018](../../docs/PROPOSALS.md) (o adaptador de servidor), aceitas na retrospectiva da fatia 1, foram aplicadas nesta fatia: a T02 e a T05, com o ADR 0035. A `/retrospectiva` atualiza o status delas.

O detalhe de cada uma está em [PROPOSALS.md](../../docs/PROPOSALS.md).

**Para levar à sessão da central** (o bloco 3; não são propostas do Nexum, porque a central é quem muda a spec de referência): a IV.12 ainda oferece um radar no canto "como opção do modo Enhanced"; o Apêndice G não lista o `light-mood.md`; e a data do front matter da spec de referência ficou em 2026-11-20, depois das correções de 2026-11-22.

# Relatório da fatia 1: A rede até o mundo

## Resumo para o dono

O `MuClient` conversa com um OpenMU de verdade, sem nenhuma modificação, rodando no seu computador pelo Docker. Ele pega a lista de servidores, entra no Game Server, faz o login, escolhe o personagem e fica no mundo, mandando o ping a cada 20 segundos. O terminal conta o que acontece: quem entra e quem sai da visão, quem anda e os valores do herói.

Por baixo, a fatia trouxe o gerador do código dos pacotes (`packetgen`, a partir dos XML do OpenMU), a cifra, o item e a aparência estendidos, a conexão de rede da engine (`Engine/Net`), a sessão, o ping, a tradução para eventos neutros e a visão do mundo. Cada leitor novo tem um alvo de fuzzing, e a janela minimizada deixou de gastar quase um núcleo inteiro do processador. Ainda não há imagem do mundo: a janela continua com o cartão de teste da fatia 0.

## Critérios de aceitação

As provas automáticas rodam em todo `py scripts/dev.py verify` e na CI. A última execução completa da CI antes do fechamento foi a [45907493559](https://github.com/ecologycrw/nexum/actions/runs/45907493559), do commit `123e3d7`, verde nos 10 checks, com o fuzzing. A do commit do fechamento, `6648407`, que só muda documentos, foi a [45913490721](https://github.com/ecologycrw/nexum/actions/runs/45913490721), também verde nos 10 checks, no [PR #11](https://github.com/ecologycrw/nexum/pull/11).

| CA | Prova | Resultado |
|---|---|---|
| CA-01 | 7 testes com a etiqueta `[0001-CA-01]` em `Games/MuOnline/Tests/FrameReaderTest.cpp`: duas mensagens num pedaço, uma mensagem partida, os quatro tipos em pedaços de todos os tamanhos até 1 byte, a maior `C2` (65.535 bytes) e os três erros (primeiro byte desconhecido, tamanho 0 e tamanho menor que o cabeçalho), cada um com o motivo | passou |
| CA-02 | 13 testes com a etiqueta `[0001-CA-02]` em `Games/MuOnline/Tests/CipherTest.cpp` (os vetores V1 a V9 da central, os 5 das capturas reais, o maior `C3` e o bloco quebrado) e 4 em `ClientCodecTest.cpp` (o Connect Server sem cifra, o `C1` sem contador, a `C3` byte a byte e o contador errado). A contagem está em "Vetores por grupo", abaixo | passou, 100% |
| CA-03 | os 613 testes gerados pelo `packetgen`, um por pacote dos 4 XML, com a etiqueta `[0001-CA-03]`, em 18 arquivos na pasta de build; 6 testes do gerador em `Tools/PacketGen/Tests/PacketGenTest.cpp`, com o XML de teste próprio que confere cada tipo de campo; 21 testes em `Games/MuOnline/Tests/DialectTest.cpp`, um por linha da tabela de variantes (§5.2 da spec de protocolo) | passou, 100% |
| CA-04 | 7 testes com a etiqueta `[0001-CA-04]` em `ExtendedItemTest.cpp` e 7 em `ExtendedAppearanceTest.cpp` (`Games/MuOnline/Tests`): as 32 combinações, as outras 224, os casos de borda, a captura real, as asas, a marca de GM e os dados cortados | passou, 100% |
| CA-05 | 13 testes com a etiqueta `[0001-CA-05]` em `Games/MuOnline/Tests/PacketExamplesTest.cpp`, com 169 conferências: os pacotes da sessão, o login igual ao da captura a não ser pelo número de série, os andares V1 a V6 e o eco do andar | passou, 100% |
| CA-06 | 9 testes com a etiqueta `[0001-CA-06]` em `Engine/Net/Tests/ConnectionTest.cpp`, contra o servidor de teste local (`LoopbackServer`): a ordem das mensagens nas duas filas, o servidor fora do ar, a conexão recusada, o fechamento por cada lado e a exceção na thread de rede; as checagens `architecture` (o Asio só no `Engine/Net`) e `terms` (o `Engine/Net` sem nada do MU), em todo `verify` | passou |
| CA-07 | 14 testes com a etiqueta `[0001-CA-07]` em `SessionTest.cpp`, 2 em `MessageQueueTest.cpp`, 2 em `NetworkTransportTest.cpp` e 3 em `OnlineClientTest.cpp` (`Games/MuOnline/Tests`): o caminho inteiro até "No mundo", as mensagens guardadas no carregamento, a recusa do login com o significado, os prazos de 10 s, os dados curtos do personagem e a época | passou |
| CA-08 | 3 testes com a etiqueta `[0001-CA-08]` em `PingTest.cpp`, 4 em `SessionTest.cpp` e 1 em `OnlineClientTest.cpp` (`Games/MuOnline/Tests`), com um relógio de teste: o ping a cada 20 s de relógio de parede desde a entrada no Game Server, as velocidades 0 e 0 antes do personagem e as do último estado depois, e a mesma fila de saída | passou |
| CA-09 | 16 testes com a etiqueta `[0001-CA-09]` em `TranslatorTest.cpp`, 11 em `WorldViewTest.cpp` e 1 em `OnlineClientTest.cpp` (`Games/MuOnline/Tests`): cada pacote da §8 vira um evento, aplicado no tick; a marca de "acabou de surgir"; o id e o código desconhecidos registrados (o código, uma vez só); os pacotes da §6.12 e o `C1 B2 1A` em silêncio. Os testes da visão usam só eventos | passou |
| CA-10 | os 7 testes das sementes (`*_seeds`), um por alvo: `nexum_core_parse_version`, `nexum_platform_load_png`, `muonline_frame_reader`, `muonline_cipher`, `muonline_extended_item`, `muonline_extended_appearance` e `muonline_dialect_packets`, nos 3 sistemas; 3 testes com a etiqueta `[0001-CA-10]` em `Engine/Platform/Tests/PngTest.cpp` (o PNG pequeno que declara 60.000 × 60.000 pixels é recusado pelo cabeçalho); o passo "Fuzzing" da CI, no Linux com Clang: na execução citada acima, cada um dos 7 alvos rodou 61 s, de 140 mil a 6,2 milhões de entradas, sem nenhuma falha | passou |
| CA-11 | teste com a etiqueta `[0001-CA-11]` em `Games/MuOnline/Tests/FrameAllocationTest.cpp` (programa `muonline_frame_allocation_tests`): depois de 3 quadros de aquecimento, 1.000 quadros com 5 mensagens do mundo em cada um não pedem memória na thread principal. Antes da correção da T16, eram 10.003 pedidos | passou nos 3 sistemas. O limite do [TD-0019](../../docs/TECH_DEBT.md) continua: a memória que o SDL pede por `malloc` não é contada |
| CA-12 | 2 testes com a etiqueta `[0001-CA-12]` em `ClientOptionsTest.cpp`, 2 em `PacketLogTest.cpp`, 1 em `SessionTest.cpp` e 1 em `DialectTest.cpp` (`Games/MuOnline/Tests`): sem `--password`, a senha vazia ou longa recusada sem rastro, o log com os 30 bytes do usuário e da senha escondidos, e a versão `20404` com o número de série `NEXUM-CLIENT-001` num lugar só; o teste do CTest `nexum_platform_hidden_line_tests` e os 4 testes do `MuClient` que dão a senha pela entrada padrão. O "sem aparecer" num terminal de verdade é a verificação manual do passo 3 do teste do dono | passou; o passo 3 foi confirmado pelo dono em 2026-11-26 |
| CA-13 | 3 testes com a etiqueta `[0001-CA-13]` em `Engine/Time/Tests/TickClockTest.cpp` (o tempo até o próximo tick e 10 minutos sem janela na tela, com um tick por quadro e nenhum perdido), 1 em `Engine/Platform/Tests/WaitTest.cpp` e 1 em `WindowTest.cpp`. Medição manual da IA, em 2026-11-26, no computador do dono: o build de desenvolvimento (Debug), no mundo do OpenMU 0.10.1 local, com a janela minimizada por 1.797 s. Foram 6 amostras de 60 s, uma a cada 5 minutos, pelo tempo de processador do processo: média de 0,91% de um núcleo, entre 0,65% e 1,25%. Isso é menos de 0,1% dos 12 núcleos lógicos, que o Gerenciador de Tarefas mostra como 0%. Os ticks ficaram em 25 por segundo (de 24,9 a 25,1 nos 360 relatórios de 5 s), sem nenhum descartado, e o ping saiu 90 vezes. Antes desta fatia, eram 92% | passou: 0,91% de média, contra o limite de 10%. O dono confirmou o passo 5 no teste de 2026-11-26 |
| CA-14 | os 2 testes de integração de `tests/integration/openmu` (etiqueta `openmu`), que passaram contra o OpenMU 0.10.1 local na T17 (detalhe em "Desvios do plano"). Verificação manual da IA, em 2026-11-26, no computador do dono, com o OpenMU 0.10.1 fixado pela impressão digital e iniciado pelo `compose.yaml`. O `MuClient` entrou com a conta `test0`, escolheu o primeiro personagem (`test0Dk`, no mapa 0 em (147, 126)) e ficou 1.800 s no mundo, com a janela minimizada, sem queda. O log mostrou 3 servidores na lista, o Game Server na porta 45902, o login aceito, 45 itens no inventário, 6 objetos na visão (o herói e 5 NPCs), 90 pings e o código de saída 0. Nenhuma linha de erro ou de aviso apareceu no log do cliente, e o log do servidor não ganhou nenhuma linha durante a partida. A senha errada, rodada de novo com o código do fechamento (`openmu_muclient_reports_a_wrong_password`), mostrou `session: the login was refused: invalid password (code 0)` e encerrou com erro, e o servidor registrou só a recusa da conta `test1`. A mensagem é em inglês, e não "senha inválida" (ver "Desvios do plano") | passou na verificação da IA e foi confirmado pelo dono: no teste de 2026-11-26, "Fiz todos os testes que você pediu e passaram" |
| CA-15 | verificação manual dos documentos na T18 e no fechamento, e as checagens `docs` e `structure` em todo `verify`: a [ARCHITECTURE](../../docs/ARCHITECTURE.md), as 7 páginas novas em [docs/modules](../../docs/modules/README.md) (`net`, `packetgen`, `mu-protocol`, `mu-session`, `mu-translate`, `mu-world` e `mu-online`), o [THIRD_PARTY](../../docs/THIRD_PARTY.md) com o Asio (Boost Software License 1.0) e os XML do OpenMU (MIT), o [ADR 0034](../../docs/adr/0034-packetgen-em-python-no-build.md) e o [TECH_DEBT](../../docs/TECH_DEBT.md), sem o TD-0015, o TD-0020 e o TD-0023, e com os itens novos TD-0026 a TD-0037, cada um com destino | confirmado |

### Vetores por grupo

O CA-05 pede a contagem dos vetores da central em cada grupo. Todos passam.

| Grupo | Vetores | Passam |
|---|---|---|
| A cifra (CA-02) | V1 a V9 do achado da cifra: o bloco de 8 bytes (V1), o ping com o contador 0 (V2), 3 `C1` do cliente (V3), o ping com os contadores 0, 1, 2, 3, 254, 255 e 0 de novo, e um `C1` e um `C2` no meio sem gastar contador (V4), 3 `C3` do servidor (V5), a mesma `C3` com os contadores 0, 1, 2, 255 e 0 de novo (V6), a `C4` do servidor (V7), o Xor3 do usuário (V8) e 2 contadores errados (V9). Das capturas reais: os pings do cliente com os contadores 1, 7 e 10, a saída do cliente com o 26 e a resposta do servidor com o 7 | 14 de 14 (9 da central e 5 das capturas), com 29 mensagens conferidas byte a byte |
| O codec e o dialeto (CA-03) | um teste por pacote dos XML do OpenMU (`ConnectServer`, `ClientToServer`, `ServerToClient` e `ChatServer`) e uma linha por variante da §5.2 | 613 de 613 pacotes e 21 de 21 linhas |
| O item e a aparência (CA-04) | o item: as 32 combinações das opções que somam bytes, as outras 224, os 2 exemplos do achado, 8 casos de borda, os valores dos soquetes, a luva de uma captura real e 3 itens cortados. A aparência: o personagem sem nada, a marca de GM e a pose, o brilho `00 01 48`, a luva da captura, as asas com o deslocamento de 8 bits, o Fenrir dourado e os bytes cortados | 256 de 256 combinações; todos os exemplos e casos de borda |
| Os pacotes de exemplo (CA-05) | o Hello e o pedido da lista, a lista de 3 servidores, o pedido e a resposta do endereço do Game Server, o `GameServerEntered`, o `LoginResponse`, o login da captura, o usuário ou a senha longos demais, a lista de personagens e a seleção, o ping da captura, o começo do `C2 12` do próprio jogador, os andares V1 a V6 e o eco do andar | 13 de 13 casos, com 169 conferências |

## Paridade

A fatia toca o nível **P0 (protocolo)**:

- **os vetores** medem a cifra, os formatos e as estruturas contra os vetores da central, que vêm dos testes do próprio OpenMU e de capturas reais. Resultado: 100% em cada grupo (CA-02 a CA-05, na tabela acima);
- **a sessão real** mede a conversa inteira contra um OpenMU 0.10.1 sem modificações. Resultado: os 2 testes de integração passaram, e a sessão de 30 minutos está no CA-14.

O P0 do M2 inteiro só fica atingido no fim da parte 2, com o andar. Os níveis P1 a P3 não se aplicam: nenhum comportamento visível do jogo mudou.

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0, e a prova visual dele (`visual_proof_0000_test_card`) continua passando nos 3 sistemas. A imagem aprovada não mudou.

## Sala limpa

As tarefas que implementam comportamento do MU são a T03 e a T04 (o codec e o dialeto), a T05 (o enquadramento), a T06 (a cifra), a T07 (o item e a aparência), a T08 (os pacotes de exemplo), a T11 e a T12 (a sessão e o ping), a T13 (a tradução e a visão do mundo), a T14 (a identidade do cliente no login), a T16 (as mensagens do mundo no teste do quadro) e a T17 (o servidor de teste). As outras são infraestrutura da engine ou documentação: o Asio (T01), a cópia dos XML (T02), a estrutura de fuzzing (T09), a conexão TCP (T10), a espera da janela minimizada (T15) e os documentos (T18). A checklist foi conferida para cada tarefa da primeira lista:

- [x] Escrito a partir de specs e achados da central, listados no `plan.md`, e não do código do MuMain. As fontes estão na seção "Fontes da central usadas" do [plano](plan.md): a spec de protocolo do cliente 106.3 (§2 a §11) e os achados da cifra, das variantes, dos códigos por idioma, da sessão, das capturas reais, do item, da aparência, das direções e da versão do Docker. Cada teste de vetor cita no começo o achado de onde vêm os bytes.
- [x] Nenhum código do MuMain aberto durante a escrita. O hook `guard-central` bloqueia a pasta `Client/` da central. O laboratório de captura (`C:\dev\nexum-lab`), que guarda também uma cópia do código do MuMain, nunca foi aberto. As duas sessões completas e o arquivo de 48 vetores do laboratório ficaram fora, porque têm o login da conta de teste e o número de série do MuMain (decisão 3 da spec).
- [x] Nenhum nome de variável, comentário ou estrutura copiado do MuMain. Os nomes dos pacotes e dos campos são os dos XML do OpenMU (por exemplo, `dialect::Login::Writer::SetUsername`), e os nomes das peças escritas à mão (`FrameReader`, `PacketEncryptor`, `Session`, `Translator` e `WorldView`) são do Nexum.
- [x] Constantes numéricas com a fonte citada numa spec. As chaves e os tamanhos da cifra vêm da §4.1 a §4.5 (o `Cipher.cpp` cita a seção de cada uma); a versão `20404`, da §5.1; as portas 44406 e 45901 a 45906, da §2 e da §10; os 20 s do ping, da §8 e do achado das capturas reais (§4.1); os 92 ou 93 bytes dos dados do personagem, da §6; os 5 a 15 bytes do item e os 27 da aparência, da §7.1 e da §7.2. Os números que não vêm do original são escolhas da spec da fatia, sem fonte no MU: os prazos de 10 s (CA-07), o número de série `NEXUM-CLIENT-001` (decisão 4) e os 60 s de fuzzing (CA-10).
- [x] Código ou XML do OpenMU com o aviso da licença MIT. Os XML e o esquema estão em `third_party/openmu`, com o `LICENSE` e a base `b53c704c6` no `README.md`, registrados no [THIRD_PARTY](../../docs/THIRD_PARTY.md). Cada cabeçalho gerado começa dizendo de qual XML do OpenMU veio, com o aviso do copyright e da licença MIT.
- [x] Nenhum asset da Webzen no commit. A fatia não tem modelos, texturas, sons, tabelas de dados nem imagens do jogo. Os vetores são bytes do protocolo, tirados dos documentos da central.
- [x] Lacunas levadas à central antes de implementar. O plano registra "Perguntas abertas para a central: nenhuma", e a central já tinha a spec de protocolo de 2026-11-22. Durante as tarefas, nenhuma descrição faltou. As escolhas que o original não faz, como quem guarda as velocidades do ping e o id lido sem o bit 15, são do Nexum e estão em "Desvios do plano". As pontas soltas da spec de referência (o bloco 3, no fim deste relatório) não travaram nenhuma tarefa.

## O que você deve testar

No Windows, com o Docker Desktop aberto e um terminal na pasta do Nexum, na branch `fatia/0001-rede-ate-o-mundo`:

1. Compile o build de desenvolvimento. No fim, aparece `OK  build`:

   ```powershell
   py scripts/dev.py build
   ```

2. Suba o OpenMU de teste. A imagem já está no seu computador, então nada é baixado. Se o servidor do laboratório da central (`nexum-lab-openmu`) estiver ligado, desligue-o antes, porque os dois usam as mesmas portas. O servidor fica pronto em poucos segundos:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml up -d
   ```

3. Entre no mundo. Quando o terminal mostrar `Password of the account test0:`, digite `test0` e tecle Enter. A senha não aparece enquanto você digita:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0
   ```

4. Confira no terminal, nesta ordem:
   - `session: the Connect Server lists 3 servers; choosing the server 0`;
   - `session: connecting to the Game Server at 127.0.0.1:45902` e `session: entered the Game Server, version 20404`;
   - `session: the login was accepted` e `session: choosing the character test0Dk`;
   - `session: in the world`, e logo depois `world: the hero 0x0200 is on the map 0 at (...)`, com a vida e a mana;
   - `world: the inventory came with 45 items`;
   - uma linha `came into view at` para o próprio herói e uma para cada NPC perto dele (na medição da IA, eram 5).

   A cada 5 segundos aparece também uma linha `time:` com cerca de `25.0 ticks per second` e `0.00 ticks dropped`.
5. Minimize a janela e deixe o programa rodando por 30 minutos. O terminal mostra `platform: the window was minimized`. No Gerenciador de Tarefas, a coluna CPU do `MuClient` fica em 0%. Na medição da IA, ele gastou 0,91% de um núcleo, menos de 0,1% do computador inteiro. Antes desta fatia, minimizado, ficava perto de 8% (92% de um núcleo, dividido pelos 12 núcleos lógicos).
6. Depois dos 30 minutos, restaure a janela: o cartão de teste volta a andar. Feche a janela. A última linha do terminal resume a sessão: `client: the session ends: ... s in the world, pings sent: ..., objects in view: ...`. Com 30 minutos, são cerca de 90 pings (um a cada 20 segundos).
7. Opcional: com o primeiro ainda no mundo, abra outro terminal e entre com a conta `test1` (senha `test1`), trocando `test0` por `test1` no comando do passo 3. Se os dois personagens estiverem perto, cada terminal mostra o outro jogador entrando na visão (`world: the player ... came into view at ...`).
8. Teste uma senha errada: rode o comando do passo 3 e digite qualquer outra senha. O terminal mostra `session: the login was refused: invalid password (code 0)` e `client: the session ends: never in the world`, e o programa encerra com erro. A spec fala em "senha inválida": a mensagem é a mesma, em inglês, como todo o log do Nexum.
9. Veja o log do servidor e cole aqui o que aparecer, se quiser que eu confira com você. Na medição da IA, a única linha nova foi a recusa da senha errada (`Account not found or invalid password`). Um erro do Castle Siege logo no início é conhecido: vem do próprio OpenMU em modo de demonstração e não tem a ver com o cliente. Nas duas partidas da IA, ele não apareceu:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml logs
   ```

10. Desligue o servidor:

    ```powershell
    docker compose -f tests/integration/openmu/compose.yaml down
    ```

11. No [PR #11](https://github.com/ecologycrw/nexum/pull/11), confira que os 10 checks estão verdes.

Depois, conte o que viu: se deu tudo certo, ou o que ficou diferente. Em seguida, a `/retrospectiva` roda na mesma branch, antes da sua aprovação e do merge.

## Desvios do plano

- **A checagem `structure` não mudou** (T02). O plano previa que ela passasse a aceitar a pasta `third_party/openmu` e o registro, no THIRD_PARTY, de conteúdo que não vem do vcpkg. Ela já aceitava a pasta, e o registro ficou numa lista própria do THIRD_PARTY ("Conteúdo de terceiros fora do vcpkg"), fora da tabela que a checagem confere, como já acontecia com o Tracy. Uma trava a menos mudou.
- **A checagem `architecture` também não mudou** (T01). Ela já dava o Asio ao `Engine/Net` desde a preparação.
- **O código gerado só tem cabeçalhos** (T03). O plano falava em "código gerado"; ele saiu como cabeçalhos com funções pequenas, sem arquivos `.cpp`, mais um arquivo de testes por XML. As peças comuns de leitura e escrita ficam escritas à mão em `Mu/Protocol/PacketFields.hpp`.
- **Seis campos de um arquivo usam um enum do outro** (T03): por exemplo, o `ClientToServerPackets.xml` usa o `ServerToClient.GuildMemberRole`. Os dois arquivos se citam um ao outro, então esses campos ficam com o byte cru (`std::uint8_t`), sem o tipo do enum. Nenhum deles é usado nesta fatia.
- **O fuzzing achou, antes de rodar, dois leitores sem guarda para uma mensagem vazia** (T09). Ao escrever os alvos, ficou claro que a cifra (`Encrypt`, `Decrypt` e o Xor32) lia o primeiro byte de uma mensagem vazia, e que o `Encrypt` aceitava uma mensagem sem código depois do cabeçalho. Os dois passaram a recusar esses casos com um erro. Na sessão, as mensagens vêm do enquadramento, que nunca entrega uma mensagem assim, mas o leitor não depende disso (INV-12).
- **O `LoadPng` ganhou dois irmãos públicos** (T09): o `ReadPngSize`, que lê o tamanho no cabeçalho, e o `DecodePng`, que lê um PNG da memória. O alvo de fuzzing do PNG usa os dois, e o `LoadPng` segue o mesmo caminho.
- **"No mundo" pede também o mapa pronto** (T11). O diagrama do plano passa de "Carregando" a "No mundo" com os dados do personagem (`C3 F3 03`). A sessão espera também o aviso de "mapa pronto" do app (`FinishLoading`), como a IV.14.1 e a fase 5 da IV.16 da spec de referência. Assim, o que chega entre os dados do personagem e o mapa pronto fica guardado e é entregue em ordem, como pede o CA-07. Nesta fatia não há mapa para carregar, e o app dá o aviso logo depois dos dados (T14). No primeiro ingresso, o `C1 F3 12` não é enviado (spec de protocolo, §6.12).
- **O tradutor de bytes do cliente (`ClientCodec`) entrou na T11**, e não na T10, porque é a sessão que escolhe a cifra de cada conexão: nenhuma no Connect Server e a do Game Server no outro.
- **A sessão não dá eventos neutros ainda** (T11). Ela entrega as mensagens do mundo em ordem, cada uma com a época, e a tradução para eventos neutros é a T13, como o plano divide.
- **As velocidades do ping vêm da sessão** (T12). O plano diz que o ping leva as velocidades "do último estado conhecido", sem dizer quem guarda esse estado. A visão do mundo só chega na T13, e o ping corre também durante o carregamento. Por isso, a sessão lê as velocidades dos dados do personagem (`C3 F3 03`) e de cada `C1 26 FF` assim que chegam. Antes do personagem, elas são 0 e 0, como nas capturas reais.
- **Os dados do personagem também viram um evento** (T13). A lista da CA-09 tem os pacotes do mundo da §8; o `C3 F3 03`, que a sessão entrega primeiro, vira o evento "o herói está no mapa" (`HeroLoaded`), com o mapa, a posição e os valores, porque a visão do mundo precisa deles para saber onde o herói está.
- **Os eventos esperam numa fila entre o quadro e o tick** (T13): o quadro tira as mensagens e as traduz, e o tick seguinte aplica os eventos, em ordem, como pede a ARCHITECTURE ("O quadro"). A fila (`Mu/World/EventQueue`) reserva a memória ao nascer, como a fila das mensagens da sessão.
- **Os eventos ficam no `Mu/World`** (T13), e não no `Mu/Translate`: a visão do mundo e os sistemas futuros dependem só dos eventos, e a tradução depende dos eventos e do protocolo. A árvore da spec de referência já põe "eventos" na pasta `World/`.
- **A leitura dos dados do personagem saiu da sessão** (T13) para `Mu/Protocol/CharacterInformation.hpp`, porque a tradução também lê o pacote de 92 ou 93 bytes. A sessão faz o mesmo de antes, com as mesmas mensagens de erro.
- **Uma linha de log sem pedir memória no `Engine/Debug`** (T13, `Nexum/Debug/LogLine.hpp`). A visão do mundo conta cada mudança no log dentro do tick, e o quadro não pode pedir memória (INV-13, CA-11). A linha é montada num espaço fixo, como a do relatório do laço (`TickReportText`), e não sabe nada do MU.
- **Todo id é lido sem o bit 15** (T13). A marca de "acabou de surgir" só existe no `C2 12` e no `C2 13`; os outros pacotes nunca a põem, então tirar o bit de todos é uma escolha do Nexum que deixa a busca na tabela sempre com o mesmo id.
- **O alvo de fuzzing dos pacotes do dialeto também traduz** (T13): a mesma mensagem passa pela tradução e pela visão do mundo, com um log que não guarda nada. Não há alvo novo.
- **Um cabeçalho por pacote e o dialeto em quatro partes** (entre a T13 e a T14, [P-0016](../../docs/PROPOSALS.md)). O plano previa, no risco "O tamanho do código gerado", medir o tempo de build e dividir o código gerado se pesasse. Pesou: o cabeçalho único do servidor tinha 1,2 MB, cada compilação que lia o dialeto passava de 1 GB de memória, e o build enchia os 16 GB do computador do dono. O `packetgen` passou a gerar um cabeçalho por pacote (`Mu/Protocol/Packets/<Arquivo>/<Pacote>.hpp`), mais os tipos comuns de cada arquivo (`Types.hpp`), e os testes gerados em arquivos de até 40 pacotes. O dialeto ficou em `Mu/Protocol/Dialect/` (`ConnectServer`, `Login`, `FirstEntry` e `World`, pelas seções da spec de protocolo), e o `Dialect.hpp` guarda só a identidade do cliente. Nenhum cabeçalho inclui todos os pacotes; cada arquivo do código inclui só as partes do dialeto que usa. Os 613 testes gerados continuam lá, um por pacote, agora em 18 arquivos em vez de 4. Medido no computador do dono, com 16 GB e 12 núcleos lógicos:

  | Medida | Antes | Depois |
  |---|---|---|
  | Build com sanitizers, no `verify`, recompilando tudo o que usa pacotes | 292 s (38 passos) | 106 s (49 passos) |
  | Build de jogador, no `verify`, na mesma situação | 231 s (35 passos) | 111 s (46 passos) |
  | Maior pico de memória de um compilador | 1,39 GB | 0,25 GB |
  | Compiladores acima de 500 MB | 13 | 0 |
  | Maior soma de memória dos compiladores ao mesmo tempo | 8,6 GB (13 compiladores) | 2,3 GB (14 compiladores) |

  Os tempos incluem os testes de cada build. O "antes" da memória foi medido no build de desenvolvimento, recompilando o que lê o dialeto (227 s); o "depois", no `verify` inteiro, que recompilou tudo o que usa pacotes nos builds com sanitizers e de jogador. Os compiladores de cada linha são os mesmos (MSVC).
- **Um teste de pacote sem pedir memória** (T11): o `IsPacket` (`Mu/Protocol/PacketFields.hpp`) diz qual pacote chegou sem montar texto. O `CheckPacket`, que o código gerado usa em toda leitura, montava o nome do pacote num `std::string` mesmo quando tudo dava certo; isso ficou para a T16, que mede o quadro com mensagens de rede.
- **O nome do pacote só vira texto quando há erro** (T16). O teste do CA-11 achou 10.003 pedidos de memória em 1.000 quadros com 5 mensagens do mundo em cada um: cerca de 10 por quadro, nas leituras dos pacotes, e 3 nos pings. O `CheckPacket` e o `StartPacket` (`Mu/Protocol/PacketFields.cpp`) montavam o nome do pacote num `std::string` antes de conferir qualquer coisa; agora o texto só é montado para descrever um erro. As mensagens de erro são as mesmas, e o código gerado não mudou.
- **O transporte de teste do quadro** (T16, `FrameTransport` em `FrameAllocationTest.cpp`): os dois primeiros quadros levam o cliente ao mundo, uma conexão por quadro, e depois cada quadro entrega as mesmas 5 mensagens do mundo, já prontas na memória, como uma conexão de verdade entrega a fila dela. As 5 são as dos testes da tradução: o NPC que entra e sai da visão, o andar do herói, a correção da posição e os valores do herói. Elas ficam em `SessionMessages.hpp`, com as fontes. O que a thread de rede de uma conexão real pede não é contado: o contador só vê a thread do quadro, que é onde as mensagens são lidas, traduzidas e aplicadas.
- **O cliente online ganhou uma área própria, o `Mu/Online`** (T14). A tabela de módulos do plano lista o `Mu/Protocol`, o `Mu/Session`, o `Mu/Translate` e o `Mu/World`. O que liga essas peças ao quadro ficou no `Mu/Online`: o `NetworkTransport`, que abre uma `Connection` com um `ClientCodec` novo para cada conexão que a sessão pede, e o `OnlineClient`, que roda a sessão, a tradução e a visão do mundo nos passos 2 e 3 do quadro e no começo de cada tick. Ele está na biblioteca, e não no programa, para os testes o alimentarem sem rede (T14) e o teste do quadro sem alocação (T16) usar um transporte de teste pela mesma interface (`Transport`).
- **A senha também pode vir da entrada padrão** (T14). O CA-12 pede a senha digitada no terminal sem aparecer. Quando a entrada não é um terminal (um arquivo ou um pipe), o `ReadHiddenLine` lê a linha como ela vem, porque nada a mostra. É assim que os testes do `MuClient` nos 3 sistemas e o teste de integração da T17 dão a senha, sem ela passar pela linha de comando. O "sem aparecer" num terminal de verdade só se confere à mão, no passo 3 do teste do dono.
- **A janela fora da tela também não recebe o quadro** (T15). O plano pede só a espera: sem nenhuma janela visível, o laço espera até o próximo tick, nunca mais que 40 ms. Com a espera, o `MuClient` minimizado passou a desenhar 25 quadros por segundo, com 25 ticks por segundo, mas ainda gastava de 7,7% a 8,6% de um núcleo no build de jogador. Cada quadro continuava pedindo ao Vulkan a imagem da janela minimizada, e esse pedido custava mais que o quadro inteiro. Agora, enquanto a janela não aparece (minimizada, escondida ou coberta, pelo `Window::IsVisible`), o quadro roda os ticks e monta o desenho, mas não vai para a placa. A espera usa o `SDL_DelayNS`, que no Windows tem um temporizador preciso: 20 ms pedidos viram cerca de 20,3 ms, e não os passos de 15,6 ms da espera da biblioteca padrão. Medido no computador do dono, sem conexão, em janelas de 15 s:

  | Build de jogador, janela minimizada | Processador (um núcleo) |
  |---|---|
  | Antes desta fatia (medido na T13 da fatia 0, [TD-0015](../../docs/TECH_DEBT.md)) | 92% |
  | Só com a espera | 7,7% e 8,6% |
  | Com a espera e sem enviar o quadro | 0,7%, 0,4% e 0,3% |

  Ao restaurar, a janela volta a 60 quadros por segundo. A medição no mundo está no CA-13.
- **O servidor de teste dispensa o repetidor** (T17). O plano previa conferir se a imagem 0.10.1 aceita um IP fixo no `RESOLVE_IP`. Ela aceita: com `RESOLVE_IP=127.0.0.1`, os Game Servers se registram em `127.0.0.1:45901` a `45906`, que são as portas publicadas pelo `compose.yaml`. O `MuClient` segue o endereço do Connect Server sem nenhum repetidor, e as portas não precisaram ir para 127.127.127.127. A imagem, com a impressão digital da spec, já estava no computador do dono (a do laboratório), e nada foi baixado.
- **O teste de integração só roda com o endereço configurado** (T17). Os dois testes de `tests/integration/openmu` levam a etiqueta `openmu` e leem o endereço do Connect Server na variável `NEXUM_OPENMU_ADDRESS`. Sem ela, como na CI, o CTest os marca como pulados ([TD-0034](../../docs/TECH_DEBT.md)). Eles usam o mesmo verificador dos testes do `MuClient` (`CheckClientRun.cmake`). A senha errada usa a conta `test1`, para não depender da `test0`, que pode estar saindo do servidor. Primeira rodada, em 2026-11-26, contra o OpenMU 0.10.1 local:

  | Teste | Resultado |
  |---|---|
  | `openmu_muclient_stays_three_minutes_in_the_world` | passou em 181,7 s: 3 servidores na lista, o Game Server na porta 45902 (a que o Connect Server indicou), o login aceito, o personagem `test0Dk` no mapa 0 em (138, 128), 6 objetos na visão, 9 pings e nenhuma linha de erro no log do cliente |
  | `openmu_muclient_reports_a_wrong_password` | passou em 1,6 s: "the login was refused: invalid password (code 0)", e a sessão encerra sem entrar no mundo |

  O log do servidor não teve nenhum erro e nenhum aviso. Depois da partida, as únicas linhas são as duas recusas de senha da `test1` ("Account not found or invalid password"), uma da conferência à mão e outra do teste. O erro do Castle Siege que o passo 9 do teste do dono cita não apareceu nesta partida: só a linha de informação "Castle Siege initialized".
- **A mensagem da senha errada é em inglês.** A spec diz que o terminal mostra "senha inválida"; o cliente escreve `session: the login was refused: invalid password (code 0)`, porque todo o log do Nexum é em inglês. O significado é o mesmo, e o passo 8 do teste do dono traz a linha exata. A spec não mudou: mudá-la pede o "sim" do dono (revisão, achado 11).
- **Nenhum ADR além do 0034.** As outras decisões da fatia seguem a spec de referência (D-10 e IV.14) ou são detalhes da implementação, registrados aqui e no [tasks.md](tasks.md). O desvio provisório da D-10 (a época fora dos eventos neutros) é um item provisório, o [TD-0037](../../docs/TECH_DEBT.md), com destino na fatia 2.

## Revisão

O subagente `revisor`, com contexto limpo, revisou em 2026-11-26 o diff `main...HEAD` até o commit `2184715`, pela lista obrigatória inteira. Não achou nenhum problema crítico. Achou 1 importante e 12 menores. As correções vão até o commit `18b8420`. O detalhe de cada achado está na seção "Revisão" do [tasks.md](tasks.md).

1. **Importante: o teste do checksum errado não chegava ao checksum (CA-02).** Procede: o teste troca o mesmo bit no tamanho e no checksum, e ganhou o caso de um byte de dados trocado. Com a conferência desligada de propósito, ele falha.
2. **Menor: o `Connection::Close` descarta o que ainda está na fila de saída.** Procede, sem efeito hoje: o contrato diz isso agora.
3. **Menor: o `Engine/Net` não tinha a fronteira das exceções do Asio e do sistema.** Procede: a thread de rede fecha a conexão com o motivo, com um teste novo.
4. **Menor: as cópias da senha são apagadas de forma desigual.** Procede: virou o TD-0036 (M7).
5. **Menor: a conta e a senha são apagadas no primeiro login, e a reconexão vai precisar delas.** Procede: entrou no TD-0028.
6. **Menor: a época não chega aos eventos neutros (D-10, item 5).** Procede: virou o TD-0037 (fatia 2).
7. **Menor: um Ctrl+C na pergunta da senha não religa o eco do terminal.** Procede: entrou no TD-0032.
8. **Menor: um `NOLINT` sem motivo em todo cabeçalho gerado.** Procede: as duas linhas saíram, e a análise estática não acha nada.
9. **Menor: o hexadecimal repetido e a regra do cabeçalho refeita no `Translator`.** Procede: cada um ficou num lugar só.
10. **Menor: o teste dos pacotes ignorados usava os mesmos apelidos que a lista do `Translator`.** Procede: o teste escreve os bytes da §6.12.
11. **Menor: a spec diz "senha inválida", e o cliente escreve em inglês.** Procede: o passo 8 do teste do dono traz a linha exata (ver "Desvios do plano").
12. **Menor: a frase "Agora" do STATUS estava desatualizada.** Procede: o STATUS foi atualizado neste fechamento.
13. **Menor: o report não tinha a contagem dos vetores, as medições do CA-13 e do CA-14, as provas e a sala limpa.** Procede: estão neste report.

## Travas alteradas e testes removidos

**Travas alteradas.** Pela comparação do `git diff --name-only main...HEAD` com a lista de travas do `AGENTS.md`, mudaram 6 arquivos:

| Trava | O que mudou | Aprovação do dono |
|---|---|---|
| `vcpkg.json` | o Asio 1.38.2, para o transporte TCP (T01) | 2026-11-25, "Sim, todas", com o download pelo vcpkg |
| `cmake/NexumPacketGen.cmake` (novo) | a função `nexum_generate_packets`, que roda o `packetgen` no build, e a busca do Python 3.10, com uma mensagem clara se ele faltar (T03). Depois, entre a T13 e a T14 ([P-0016](../../docs/PROPOSALS.md)), a função passou a pedir a lista dos arquivos ao `packetgen` quando o CMake configura (`--list`), a declarar todos como saídas e a configurar de novo quando um XML ou o script muda; os parâmetros passaram a ser `INCLUDE_DIR` e `TESTS_PREFIX` | a primeira versão em 2026-11-25, "Sim, todas"; a segunda em 2026-11-26, "Escolha a opção 1", com a confirmação da edição da trava |
| `cmake/NexumFuzzing.cmake` (novo) | a opção `NEXUM_FUZZING` e a função `nexum_add_fuzz_target`: sem a opção, cada alvo vira um teste das sementes nos 3 sistemas; com ela, só no Clang, vira também um programa do libFuzzer (T09) | 2026-11-25, "Sim, todas" |
| `CMakePresets.json` | o preset `linux-fuzz`, de configuração, de build e de teste (T09) | 2026-11-25, "Sim, todas" |
| `.github/workflows/ci.yml` | o passo "Fuzzing" no job do Linux com Clang, que compila o preset `linux-fuzz` e roda cada alvo por 60 s a partir das sementes (T09) | 2026-11-25, "Sim, todas" |
| `scripts/dev.py` | o `tidy` compila o build de desenvolvimento antes de analisar, porque o `clang-tidy` lê os cabeçalhos gerados dos pacotes, e o job da análise estática na CI só configurava (entre a T10 e a T11, pela primeira CI do PR #11) | 2026-11-26, "Sim, pode mudar" |

As checagens `structure` e `architecture` (`scripts/checks/`), que o plano previa mudar, não mudaram (ver "Desvios do plano").

**Testes removidos:** nenhum. Nenhum teste foi apagado nem enfraquecido. Os testes que já existiam e mudaram só ganharam casos: o `FrameAllocationTest.cpp` passou a ter mensagens de rede em cada quadro (CA-11); o `PngTest.cpp`, o `WindowTest.cpp`, o `TickClockTest.cpp` e o `ClientOptionsTest.cpp` ganharam os casos desta fatia. Uma função auxiliar do `ClientOptionsTest.cpp` (`ErrorOf`) mudou de forma, sem tirar nenhuma conferência.

## Itens provisórios

**Criados** em [TECH_DEBT.md](../../docs/TECH_DEBT.md), cada um com o destino:

| Item | O que é | Destino |
|---|---|---|
| TD-0026 | o fuzzing roda 60 s por alvo na CI, e não 1 hora | M2 |
| TD-0027 | os prazos das respostas da sessão são fixos, de 10 s | M7 |
| TD-0028 | uma queda ou uma recusa encerra a sessão, sem reconexão, e a conta e a senha são apagadas no primeiro login | M7 |
| TD-0029 | a visão do mundo é uma tabela por id, sem entidades | M4 |
| TD-0030 | os personagens transformados e as invocações não são traduzidos | M4 |
| TD-0031 | os itens no chão são ignorados, e os efeitos dos jogadores vêm só contados | M5 |
| TD-0032 | a conta, o servidor e o personagem vêm da linha de comando, e a senha do terminal; um Ctrl+C na pergunta não religa o eco | M6 |
| TD-0033 | o mapa é dado por pronto assim que os dados do personagem chegam | M3 |
| TD-0034 | o teste de integração contra o OpenMU roda só no computador do dono | M7 |
| TD-0035 | os vetores vêm dos documentos da central, sem o programa em C# da V.2.1 | M2 |
| TD-0036 | as cópias do login continuam na memória até serem sobrescritas | M7 |
| TD-0037 | os eventos neutros não levam a época, como pede a D-10 | fatia 2 |

**Resolvidos**, saíram do TECH_DEBT na T18:

| Item | Como foi resolvido |
|---|---|
| TD-0015 | fora da tela (minimizada, escondida ou coberta), o laço do `MuClient` não envia o quadro para a placa e espera até o próximo tick (T15). Minimizado, no build de jogador e sem conexão, o processador caiu de 92% para cerca de 0,3% a 0,7% de um núcleo; a medição no mundo está no CA-13 |
| TD-0020 | o `LoadPng` confere o tamanho no cabeçalho do PNG antes de ler os pixels (T09). O teste `[0001-CA-10]` do `PngTest.cpp` recusa um arquivo de 33 bytes que declara 60.000 × 60.000 pixels |
| TD-0023 | a estrutura de fuzzing existe (T09, `cmake/NexumFuzzing.cmake`): cada leitor tem um alvo, inclusive o `ParseVersion` e o `LoadPng`, da fatia 0. As sementes rodam como testes comuns nos 3 sistemas, e a CI roda cada alvo por 60 s no Linux com Clang |

Nenhum item mudou de destino.

## Erros e lições

Ocorrências da escada do `docs/WORKFLOW.md`, para a `/retrospectiva` decidir o degrau:

1. **Conversões de sinal que o MSVC não acusa quebraram a CI duas vezes** (T09, commit `18c4269`, e T14, commit `22b35ab`). Na primeira, o teste de fuzzing da cifra somava um tamanho sem sinal a um iterador, e o Clang e o GCC recusaram no Linux e no macOS. Na segunda, o GCC recusou a soma de um `std::size_t` com um `int`, que o Clang também tinha deixado passar. É a **segunda ocorrência** na mesma fatia: pela escada, ela vira uma proposta de regra. Hoje, a única defesa é a nota do `tasks.md` (rodar o `clang-tidy` do Visual Studio com `-Wsign-conversion` antes de enviar).
2. **Um teste que passava sem provar o que dizia** (T06, achado importante da revisão, corrigido no `8242de5`). O teste do checksum errado falhava antes de chegar ao checksum. A fatia 0 teve um achado do mesmo tipo (o teste do CA-09, que não via a memória do SDL). Se a retrospectiva contar os dois como o mesmo erro, é a segunda ocorrência. A correção desta fatia já usou o jeito que pega o problema: desligar a conferência de propósito e ver o teste falhar.
3. **Um `NOLINT` sem motivo em todo cabeçalho gerado** (T03, achado 8 da revisão, corrigido no `caaa674`). A [P-0015](../../docs/PROPOSALS.md), aberta, trata justamente de conferir os `NOLINT` novos. Primeira ocorrência.

Lições técnicas, sem erro da IA, que ficam para as próximas fatias:

- o código gerado pode encher a memória do build: medir o pico de cada compilador logo que o gerador nasce, e não só o tempo (P-0016);
- a análise estática precisa do build feito quando há cabeçalhos gerados (o `tidy` do `scripts/dev.py` compila antes, desde a T10);
- no Windows, o `file(WRITE)` do CMake grava cada `\n` como `\r\n`; para finais de linha exatos, use `file(CONFIGURE ... NEWLINE_STYLE)`;
- não rode um build enquanto o `verify` roda em segundo plano na mesma pasta: os dois colidem ("failed recompaction");
- a janela minimizada também precisa parar de pedir a imagem ao Vulkan, e não só esperar: só a espera ainda gastava 8% de um núcleo (T15).

## Propostas geradas

- **P-0016:** o código gerado dos pacotes sem estourar a memória do build. Aceita e aplicada na própria fatia, entre a T13 e a T14.
- **P-0017:** as mensagens de erro da rede em UTF-8 no Windows. Vinda da T14; aberta.
- **P-0018:** o adaptador de servidor entre o protocolo e o jogo. Vinda de uma pergunta do dono depois da T18; aberta, para o `/planejar` da fatia 2.

A P-0014 teve o item 2 decidido na aprovação da spec (o TD-0015 entrou nesta fatia); o item 8, medir o custo da fatia, fica para a `/retrospectiva`.

Na retrospectiva (2026-11-26), a escada de erros subiu dois degraus. As conversões de sinal, com o teste do assert da fatia 0, viraram a **P-0019**: o PR abre em rascunho logo depois da lista de tarefas, para a CI ver cada push. O teste que passava sem provar o que dizia, com o do CA-09 da fatia 0, virou a **P-0020**: todo teste de recusa confere o motivo exato, e os testes com risco real de falso positivo são sabotados uma vez. As duas foram aceitas e aplicadas. O `NOLINT` sem motivo, primeira ocorrência, fica anotado. A P-0015 e a P-0018 foram aceitas para a fatia 2, a P-0017 para o M7, e a P-0014 fechou com a medição do custo da fatia. A próxima fatia é a fatia 2, a gravação `.mucap` e o replay.

O detalhe de cada uma está em [PROPOSALS.md](../../docs/PROPOSALS.md).

**Para levar à sessão da central** (o bloco 3; não são propostas do Nexum, porque a central é quem muda a spec de referência): a IV.12 ainda oferece um radar no canto "como opção do modo Enhanced"; o Apêndice G não lista o `light-mood.md`; e a data do front matter da spec de referência ficou em 2026-11-20, depois das correções de 2026-11-22.

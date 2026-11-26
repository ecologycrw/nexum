# Relatório da fatia 1: A rede até o mundo

> Rascunho em construção durante o `/implementar`. As seções de provas, sala limpa e teste do dono são preenchidas no `/fechar`. Por enquanto, aqui ficam os desvios do plano e as travas alteradas, tarefa a tarefa.

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
- **Um teste de pacote sem pedir memória** (T11): o `IsPacket` (`Mu/Protocol/PacketFields.hpp`) diz qual pacote chegou sem montar texto. O `CheckPacket`, que o código gerado usa em toda leitura, monta o nome do pacote num `std::string` mesmo quando tudo dá certo; isso fica para a T16, que mede o quadro com mensagens de rede.
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

  Ao restaurar, a janela volta a 60 quadros por segundo. A medição oficial do CA-13, no mundo, fica para o teste do dono.
- **O servidor de teste dispensa o repetidor** (T17). O plano previa conferir se a imagem 0.10.1 aceita um IP fixo no `RESOLVE_IP`. Ela aceita: com `RESOLVE_IP=127.0.0.1`, os Game Servers se registram em `127.0.0.1:45901` a `45906`, que são as portas publicadas pelo `compose.yaml`. O `MuClient` segue o endereço do Connect Server sem nenhum repetidor, e as portas não precisaram ir para 127.127.127.127. A imagem, com a impressão digital da spec, já estava no computador do dono (a do laboratório), e nada foi baixado.
- **O teste de integração só roda com o endereço configurado** (T17). Os dois testes de `tests/integration/openmu` levam a etiqueta `openmu` e leem o endereço do Connect Server na variável `NEXUM_OPENMU_ADDRESS`. Sem ela, como na CI, o CTest os marca como pulados ([TD-0034](../../docs/TECH_DEBT.md)). Eles usam o mesmo verificador dos testes do `MuClient` (`CheckClientRun.cmake`). A senha errada usa a conta `test1`, para não depender da `test0`, que pode estar saindo do servidor. Primeira rodada, em 2026-11-26, contra o OpenMU 0.10.1 local:

  | Teste | Resultado |
  |---|---|
  | `openmu_muclient_stays_three_minutes_in_the_world` | passou em 181,7 s: 3 servidores na lista, o Game Server na porta 45902 (a que o Connect Server indicou), o login aceito, o personagem `test0Dk` no mapa 0 em (138, 128), 6 objetos na visão, 9 pings e nenhuma linha de erro no log do cliente |
  | `openmu_muclient_reports_a_wrong_password` | passou em 1,6 s: "the login was refused: invalid password (code 0)", e a sessão encerra sem entrar no mundo |

  O log do servidor não teve nenhum erro e nenhum aviso. Depois da partida, as únicas linhas são as duas recusas de senha da `test1` ("Account not found or invalid password"), uma da conferência à mão e outra do teste. O erro do Castle Siege que o passo 9 do teste do dono cita não apareceu nesta partida: só a linha de informação "Castle Siege initialized".

## Travas alteradas

- **`vcpkg.json`** (T01): entrou o Asio 1.38.2, para o transporte TCP. Aprovado pelo dono em 2026-11-25 ("Sim, todas"), com o download pelo vcpkg.
- **`cmake/NexumPacketGen.cmake`** (T03, arquivo novo): a função `nexum_generate_packets`, que roda o `packetgen` no build, e a busca do Python 3.10, com uma mensagem clara se ele faltar. Aprovado pelo dono na mesma resposta.
- **`cmake/NexumFuzzing.cmake`** (T09, arquivo novo): a opção `NEXUM_FUZZING` e a função `nexum_add_fuzz_target`. Sem a opção, cada alvo vira um teste que roda as sementes dele nos 3 sistemas; com ela, só no Clang, vira também um programa do libFuzzer. Aprovado pelo dono em 2026-11-25 ("Sim, todas").
- **`CMakePresets.json`** (T09): o preset `linux-fuzz`, de configuração, de build e de teste. Mesma aprovação.
- **`.github/workflows/ci.yml`** (T09): o passo "Fuzzing" no job do Linux com Clang, que compila o preset `linux-fuzz` e roda cada alvo por 60 s a partir das sementes. Mesma aprovação.
- **`cmake/NexumPacketGen.cmake`** (entre a T13 e a T14, [P-0016](../../docs/PROPOSALS.md)): a função `nexum_generate_packets` pede ao `packetgen` a lista dos arquivos quando o CMake configura (`--list`), declara todos como saídas da regra e configura de novo quando um XML ou o script muda. Os parâmetros passaram a ser a pasta dos cabeçalhos (`INCLUDE_DIR`) e o começo do nome dos testes (`TESTS_PREFIX`). O dono escolheu a opção 1 da proposta em 2026-11-26 ("Escolha a opção 1") e confirmou a edição da trava.
- **`scripts/dev.py`** (entre a T10 e a T11, pela primeira CI do PR #11): o `tidy` compila o build de desenvolvimento antes de analisar, porque o `clang-tidy` lê os cabeçalhos gerados dos pacotes, e o job da análise estática na CI só configurava. Aprovado pelo dono em 2026-11-26 ("Sim, pode mudar").

## Itens provisórios

Saíram do [TECH_DEBT](../../docs/TECH_DEBT.md) na T18:

| Item | Como foi resolvido |
|---|---|
| TD-0015 | Fora da tela (minimizada, escondida ou coberta), o laço do `MuClient` não envia o quadro para a placa e espera até o próximo tick (T15). Minimizado, no build de jogador, o processador caiu de 92% para cerca de 0,3% a 0,7% de um núcleo (o detalhe está em "Desvios do plano"). A medição no mundo é o passo 5 do teste do dono |
| TD-0020 | O `LoadPng` confere o tamanho no cabeçalho do PNG antes de ler os pixels (T09). O teste `[0001-CA-10]` do `PngTest.cpp` recusa um arquivo de 33 bytes que declara 60.000 × 60.000 pixels |
| TD-0023 | A estrutura de fuzzing existe (T09, `cmake/NexumFuzzing.cmake`): cada leitor tem um alvo, inclusive o `ParseVersion` e o `LoadPng`, da fatia 0. As sementes rodam como testes comuns nos 3 sistemas, e a CI roda cada alvo por 60 s no Linux com Clang |

Entraram nesta fatia, cada um com o destino:

| Item | O que é | Destino |
|---|---|---|
| TD-0026 | o fuzzing roda 60 s por alvo na CI, e não 1 hora | M2 |
| TD-0027 | os prazos das respostas da sessão são fixos, de 10 s | M7 |
| TD-0028 | uma queda ou uma recusa encerra a sessão, sem reconexão | M7 |
| TD-0029 | a visão do mundo é uma tabela por id, sem entidades | M4 |
| TD-0030 | os personagens transformados e as invocações não são traduzidos | M4 |
| TD-0031 | os itens no chão são ignorados, e os efeitos dos jogadores vêm só contados | M5 |
| TD-0032 | a conta, o servidor e o personagem vêm da linha de comando, e a senha do terminal | M6 |
| TD-0033 | o mapa é dado por pronto assim que os dados do personagem chegam | M3 |
| TD-0034 | o teste de integração contra o OpenMU roda só no computador do dono | M7 |
| TD-0035 | os vetores vêm dos documentos da central, sem o programa em C# da V.2.1 | M2 |

# Módulo: MU, Recording

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A gravação de uma sessão no mundo num arquivo `.mucap` e o que o replay usa para conferir que chegou ao mesmo lugar: o formato (a escrita e a leitura com todas as conferências), a impressão do estado da visão do mundo e o gravador. O replay em si é uma implementação do adaptador de servidor e fica no [MU: Online](mu-online.md). Esta área não abre arquivo: os bytes vão e vêm pelo `MuClient`, com o [Platform](platform.md). O `.mucap` é um formato do Nexum ([V.2.2 da spec de referência](../referencia/especificacao-engine-propria-caminho-b.md#v22-gravação-e-replay)), e só o build de desenvolvimento grava e reproduz ([ADR 0035](../adr/0035-adaptador-de-servidor-gravacao-e-replay.md)).

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Recording`. Ela é compilada também no build de jogador, mas nada lá a chama |
| `MuClient` | Games | a ligação com as opções `--record` e `--replay`, em `Games/MuOnline/App/Private/RecordingTools.cpp`, compilada só com `NEXUM_AUTHORING` |
| `muonline_recording` | Games | o alvo de fuzzing da leitura e do replay |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/Recording/`.

## Conceitos

- **A gravação** começa na entrada no mundo e guarda só o que chega do servidor: as mensagens do mundo já decifradas, as mudanças de época, a marca de cada tick que tirou eventos da fila, os pontos de controle e o fim. O que o cliente envia nunca entra, nem a lista de servidores, o login ou a lista de personagens. Assim, a conta e a senha nunca chegam ao arquivo (INV-18).
- **`RecordingWriter`** (`Recording.hpp`): escreve o cabeçalho e os registros num espaço reservado uma vez, que sempre guarda lugar para o fim. Quando um registro não cabe, o escritor fica cheio e só aceita o fim, e a gravação continua válida. Ele só se move, porque uma cópia perderia o espaço.
- **`ParseRecording` e `ReadRecording`** (`Recording.hpp`): conferem a gravação inteira antes de devolvê-la, na ordem da seção "A leitura", e recusam com o primeiro motivo achado. A `Recording` devolvida guarda os bytes, as contagens (`Messages`, `Ticks`, `Checkpoints` e `End`) e um cursor que lê os registros em ordem sem pedir memória. O `ReadRecording` recusa um arquivo acima de 64 MiB antes de ler o conteúdo. Os motivos não citam o caminho: quem chama o junta.
- **`StateDigestOf`** (`StateDigest.hpp`): a impressão do estado da visão do mundo, na versão 1. Não pede memória, então pode ser calculada dentro do quadro.
- **`Recorder`** (`Recorder.hpp`): o gravador que o `LiveServer` avisa da entrada no mundo, de cada mensagem do mundo antes da tradução, de cada mudança de época e de cada tick ([MU: Online](mu-online.md)). O relógio da gravação começa na entrada no mundo, com o tempo 0, e cada tempo é arredondado para o microssegundo e nunca volta. O ponto de controle vem no primeiro tick a partir de cada 5 s desde a entrada (5, 10, 15 s...). O `Finish` grava o fim. Antes, quando o último registro é um tick sem ponto de controle, ele grava o ponto de controle do fim. Depois de uma mensagem isso não acontece, porque um ponto de controle só vem logo depois de um tick: por isso o `MuClient` aplica os eventos que esperam antes de terminar. O espaço do `MuClient` é de 16 MiB (`kRecorderRoom`). Quando ele enche, o log avisa uma vez, na categoria `recording`, e o fim fica marcado como "cheio".
- **Um tick sem eventos não é gravado:** o cliente online só avisa o servidor de um tick que tirou eventos da fila. Por isso, uma sessão parada tem poucos pontos de controle: 60 s no mundo, com um jogador entrando na visão e saindo, deram 6 ticks e 2 pontos de controle.

## O formato `.mucap`, versão 1

Tudo em little-endian. Um **texto** é 1 byte com o tamanho, de 0 a 64, seguido de tantos bytes em UTF-8.

**O cabeçalho**

| Bytes | Campo | Valor |
|---|---|---|
| 0 a 7 | marca | `4E 58 4D 55 43 41 50 1A` (`NXMUCAP` e o byte `1A`) |
| 8 e 9 | versão do formato | `01 00` |
| 10 e 11 | versão da impressão do estado | `01 00` |
| a partir do 12 | versão do Nexum | texto: a versão do `--version`, como `0.0.0` |
| em seguida | versão do dialeto | texto: `20404` (`Mu/Protocol/Dialect.hpp`) |
| em seguida | servidor | texto: o `--server-version`, ou `unknown` |

**Os registros** vêm um depois do outro, até o fim do arquivo. Cada um tem o tipo (1 byte), o tamanho do conteúdo (4 bytes) e o conteúdo. O conteúdo começa sempre pelo tempo (8 bytes), em microssegundos desde a entrada no mundo, pelo relógio monotônico do quadro. A época tem 4 bytes, e o id do herói, 2.

| Tipo | Registro | Tamanho do conteúdo | Conteúdo |
|---|---|---|---|
| 1 | entrada no mundo | 14 | tempo (8), época (4), id do herói (2) |
| 2 | mensagem | 15 a 65.547 | tempo (8), época (4), a mensagem do mundo decifrada, como a sessão a entrega à tradução, de 3 a 65.535 bytes |
| 3 | mudança de época | 12 | tempo (8), época nova (4) |
| 4 | tick | 12 | tempo (8), quantos eventos o tick tirou da fila, aplicados ou descartados pela época (4) |
| 5 | ponto de controle | 28 | tempo (8), mensagens desde o começo (4), impressão do estado (8), objetos na visão (4), itens no inventário (4) |
| 6 | fim | 9 | tempo (8), como acabou (1): `0` o cliente fechou ou o tempo pedido no mundo passou, `1` a sessão falhou, `2` o espaço da gravação encheu |

Por exemplo, o fim de uma gravação que o cliente fechou depois de 20 s no mundo (20.000.000 µs) são os 14 bytes `06 09 00 00 00 00 2D 31 01 00 00 00 00 00`: o tipo 6, o tamanho 9, o tempo e o motivo 0.

Uma sessão que falhou antes do mundo grava só o cabeçalho e o fim. A gravação de referência (`Games/MuOnline/Tests/Recordings/openmu-two-players.mucap`) tem 1.426 bytes: 23 mensagens, 6 ticks e 2 pontos de controle.

### A leitura

A leitura confere, nesta ordem, e recusa com o motivo exato:

1. o tamanho do arquivo, até 64 MiB, antes de ler ("the file ... has N bytes, more than the limit of L");
2. a marca ("the recording does not start with the mark of a .mucap"), as duas versões ("the format version 2 is unknown: this Nexum reads the version 1") e cada texto do cabeçalho, inteiro e com até 64 bytes;
3. cada registro inteiro dentro do arquivo ("the record at byte N is cut"), com o tipo conhecido ("has the unknown type 9") e o tamanho exato do tipo ("has 13 bytes of content, not 14"), e o fim com um motivo conhecido;
4. o tempo que nunca volta ("goes back in time");
5. no máximo uma entrada no mundo, antes de qualquer mensagem, mudança de época ou tick ("comes before the entry in the world", "is the second one");
6. cada ponto de controle logo depois de um tick ("does not come right after a tick") e com a contagem certa das mensagens até ali;
7. um único fim, como último registro ("comes after the end", "the recording has no end").

### A impressão do estado, versão 1

É o FNV-1a de 64 bits ([Core](core.md)) sobre os campos abaixo, nesta ordem. Cada número entra com o próprio tamanho, em little-endian, e cada marca de sim ou não entra como 1 byte, `0` ou `1`:

1. **O herói:** carregado (1), id (2), mapa (2), posição x (1) e y (1); a vida (4), o escudo (4), a mana (4), a habilidade (4), a velocidade de ataque (2) e a de magia (2); os máximos da vida, do escudo, da mana e da habilidade (4 cada).
2. **A soma das impressões dos objetos na visão** (8). A impressão de cada objeto é o FNV-1a de: id (2), tipo de objeto (1: `0` jogador, `1` NPC ou monstro), posição (x e y, 1 cada), destino (x e y), rotação (1), nome (o tamanho em 1 byte, e os bytes), aparência, número de efeitos (1) e número do tipo (2). A aparência é a classe (1), a pose (1), o conjunto antigo completo (1), o GM (1), os 7 itens vestidos, as asas, o pet e a variante do Fenrir (1). Cada item vestido é: presente (1), grupo (1), número (2), brilho (1), excelente (1) e antigo (1).
3. **A soma das impressões dos itens do inventário** (8). A impressão de cada item é o FNV-1a de: slot (1), grupo (1), número (2), nível (1), durabilidade (1), tem opção (1), tipo (1) e nível (1) da opção, sorte (1), skill (1), excelentes (1), tem antigo (1), conjunto (1) e nível do bônus (1) do antigo, tem harmonia (1), número (1) e nível (1) da harmonia, opção 380 (1), tem sockets (1), bônus dos sockets (1), quantidade de sockets (1) e os 15 sockets (1 cada).
4. **A quantidade de objetos e a de itens** (4 cada).
5. **A contagem das mudanças do terreno e a dos eventos ignorados** (8 cada).

As somas são módulo 2⁶⁴. Assim, a ordem das tabelas não muda a impressão, e nada precisa ser ordenado. Quando a visão do mundo mudar de forma (as entidades do M4, [TD-0029](../TECH_DEBT.md)), a impressão ganha uma versão 2, e a leitura recusa a versão que não conhece, em vez de o replay acusar uma diferença.

## Como usar

- **Gravar:** `MuClient --connect ... --account ... --record ARQUIVO.mucap [--server-version TEXTO]`, só no build de desenvolvimento. O `MuClient` escreve o cabeçalho antes de pedir a senha, para recusar um caminho ruim antes de conectar, e o arquivo inteiro depois do último quadro, fora do laço ([DEV.md](../setup/DEV.md#gravar-e-reproduzir-uma-sessão)).
- **Reproduzir:** `MuClient --replay ARQUIVO.mucap` lê e confere a gravação antes de a janela abrir e a entrega ao `ReplayServer` ([MU: Online](mu-online.md)).
- **No código:** os testes montam gravações com o `RecordingWriter` (`Games/MuOnline/Tests/RecordingTest.cpp`) e gravam sessões de teste com o `Recorder` e o transporte de teste (`Tests/ScriptedTransport.hpp`, em `RecorderTest.cpp`).

## Fontes da central

Não se aplica: o `.mucap`, a impressão e o gravador são do próprio Nexum (a V.2.2, a IV.18 e a III.8 da spec de referência). As mensagens gravadas são as do OpenMU, lidas pela mesma tradução da fatia 1, cujas fontes estão no [MU: Translate](mu-translate.md).

## Invariantes relevantes

- **INV-06:** o arquivo é lido antes da janela abrir e escrito depois do último quadro; o quadro nunca espera pelo disco.
- **INV-12:** a leitura confere a gravação inteira antes do uso, com os limites acima, e passa por fuzzing.
- **INV-13:** gravar e tirar a impressão não pedem memória dentro do quadro (CA-10 da fatia 2).
- **INV-18:** a gravação começa no mundo e nunca vê o que o cliente envia, então a conta e a senha não chegam ao arquivo.
- **INV-19:** o build de jogador recusa `--record`, `--server-version` e `--replay`.

## Testes

- Em `Games/MuOnline/Tests/` (`muonline_tests`): `RecordingTest.cpp` (os bytes fixos de uma gravação pequena, a ida e a volta e cada recusa com o motivo exato), `StateDigestTest.cpp` (os valores fixos da visão vazia e de uma cena cheia, a ordem que não importa e cada campo que importa) e `RecorderTest.cpp` (os registros, os pontos de controle a cada 5 s, o espaço cheio e a conta e a senha fora do arquivo).
- O alvo de fuzzing `muonline_recording` (`RecordingFuzzTest.cpp`, com as sementes em `FuzzSeeds/Recording`, inclusive uma cópia da gravação de referência) lê os bytes e, quando a gravação vale, a reproduz inteira.
- O quadro sem alocação gravando: os casos `[0002-CA-10]` de `FrameAllocationTest.cpp` (`muonline_frame_allocation_tests`).
- A gravação de referência e o teste de integração que grava e reproduz estão no [MU: Online](mu-online.md).

## Itens provisórios

- [TD-0038](../TECH_DEBT.md): os comandos do jogador não entram na gravação, até o `.mucap` versão 2, com o andar.
- [TD-0039](../TECH_DEBT.md): a gravação fica na memória, com até 16 MiB, e só é escrita quando o cliente termina.

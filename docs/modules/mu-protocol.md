# Módulo: MU, Protocol

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O protocolo do cliente 106.3 com o OpenMU, em bytes: o enquadramento das mensagens, a cifra no papel do cliente, a tabela do dialeto (qual variante de cada pacote o Nexum usa), as estruturas escritas à mão e a transformação de bytes que a engine roda na thread de cada conexão. Não guarda estado de sessão e não decide nada do jogo: a ordem da conversa é da [sessão](mu-session.md), e o significado dos pacotes do mundo é da [tradução](mu-translate.md).

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Protocol` |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/Protocol/`. O código gerado dos pacotes fica na pasta de build, em `Mu/Protocol/Packets/` ([PacketGen](packetgen.md)). Os componentes neutros que o item e a aparência viram ficam em `Mu/Items/Item.hpp` e `Mu/Characters/Appearance.hpp`.

## Conceitos

- **`FrameReader`** (`FrameReader.hpp`): recebe os bytes em pedaços de qualquer tamanho e entrega as mensagens inteiras, dos tipos `C1` a `C4`, sem cópia quando a mensagem chega inteira. Um primeiro byte desconhecido, um tamanho 0 ou um tamanho menor que o cabeçalho é um erro que fecha a conexão.
- **A cifra** (`Cipher.hpp`): o SimpleModulus com o contador de cada direção, o Xor32 em tudo o que o cliente envia e o Xor3 nos campos do login. Um contador errado, um checksum errado ou um bloco inválido é um erro, sem tentar ressincronizar. As chaves são constantes públicas: é ofuscação, não segurança.
- **`ClientCodec`** (`ClientCodec.hpp`): a transformação de bytes do cliente, que implementa o `MessageCodec` do [Net](net.md). Na entrada, o enquadramento e o SimpleModulus nos `C3` e `C4`. Na saída, o Xor32 e o SimpleModulus. O Connect Server usa só o enquadramento. Cada conexão tem o seu, porque os dois contadores começam do 0 a cada conexão.
- **O dialeto** (`Dialect.hpp` e `Dialect/`): o único lugar do código que sabe quais variantes geradas o 106.3 usa. O `Dialect.hpp` guarda só a identidade do cliente: a versão `20404`, o número de série `NEXUM-CLIENT-001` e a porta 44406 do Connect Server. A tabela de variantes vem em quatro partes, uma por etapa da conversa: `ConnectServer.hpp`, `Login.hpp`, `FirstEntry.hpp` e `World.hpp`. Cada arquivo inclui só as partes que usa. Os códigos que mudam com o idioma são os ingleses.
- **As estruturas escritas à mão:**
  - `ReadExtendedItem` (`ExtendedItem.hpp`): o item estendido, de 5 a 15 bytes, vira um `mu::items::Item`, com o tamanho calculado pelas opções;
  - `ReadExtendedAppearance` (`ExtendedAppearance.hpp`): a aparência estendida, de 27 bytes, vira um `mu::characters::Appearance`;
  - `CharacterInformation.hpp`: os dados do personagem (`C3 F3 03`), com 92 ou 93 bytes;
  - `LoginMessage.hpp` (`WriteLogin`): o login de 60 bytes, com o Xor3 no usuário e na senha.

  Com dados cortados, o item e a aparência leem um byte que falta como 0, como a spec descreve, e nunca leem além do fim.
- **`PacketFields`** (`PacketFields.hpp`): as peças que o código gerado usa para ler e escrever os campos pela posição. O `CheckPacket` e o `StartPacket` só montam texto quando há um erro, para a leitura de um pacote não pedir memória.
- **`PacketLog`** (`PacketLog.hpp`): os pacotes como o log os mostra, com a direção, o tamanho e os bytes. O usuário e a senha de um login enviado aparecem como `**`.

## Como usar

A sessão monta e lê os pacotes pelas partes do dialeto, como em `Games/MuOnline/Private/Session/Session.cpp`. O `NetworkTransport` cria um `ClientCodec` para cada conexão ([MU: Online](mu-online.md)).

## Fontes da central

A base é o OpenMU `b53c704c6`. A spec de protocolo é `docs/specs/protocolo/protocolo-106-3-m2.md`, e os achados ficam em `knowledge/protocolo/`:

- o enquadramento: §3;
- a cifra: §4.1 a §4.5 e o achado `2026-11-21-cifra-simplemodulus-xor32-xor3.md`, com as chaves e os vetores V1 a V9;
- o dialeto: §5.1 e §5.2 e os achados `2026-11-21-variantes-por-versao-m2.md` e `2026-11-21-codigos-por-idioma.md`;
- o item e a aparência: §7.1 e §7.2 e os achados `2026-11-21-item-estendido.md` e `2026-11-21-aparencia-estendida.md`;
- os dados do personagem e o login: §6.6 e §6.11;
- os vetores das capturas reais: o achado `2026-11-21-capturas-reais-sessao-106-3.md`, §3.

A lista completa, com o que cada fonte descreve, está no [plano da fatia 1](../../specs/0001-rede-ate-o-mundo/plan.md#fontes-da-central-usadas).

## Invariantes relevantes

- **INV-12:** todo byte de fora é lido pela posição e conferido antes. Nenhum pacote é lido por um *cast* de struct.
- **INV-09:** a cifra e o contador rodam só na thread de rede, na ordem do fio.
- **INV-13:** ler um pacote não pede memória.
- **INV-16:** tudo vem das specs e dos achados da central e dos XML do OpenMU. Nada vem do código do MuMain.

## Testes

Em `Games/MuOnline/Tests/` (`muonline_tests`):

- `FrameReaderTest.cpp`, `CipherTest.cpp`, `ClientCodecTest.cpp` e `DialectTest.cpp` (um teste para cada linha da tabela de variantes);
- `ExtendedItemTest.cpp` (as 32 + 224 combinações), `ExtendedAppearanceTest.cpp` e `PacketLogTest.cpp`;
- `PacketExamplesTest.cpp`, com os pacotes de exemplo da central lidos e escritos de novo.

Os testes gerados de cada pacote ficam no `muonline_packet_tests`. Cada leitor tem um alvo de fuzzing (`*FuzzTest.cpp`, com as sementes em `FuzzSeeds/`): o enquadramento, a cifra, o item, a aparência e os pacotes do dialeto. As sementes rodam como testes comuns nos 3 sistemas (`muonline_*_seeds`), e a CI roda cada alvo por 60 s no Linux com Clang.

## Itens provisórios

- [TD-0026](../TECH_DEBT.md): a CI roda cada alvo de fuzzing por 60 s, e não 1 hora.
- [TD-0035](../TECH_DEBT.md): os vetores vêm dos documentos da central, sem o programa em C# que gera vetores com a biblioteca do OpenMU.

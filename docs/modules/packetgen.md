# Módulo: PacketGen

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O gerador do codec ([ADR 0034](../adr/0034-packetgen-em-python-no-build.md)). O `packetgen` lê os XML de pacotes do OpenMU e gera o C++ que lê e escreve todos os pacotes, nas duas direções, mais um teste para cada pacote. Ele não escolhe quais variantes o cliente 106.3 usa: isso é da tabela do dialeto, escrita à mão no [MU: Protocol](mu-protocol.md). É um script em Python 3, só com a biblioteca padrão, que roda no build e não entra em nenhum programa.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `packetgen_script_tests` | Tools | os testes do script em Python (teste do CTest) |
| `packetgen_tests` | Tools | o C++ gerado de um XML de teste próprio, conferido byte a byte |

O script fica em `Tools/PacketGen/Private/packetgen.py`. A função `nexum_generate_packets`, de `cmake/NexumPacketGen.cmake`, roda o script em todo build, também no de jogador, porque o jogo lê os pacotes com o código gerado. A pasta `Tools/PacketGen` só entra no build de desenvolvimento, com os testes do gerador (INV-02).

## Conceitos

- **O que entra:** os quatro XML de pacotes de `third_party/openmu` (`ConnectServerPackets.xml`, `ClientToServerPackets.xml`, `ServerToClientPackets.xml` e `ChatServerPackets.xml`, com 613 pacotes ao todo) e os enums comuns do `CommonEnums.xml`, da base `b53c704c6` ([THIRD_PARTY](../THIRD_PARTY.md)). O `PacketHeaders.xml` e o esquema `PacketDefinitions.xsd` ficam junto, como referência do formato: o script não os lê, e as estruturas de cabeçalho do `PacketHeaders.xml` (o primeiro byte, o tamanho do cabeçalho e se há subcódigo) estão escritas nele.
- **O que sai,** na pasta de build (`build/<preset>/Games/MuOnline/Generated/`):
  - um cabeçalho por pacote, `Mu/Protocol/Packets/<Arquivo>/<Pacote>.hpp`, com as constantes (o tipo do cabeçalho, o código, o subcódigo e o tamanho), a leitura (`View::Read`, que confere o cabeçalho, o código, o subcódigo e o tamanho mínimo antes de dar acesso aos campos) e a escrita (`Writer`, que monta o pacote num espaço dado);
  - os tipos comuns de cada arquivo (`Types.hpp`), com os enums do XML;
  - os testes gerados, em arquivos de até 40 pacotes, que conferem os bytes fixos, o tamanho e a recusa de um pacote menor que o mínimo.
- **Cada campo** é lido e escrito pela posição, com a ordem dos bytes e o deslocamento de bits que o XML declara, nunca por um *cast* de struct. As peças comuns de leitura e escrita ficam escritas à mão, em `Mu/Protocol/PacketFields.hpp`.
- **Nenhum cabeçalho junta todos os pacotes** ([P-0016](../PROPOSALS.md)): cada arquivo do jogo inclui só as partes do dialeto que usa, e cada parte inclui só os pacotes dela. Os testes do script conferem isso.
- **`--list`:** o CMake pede ao script a lista dos arquivos que ele vai gerar quando configura, e configura de novo quando um XML ou o script muda.

## Como usar

O módulo MU chama `nexum_generate_packets` no `Games/MuOnline/CMakeLists.txt`. Os nomes dos campos são os do XML, como `dialect::Login::Writer::SetUsername`. Para seguir um OpenMU novo: copiar os XML de uma base nova para `third_party/openmu`, compilar e refazer a tabela do dialeto.

## Fontes da central

O gerador só lê os XML do OpenMU (MIT), em `Server/src/Network/Packets/` na base `b53c704c6`. O que cada pacote significa para o 106.3 vem da spec de protocolo da central, pela tabela do dialeto ([MU: Protocol](mu-protocol.md)).

## Invariantes relevantes

- **INV-02:** o script é uma ferramenta do build. Os testes dele só existem no build de desenvolvimento, e o código gerado é a única coisa que chega ao jogo.
- **INV-12:** a leitura gerada confere o tamanho antes de qualquer campo e nunca lê além do fim.
- **INV-14:** o script em Python pode usar exceções (é `Tools/`). O C++ gerado devolve os erros por valor.

## Testes

- `Tools/PacketGen/Tests/test_packetgen.py` (`packetgen_script_tests`): a leitura dos XML, as recusas e os arquivos gerados, inclusive que nenhum cabeçalho junta todos os pacotes.
- `Tools/PacketGen/Tests/PacketGenTest.cpp` (`packetgen_tests`): o C++ gerado do `Fixture.xml`, um XML de teste próprio com cada tipo de campo, conferido byte a byte.
- `muonline_packet_tests`: os testes gerados de todos os pacotes dos XML do OpenMU.

## Itens provisórios

Nenhum. Os seis campos que usam um enum do outro arquivo (por exemplo, o `ClientToServerPackets.xml` usa o `ServerToClient.GuildMemberRole`) ficam com o byte cru, porque os dois arquivos se citam. Nenhum deles é usado ainda (report da [fatia 1](../../specs/0001-rede-ate-o-mundo/report.md)).

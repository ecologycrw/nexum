# Módulo: Debug

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O log da engine, por categoria e por nível, e o profiler Tracy escondido atrás de macros. O log não é global: cada peça recebe o seu `Logger`. O profiler fica desligado por padrão e só existe no build de desenvolvimento.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_debug` (`Nexum::Debug`) | Engine | biblioteca; liga o Tracy só com `NEXUM_PROFILER=ON` |

Cabeçalhos públicos em `Engine/Debug/Public/Nexum/Debug/`. A regra da opção do profiler fica em `Engine/Debug/ProfilerOption.cmake`.

## Conceitos

- **`Logger`** (`Logger.hpp`): recebe a saída (`LogSink`) no construtor. Filtra por nível (trace, debug, info, warning e error), e o nível mínimo pode mudar com o programa rodando. Cada mensagem leva a sua categoria (`LogCategory`), com um nome fixo, como `render` ou `time`. Escrever nunca pede memória.
- **`LogSink`**: a saída trocável. Os testes usam uma saída em memória (`tests/support/Public/Nexum/TestSupport/MemoryLogSink.hpp`).
- **`TerminalLogSink`** (`TerminalLogSink.hpp`): uma linha por mensagem na saída de erro, como `[warning] render: the swapchain was rebuilt`, escrita numa chamada só.
- **`LogLine`** (`LogLine.hpp`): uma linha de log montada no lugar, num espaço de tamanho fixo, com textos e números em decimal ou em hexadecimal. Montar a linha nunca pede memória, então o código do caminho quente pode dizer o que fez, como a tradução e a visão do mundo do MU fazem. O que não cabe é cortado, e o `Cut()` diz isso.
- **O profiler** (`Profiler.hpp`): as macros `NEXUM_PROFILE_SCOPE("nome")` e `NEXUM_PROFILE_FRAME()`.
  - Desligado, elas não geram código nenhum.
  - Ligado, cada trecho medido se registra uma vez numa tabela fixa de 4.096 lugares (`kMaxProfileSites`), e o Tracy roda no modo sob demanda.
  - Só o `Engine/Debug/Private/Profiler.cpp` vê o Tracy.
  - O build de jogador recusa `NEXUM_PROFILER=ON` antes de o vcpkg começar.

As mensagens do próprio SDL chegam a este log pela `SdlLogRoute`, que fica no módulo [Platform](platform.md), porque só ele pode ver o SDL.

## Como usar

Cada peça recebe um `Logger&` e escreve com uma categoria própria. O `MuClient` cria o log com o `TerminalLogSink` e o entrega às peças. Para ligar o profiler, veja o [DEV.md](../setup/DEV.md).

## Fontes da central

Não se aplica: o módulo não reproduz nada do original.

## Invariantes relevantes

- **INV-10:** o Tracy fica atrás de um único módulo.
- **INV-19:** o build de jogador nunca tem o profiler.
- **INV-13:** escrever no log e montar uma linha com o `LogLine` não pedem memória.

## Testes

Em `Engine/Debug/Tests/`: `LoggerTest.cpp`, `LogLineTest.cpp` e `ProfilerTest.cpp` (`nexum_debug_tests`), `TerminalLogSinkTest.cpp` (`nexum_debug_terminal_log_tests`, que confere a linha que chega ao terminal) e os 3 testes do CTest `nexum_debug_profiler_*`, que rodam a regra da opção com cada combinação. Na CI, o passo "Build with the profiler" compila tudo com o Tracy e roda estes testes no Linux e no macOS.

## Itens provisórios

- [TD-0011](../TECH_DEBT.md): o log só vai para o terminal, de dentro do quadro e sem fila; o SDL filtra as próprias mensagens.
- [TD-0012](../TECH_DEBT.md): o build com o profiler não tem preset, e na CI não roda no Windows.

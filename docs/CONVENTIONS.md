# Convenções

> Aqui fica só o que as ferramentas não verificam sozinhas. Formatação, avisos, funções proibidas, nomes e dependências entre camadas são garantidos por `.clang-format`, `.clang-tidy`, sanitizers e `scripts/checks`. Este documento faz o papel do `CODING_RULES.md` citado na spec ([ADR 0003](adr/0003-linguagem-build-e-dependencias.md), [ADR 0019](adr/0019-convencoes.md)).

## Idiomas

| O quê | Idioma |
|---|---|
| Código, identificadores, comentários no código, mensagens de commit | inglês |
| Specs, planos, relatórios, ADRs, STATUS e demais docs do projeto | português |
| Nomes de arquivos, pacotes e campos de formato do MU | como no original, para conseguir comparar com as specs da central |

## Nomes

| O quê | Padrão | Exemplo |
|---|---|---|
| Pastas do produto e módulos | `PascalCase` | `Engine/Platform`, `Kit/GridRpg` |
| Pastas da infraestrutura | minúsculas | `scripts/checks`, `tests/architecture` |
| Alvos do build da engine e do kit | `nexum_<módulo>`, com o alias `Nexum::<Módulo>` | `nexum_core` / `Nexum::Core` |
| Alvos do módulo MU | `MuOnline` (biblioteca), `MuClient` (executável) | — |
| Namespaces | `nexum::<módulo>` na engine, `nexum::gridrpg` no kit, `mu::<área>` no módulo MU | `nexum::core`, `mu::protocol` |
| Arquivos C++ | `PascalCase`, com `.hpp` e `.cpp`, e o nome do tipo principal | `TickClock.hpp` |
| Includes públicos | `Nexum/<Módulo>/<Arquivo>.hpp`, `Nexum/GridRpg/<Área>/<Arquivo>.hpp`, `Mu/<Área>/<Arquivo>.hpp` | `Nexum/Core/Result.hpp` |
| Tipos | `PascalCase` | `TickClock` |
| Funções | `PascalCase` | `AdvanceTime` |
| Variáveis e parâmetros | `camelCase` | `elapsedTicks` |
| Membros privados e protegidos | `m_camelCase` | `m_elapsed` |
| Membros públicos de structs simples | `camelCase` | `major` |
| Constantes (`constexpr`, globais, estáticas) | `kPascalCase` | `kTickMilliseconds` |
| Macros (evitar) | `NEXUM_MAIUSCULAS` | `NEXUM_ASSERT` |
| Identificadores | tipos fortes, nunca `int` cru | `EntityId`, `ServerObjectId`, `MapEpoch` |
| Testes | `<Módulo>/Tests/<Arquivo>Test.cpp` | `Engine/Core/Tests/VersionTest.cpp` |
| Specs | `specs/NNNN-nome-em-kebab-case/` | `specs/0000-fundacao/` |
| ADRs | `docs/adr/NNNN-nome-em-kebab-case.md`, em ASCII | `docs/adr/0005-tick-fixo-de-25-hz.md` |

Funções que a linguagem exige em minúsculas (`begin`, `end`, `size`, `data`, `empty`, `swap`) ficam em minúsculas.

## Convenções do mundo

| Tema | Convenção |
|---|---|
| Eixos | X e Y no chão, **Z para cima** |
| Unidades | as do MU: **100 unidades = 1 célula** |
| Ângulos | graus nos dados e nas ferramentas; radianos no código |
| Texturas | origem no canto superior esquerdo; o importador inverte o que vier de baixo para cima |
| Tempo | simulação em ticks inteiros; relógio de parede em segundos (`double`) |
| Texto | UTF-8 em tudo; conversão só na borda com o sistema operacional |
| Caminhos | sempre pelo VFS, com barra normal |
| Lateralidade do mundo | *lacuna*: confirmada no M1 com uma cena de teste importada. Não chutar |

## Commits

Formato: `tipo(escopo): descrição curta em inglês`

- `tipo`: `feat`, `fix`, `docs`, `test`, `refactor`, `perf`, `build`, `ci` ou `chore`.
- `escopo`: o número da fatia (`0000`) ou a área (`prep`, `docs`, `ci`, `consolidation`).
- Dentro de uma fatia, a descrição começa pela tarefa: `feat(0000): T03 add the tick clock`.
- A linha `Co-Authored-By` do Claude fica no fim da mensagem.

## Padrões da casa

- **O servidor decide.** Ações do jogador são pedidos; o resultado vem de volta como evento neutro.
- **Significados injetados.** O kit recebe regras do módulo MU (o que bloqueia o andar, o tamanho do inventário). Ele não as conhece.
- **O desenho recebe um retrato** e não tem como escrever no mundo.
- **Referência por handle ou ID**, e não por ponteiro direto entre objetos de sistemas diferentes.
- **Dados onde há motivo para ajustar** (itens, monstros, parâmetros de efeitos, sons); código onde não há.
- **A interface pensa no destino, a implementação pensa no agora.**
- **Threads só nas fronteiras.** Rede e carga de arquivos rodam em segundo plano e conversam com a thread principal por filas.
- **Dependências explícitas:** cada objeto recebe o que precisa no construtor.

## Regras de C++

- **C++20**, sem extensões de compilador e sem módulos do C++20. Um recurso do C++23 só entra por ADR, quando os três compiladores o suportarem bem.
- **Memória com dono claro (RAII).** Use `std::unique_ptr` e containers. `new`, `delete`, `malloc` e `free` soltos são proibidos. `std::shared_ptr` só com um motivo escrito.
- **Handles geracionais** para o que pode morrer (entidades, recursos, vozes). Um handle velho é detectado, nunca acessa memória inválida.
- **Ponteiro cru e referência nunca são donos** de nada.
- **Erros por valor:** funções que podem falhar de forma esperada devolvem `Result<T>`. `assert` e log para bugs. **`throw` é proibido no runtime** (`Engine/`, `Kit/`, `Games/`); exceções só nas ferramentas offline (`Tools/`). Exceções de bibliotecas externas são tratadas na fronteira com elas.
- **`[[nodiscard]]`** em toda função cujo resultado não pode ser ignorado.
- **Buffers com tamanho:** use `std::span`, `std::string_view` e `std::array`. Funções de C sem limite de tamanho, como `strcpy` e `sprintf`, são proibidas.
- **Conversões explícitas:** nada de conversão de inteiros que possa perder valor sem checar.
- **Dados externos passam por um leitor com limites.** Nunca transforme bytes de rede ou de arquivo em struct com `reinterpret_cast` ou `memcpy` direto.
- **Sem estado global mutável.**
- **Tempo e sorteios** vêm das fontes da engine. Relógios do sistema só em `Engine/Time` e nas ferramentas de `Tools/`; `rand` e `srand` são proibidos em todo lugar ([ADR 0015](adr/0015-aleatoriedade-deterministica.md)).
- **O caminho quente não pede memória.** Arenas por quadro e por tick; reserve ao criar e reutilize.

## Antipadrões proibidos

- Singletons e "managers" globais.
- Herança profunda. Componentes são structs simples; sistemas são funções numa ordem fixa.
- Uma interface para cada classe "por precaução", e camadas sem motivo.
- Abstração antes da necessidade, inclusive um ECS genérico onde uma função direta resolve.
- Lógica de jogo dentro das funções de desenho.
- Thread nova, render graph ou cache sem uma medição que justifique.

## Régua de qualidade

O que um revisor sênior recusaria no Nexum, mesmo com o build e os testes verdes:

- **Função longa:** uma função que faz várias etapas em sequência. Cada etapa vira uma função com nome.
- **Classe com mais de uma responsabilidade:** se a descrição da classe precisa de "e também", ela se divide.
- **Código repetido:** na terceira cópia, vira uma função só (ver a [regra de três](#regra-de-três)).
- **Tratamento de erro que esconde o problema:** um erro ignorado, ou um `catch` que engole o que não sabe tratar.
- **Nome vago:** `Manager`, `Helper`, `Data`, `Info`, `Util`, `Handler`, `temp`. O nome diz o que a coisa é ou faz.
- **Teste que verifica a implementação e não o comportamento.** Para formatos binários e pacotes, os bytes são fixados no teste.
- **Número mágico sem fonte:** toda constante que reproduz o original cita a spec de onde veio.

Uma parte é verificada por máquina: o clang-tidy limita a complexidade das funções. O resto é conferido pelo revisor, na lista obrigatória dele.

## Regra de três

Uma abstração só nasce quando o mesmo problema aparece pela terceira vez. Na primeira vez, resolve-se direto. Na segunda, nota-se a repetição. Na terceira, abstrai-se, já sabendo o que de fato se repete. As fronteiras entre camadas ([ARCHITECTURE](ARCHITECTURE.md#fronteiras-entre-camadas)) são a exceção, porque são requisitos do produto.

## Código de referência

A IA imita o código que já existe. Por isso, os melhores exemplos de cada padrão são listados aqui conforme surgem.

| Para... | Siga |
|---|---|
| declarar uma biblioteca, um programa ou testes | `cmake/NexumModule.cmake`, com o exemplo em `Engine/Core/CMakeLists.txt` |
| ler dados externos com validação e devolver erros por valor | `Engine/Core/Private/Version.cpp` |
| testar uma leitura com dados inválidos | `Engine/Core/Tests/VersionTest.cpp` |
| esconder uma biblioteca externa atrás do módulo dono | `Engine/Platform/Private/PlatformInfo.cpp`; com objetos da biblioteca, `Engine/Render/Public/Nexum/Render/GpuDevice.hpp` e `Engine/Render/Private/GpuDevice.cpp` |
| guardar objetos que podem morrer por handle geracional, num pool que não cresce | `Engine/Core/Public/Nexum/Core/HandlePool.hpp`, com os testes em `Engine/Core/Tests/HandlePoolTest.cpp` |
| receber o tempo de quem chama, sem relógio dentro, e simular minutos em milissegundos no teste | `Engine/Time/Public/Nexum/Time/TickClock.hpp` e `Engine/Time/Tests/TickClockTest.cpp` |
| receber a saída no construtor, sem estado global | `Engine/Debug/Public/Nexum/Debug/Logger.hpp`, com o log em memória dos testes em `tests/support/Public/Nexum/TestSupport/MemoryLogSink.hpp` |
| montar um quadro sem pedir memória e provar isso | `Engine/Core/Public/Nexum/Core/FrameArena.hpp` e `Games/MuOnline/Tests/FrameAllocationTest.cpp` |
| separar o estado que muda no tick do retrato que o desenho só lê | `Games/MuOnline/Public/Mu/Foundation/FoundationScreen.hpp` |
| testar que um programa falha com a mensagem certa | `Engine/Core/Tests/CMakeLists.txt` (o teste `nexum_core_assert_failure_tests`) |
| juntar a captura e a comparação de uma cena num teste da prova visual | `tests/visual/CMakeLists.txt` |

## Testes

- O framework é o **doctest**. O nome do teste descreve o comportamento: `"the tick clock runs 25 ticks per second"`.
- Um teste que prova um critério de aceitação leva a etiqueta no começo do nome: `TEST_CASE("[0000-CA-04] ...")`.
- Tempo e sorteios nos testes vêm das fontes injetadas, nunca do sistema.
- Todo leitor de dados externos tem testes com dados inválidos.
- Decodificadores de pacote são conferidos contra os vetores gerados pelo próprio OpenMU (a partir do M2).
- Um teste nunca é apagado ou enfraquecido para "fazer passar".

## Documentação

- Curta. Aponta para outro documento em vez de repetir o que está nele.
- Diagramas Mermaid quando ajudarem:

| Tipo | Para quê |
|---|---|
| `flowchart LR` | fluxos e processos |
| `flowchart TB` | camadas e hierarquias |
| `sequenceDiagram` | quem fala com quem, e em que ordem |
| `stateDiagram-v2` | estados e transições |

- No máximo cerca de 15 elementos por diagrama, uma ideia por diagrama.
- O diagrama complementa o texto: toda regra importante também fica escrita.

## Scripts

- Ferramentas internas em Python 3, só com a biblioteca padrão, em `scripts/`.
- Hooks do Git em `sh` (rodam no Windows, pelo Git, e no Linux e no macOS).
- Hooks do Claude Code em PowerShell 5.1, salvos em UTF-8 com BOM.

## Conteúdo de terceiros

- **Sala limpa:** nada do código do MuMain, nem copiado nem adaptado. Nomes de variáveis, comentários e estruturas dele também não ([WORKFLOW](WORKFLOW.md#sala-limpa)).
- **OpenMU (MIT):** código, XML de pacotes e documentos podem ser reaproveitados, com o aviso da licença junto do arquivo.
- **Linha vermelha:** nomes do MU (mapas, classes, itens, códigos de pacote, o namespace `mu`) nunca em `Engine/` nem em `Kit/`. Os nomes dos formatos originais (`.bmd`, `.ozj`...) só em `Tools/`.
- **Nada da Webzen no repositório:** nenhum modelo, textura, som, tabela ou imagem com a arte do jogo. O conteúdo de teste é próprio e fica em `Content/`.
- Assets externos só com licença compatível (CC0, CC-BY com atribuição, MIT), registrada junto do asset.
- Bibliotecas externas ficam registradas, com a licença, em [THIRD_PARTY.md](THIRD_PARTY.md).

# Módulo: Core

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A base que todos os outros módulos usam: o assert, os erros por valor, os handles geracionais, a memória do quadro, uma imagem RGBA simples e o FNV-1a de 64 bits. Não depende de nenhum outro módulo da engine, não abre arquivo, não fala com o sistema operacional e não sabe o que é MU.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_core` (`Nexum::Core`) | Engine | biblioteca; liga só o `tl::expected` |

Cabeçalhos públicos em `Engine/Core/Public/Nexum/Core/`.

## Conceitos

- **`NEXUM_ASSERT(condição)`** (`Assert.hpp`): no build de desenvolvimento, uma condição falsa escreve `NEXUM_ASSERT failed: <condição> at <arquivo>:<linha>` e encerra o programa. No build de jogador, a condição nem é avaliada, então ela nunca pode ter efeito colateral. Serve só para erro de programação; erro de dado de fora volta por valor.
- **`Result<T>` e `Error`** (`Result.hpp`): o resultado ou o erro, com uma mensagem. Hoje é o `tl::expected` ([TD-0004](../TECH_DEBT.md)).
- **`Handle<T>` e `HandlePool<T>`** (`Handle.hpp`, `HandlePool.hpp`): o handle guarda o lugar e a geração, e o tipo do objeto faz parte dele. O pool reserva toda a memória ao ser criado e nunca cresce: cheio, ele devolve um erro. Um handle de objeto removido é recusado, mesmo com o lugar reaproveitado. Os objetos não mudam de endereço enquanto outros entram e saem. Um lugar cuja geração chegaria ao fim é aposentado.
- **`FrameArena`** (`FrameArena.hpp`): memória reservada de uma vez, entregue em pedaços alinhados e zerada a cada quadro (`Reset`). Sem espaço, devolve um erro e não muda nada. Nunca roda destrutores, por isso o `AllocateArray<T>` só aceita tipos que não precisam deles.
- **`Image` e `Rgba8`** (`Image.hpp`, `Rgba8.hpp`): largura × altura pixels, linha a linha a partir do canto de cima à esquerda. O `FromPixels` recusa uma lista com o tamanho errado. O `At(x, y)` fora da imagem é um assert.
- **`Version`** (`Version.hpp`): a versão da engine e a leitura de um texto de versão, tratado como dado de fora.
- **`Fnv1a64`** (`Fnv1a.hpp`, só no cabeçalho e `constexpr`): o FNV-1a de 64 bits (Fowler, Noll e Vo). Cada byte entra no estado com um "ou exclusivo" e uma multiplicação pelo primo. Não usa tabela nem memória, então serve para a impressão de um estado que muda a cada tick, como a da gravação ([MU: Recording](mu-recording.md)). Entram bytes (`Add`), textos sem o terminador e números (`AddLittleEndian`), estes sempre em little-endian, um byte por vez: o resultado é o mesmo em todo sistema. O `Fnv1a64Of` dá o valor de uma vez. Não é um hash criptográfico: ele acha diferenças por acidente, não as feitas de propósito.

## Como usar

O código de referência está em [CONVENTIONS](../CONVENTIONS.md#código-de-referência): handles no lugar de ponteiros e o quadro sem pedir memória. A tela da fundação (`Games/MuOnline/Private/Foundation/FoundationScreen.cpp`) monta o retrato do quadro numa `FrameArena`.

## Fontes da central

Não se aplica: o módulo não reproduz nada do original.

## Invariantes relevantes

- **INV-11:** memória com dono claro e handles. O pool e a arena são as peças que tornam isso possível.
- **INV-13:** caminho quente sem alocação. O pool e a arena reservam tudo no começo.
- **INV-14:** erros por valor (`Result`); o assert é só para erro de programação.
- **INV-12:** o `FromPixels` e o `ParseVersion` validam o que vem de fora.

## Testes

Em `Engine/Core/Tests/` (`nexum_core_tests`): `HandlePoolTest.cpp`, `FrameArenaTest.cpp`, `ImageTest.cpp`, `AssertTest.cpp`, `ResultTest.cpp`, `VersionTest.cpp` e `Fnv1aTest.cpp` (os valores de teste publicados do algoritmo no documento do IETF, o mesmo valor em partes, os números em little-endian com o seu tamanho, os negativos e o cálculo em tempo de compilação). O `ParseVersion` tem um alvo de fuzzing (`ParseVersionFuzzTest.cpp`, com as sementes em `FuzzSeeds/ParseVersion`), que roda as sementes como um teste comum nos 3 sistemas e roda de verdade na CI do Linux com Clang. O assert falso roda num programa próprio, conferido pelo script `CheckAssertFailure.cmake` (teste do CTest `nexum_core_assert_failure_tests`), porque o assert encerra o programa inteiro.

## Itens provisórios

- [TD-0004](../TECH_DEBT.md): o `Result` usa o `tl::expected` até o C++23.
- Quando um pool enche, quem o usa decide o que fazer por cima do erro. Os pools de partículas e efeitos, que descartam o mais velho, chegam no M5.

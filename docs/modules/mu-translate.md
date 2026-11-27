# Módulo: MU, Translate

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A tradução dos pacotes do mundo em eventos neutros (IV.14.4 da spec de referência). Ela lê cada pacote com a variante do dialeto 106.3 e põe os eventos numa fila, sem aplicar nada: quem aplica é a [visão do mundo](mu-world.md), dentro do tick. Ela não guarda estado do mundo.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Translate` |

Cabeçalho público em `Games/MuOnline/Public/Mu/Translate/Translator.hpp`.

## Conceitos

- **`Translator`**: nasce com o id do herói (`Session::PlayerId()`). O `Translate(época, mensagem, fila)` põe na `EventQueue` os eventos de uma mensagem, cada um com a época da mensagem.
- **Os pacotes traduzidos:** os dados do personagem (`C3 F3 03`), os jogadores que entram na visão (`C2 12`, inclusive o próprio herói), os NPCs e monstros (`C2 13`), os objetos que saem da visão (`C1 14`), o eco do andar (`C1 D4`), a posição corrigida (`C1 15`), os stats do herói (`C1 26 FF`), o inventário (`C4 F3 10`, só lido) e a mudança dos atributos do terreno (`C1 46`, só lida e registrada). A marca de "acabou de surgir" fica separada do id.
- **As reações** (IV.14.6):
  - um pacote que não se lê vai para o log e é descartado inteiro;
  - um código desconhecido é ignorado e registrado uma vez só;
  - os pacotes conhecidos que a fatia não usa (os da §6.12 e o `C1 B2 1A`) são ignorados sem aviso.
- **O log** é montado com o `LogLine` do [Debug](debug.md), sem pedir memória.

## Como usar

No passo 2 do quadro, o servidor passa cada mensagem do mundo ao `Translate` ([MU: Online](mu-online.md)): o `LiveServer`, com as mensagens que a sessão entrega, e o `ReplayServer`, com as mensagens da gravação. As duas implementações usam a mesma tradução, e por isso um erro dela aparece num replay, sem servidor. Os testes dão os bytes dos pacotes e conferem os eventos (`Games/MuOnline/Tests/TranslatorTest.cpp`).

## Fontes da central

- Os pacotes do mundo e os campos: a spec de protocolo, §6.11, §6.12, §7 e §8.
- A variante 5.0 do `C2 13` e os layouts do `C1 14` e do `D4`: o achado `2026-11-21-variantes-por-versao-m2.md`, §4.
- O layout do `C2 12`: o achado `2026-11-21-aparencia-estendida.md`, §2.
- O eco do andar e as direções: o achado `2026-11-22-direcoes-do-walkrequest.md` (só a leitura do eco nesta fatia).
- A ordem real do primeiro ingresso e o `C1 B2 1A`: o achado `2026-11-21-capturas-reais-sessao-106-3.md`, §3 e §4.

## Invariantes relevantes

- **INV-01:** os eventos são o que o servidor disse; a tradução não prevê nada.
- **INV-12:** cada pacote é conferido pela leitura gerada antes de qualquer campo.
- **INV-13:** traduzir uma mensagem não pede memória, enquanto a fila tiver espaço.

## Testes

Em `Games/MuOnline/Tests/TranslatorTest.cpp` (`muonline_tests`): cada pacote vira o evento certo, com os bytes das capturas reais e os calculados pelos layouts da central, e os casos de pacote cortado, de código desconhecido e de pacote ignorado.

## Itens provisórios

- [TD-0030](../TECH_DEBT.md): os personagens transformados (`C2 45`) e as invocações (`C2 1F`) não são traduzidos.
- [TD-0031](../TECH_DEBT.md): os itens no chão (`C2 20`) são ignorados, e os jogadores chegam só com a contagem dos efeitos.

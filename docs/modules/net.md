# Módulo: Net

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O transporte da engine: conexões TCP, cada uma numa thread própria, com uma fila de entrada e uma única fila de saída. O módulo só move bytes. Onde uma mensagem acaba, como ela é aberta quando chega e como é fechada antes de sair, quem decide é o jogo, pela transformação de bytes que ele entrega à conexão. O módulo não sabe nada do MU, não reconecta sozinho e nunca faz a thread principal esperar pela rede.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_net` (`Nexum::Net`) | Engine | biblioteca; liga o `Nexum::Core`, e o Asio por dentro |
| `nexum_net_test_server` | Engine | biblioteca só dos testes, com o servidor TCP local; nenhum programa do jogador a carrega |

Cabeçalhos públicos em `Engine/Net/Public/Nexum/Net/`. O Asio só aparece em `Engine/Net/Private/` ([ADR 0033](../adr/0033-dono-de-cada-biblioteca-externa.md)).

## Conceitos

- **`Connection`** (`Connection.hpp`): o `Open` recebe o endereço e a transformação de bytes, e começa a thread da conexão, que procura o endereço e conecta.
  - O resultado chega como um evento: `Connected` (a conexão abriu), `Message` (uma mensagem inteira, já aberta pela transformação) ou `Closed` (a conexão acabou, com o motivo).
  - O `Send` copia a mensagem para o fim da fila de saída. O `Poll` entrega, em ordem, todos os eventos que chegaram desde a última chamada, sem nunca esperar pela rede. O `Close` pede o fechamento, e um `Closed` vem depois.
  - As duas filas reservam a memória quando a conexão abre (`ConnectionOptions::queueBytes`, 256 KiB por padrão) e são trocadas sob mutex ([ADR 0006](../adr/0006-modelo-de-threads.md)). Uma mensagem maior que o espaço fecha a conexão.
  - A thread roda o Asio em passos de 5 ms e olha a fila de saída entre um passo e outro. Acordá-la pelo Asio pediria memória na thread principal a cada mensagem.
- **`MessageCodec` e `MessageSink`** (`MessageCodec.hpp`): a transformação de bytes, que o jogo implementa. O `Receive` abre os bytes lidos do socket em mensagens inteiras e as entrega em ordem. O `Send` fecha uma mensagem nos bytes que vão para o fio. As duas funções rodam na thread da conexão, então a ordem em que as mensagens são fechadas é a ordem do fio. Um erro de uma delas fecha a conexão, com o erro como motivo.
- **`LoopbackServer`** (`LoopbackServer.hpp`): um servidor TCP em 127.0.0.1, com um cliente só, conduzido passo a passo por quem o usa: aceitar, ler, escrever e fechar, cada passo com um tempo limite. Existe porque os testes dos jogos não podem abrir sockets: só o `Engine/Net` pode usar o Asio.

## Como usar

O módulo MU abre uma `Connection` para cada endereço que a sessão pede, cada uma com um `ClientCodec` novo (`Games/MuOnline/Private/Online/NetworkTransport.cpp`). A cada quadro, o passo 2 chama o `Poll` e entrega os eventos à sessão ([MU: Online](mu-online.md)).

## Fontes da central

Não se aplica: o módulo só move bytes. O que o MU faz com eles está no [MU: Protocol](mu-protocol.md).

## Invariantes relevantes

- **INV-06:** a thread principal só põe e tira das filas, sem esperar pela rede.
- **INV-09:** as mensagens saem na ordem da única fila de saída, e a transformação roda só na thread da conexão.
- **INV-10:** o Asio fica atrás deste módulo (checagem `architecture`).
- **INV-03:** o módulo não sabe o que é MU (checagem `terms`).
- **INV-13:** as filas reservam a memória ao abrir; depois, pôr e tirar mensagens não pede memória.
- **INV-14:** o Asio é usado com os códigos de erro, sem exceções, e os erros voltam por valor.

## Testes

Em `Engine/Net/Tests/ConnectionTest.cpp` (`nexum_net_tests`), contra o `LoopbackServer`: a conexão abre, as mensagens chegam na ordem e saem na ordem da fila, e um servidor fora do ar, uma conexão recusada e uma conexão fechada por qualquer um dos lados viram um `Closed` com o motivo. Na CI, os sanitizers rodam os mesmos testes.

## Itens provisórios

Nenhum neste módulo. A reconexão é da sessão do jogo ([TD-0028](../TECH_DEBT.md)).

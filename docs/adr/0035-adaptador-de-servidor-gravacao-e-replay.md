---
id: 0035
titulo: O adaptador de servidor, e a gravação e o replay no módulo MU
status: aceita
data: 2026-11-26
substitui:
substituida-por:
---

# ADR 0035: O adaptador de servidor, e a gravação e o replay no módulo MU

## Contexto

Na fatia 1, o cliente online (`OnlineClient`, em `Mu/Online`) guarda direto as peças do OpenMU 106.3: a sessão (`Mu/Session`) e a tradução das mensagens em eventos neutros (`Mu/Translate`). A fatia 2 traz a primeira outra fonte de eventos, o replay de uma gravação `.mucap`. Sem um encaixe entre o servidor e o jogo, seria preciso duplicar o cliente online ou remendá-lo. Era o que a [P-0018](../PROPOSALS.md) apontava.

A spec de referência pede três coisas que tocam nesse encaixe:

- **A época em cada evento** ([D-10](../referencia/especificacao-engine-propria-caminho-b.md#d-10--rede-adaptador-em-camadas-codec-gerado-dialeto-1063), item 5). Hoje a época só é conferida nas mensagens, antes da tradução. Quando a sessão cai, os eventos já traduzidos ainda chegam à visão do mundo no tick seguinte (o TD-0037, que esta decisão resolveu na fatia 2).
- **A gravação e o replay como ferramenta de desenvolvimento** ([III.8](../referencia/especificacao-engine-propria-caminho-b.md#iii8-modos-de-operação-runtime-e-authoring), [IV.18](../referencia/especificacao-engine-propria-caminho-b.md#iv18-diagnóstico-e-ferramentas-de-desenvolvimento) e [V.2.2](../referencia/especificacao-engine-propria-caminho-b.md#v22-gravação-e-replay)). No modo de autoria, o `MuClient` pode estar "reproduzindo uma gravação".
- **Um lugar para essa ferramenta.** A árvore da [III.2](../referencia/especificacao-engine-propria-caminho-b.md#iii2-repositório-e-módulos) põe a gravação e o replay em `Tools/capture-replay/`. Mas é o próprio `MuClient` que grava e reproduz, e o jogo não pode depender de `Tools/` (INV-02).

O dono aprovou esta decisão junto com a spec da [fatia 2](../../specs/0002-gravacao-e-replay/spec.md) (decisões 1 e 3), em 2026-11-26.

## Decisão

1. **O cliente online fala com o servidor só por um contrato,** o adaptador de servidor (`Mu/Online/ServerAdapter.hpp`). O contrato fica no módulo MU, e não na engine, porque os eventos neutros ainda falam de MU (INV-03). Ele tem estas partes:
   - `Start(now)`: o começo da sessão;
   - `TakeEvents(now, events)`: o passo 2 do quadro. Põe na fila os eventos traduzidos desde o quadro anterior, cada um com a época, sem nunca esperar pela rede ou pelo disco (INV-06);
   - `AdvanceClock(now)`: o passo 3 do quadro, com os relógios de parede (os prazos e o ping);
   - `EventsApplied(view, count, now)`: o aviso, depois de cada tick que tirou eventos da fila, de quantos eram e de como ficou a visão do mundo;
   - `CurrentEpoch()`, `Status()` e `Summary(now)`: a época atual, o estado e uma linha sobre a sessão.

   O estado é um de quatro: conectando, no mundo, terminou ou falhou. Quando falha, o estado leva o motivo. O contrato não sabe de bytes, de conexões nem de arquivos.

2. **Há duas implementações.** O `LiveServer` é o OpenMU 106.3 ao vivo e junta as peças de hoje: o transporte, a sessão e a tradução. O `ReplayServer` lê uma gravação `.mucap` e entrega as mesmas mensagens à mesma tradução. A conta e a senha entram só no `LiveServer`, quando ele nasce. O contrato nunca as vê, e o replay nunca recebe uma senha. Isso é um pequeno desvio da P-0018, que punha as credenciais no começo do contrato.

3. **O cliente online fica só com o que é do jogo:** a fila de eventos, a visão do mundo e o tempo pedido no mundo. Ele não sabe de onde vêm os eventos.

4. **Cada evento leva a época** (D-10, item 5). A tradução recebe a época da mensagem, e a fila guarda cada evento com ela. No tick, o cliente online descarta e conta cada evento de uma época mais velha que a `CurrentEpoch()` do adaptador, antes de ele chegar à visão do mundo. A visão do mundo não sabe de época.

5. **Os comandos do jogador** (andar, atacar) entram no contrato com o andar, na parte 2 do M2. Eles serão valores de tamanho fixo, como os eventos, para o encaixe não pedir memória a cada quadro (INV-13). Até lá, a V.2.2, que pede os comandos na gravação, fica cumprida pela metade. Isso é um item provisório ([TD-0038](../TECH_DEBT.md)): o `.mucap` versão 2 traz os comandos.

6. **A gravação e o replay ficam no módulo MU, e não em `Tools/`.** O formato `.mucap`, a impressão do estado do mundo e o gravador ficam em `Mu/Recording`. O replay fica em `Mu/Online/ReplayServer`, por ser uma implementação do contrato. Isso é um desvio da árvore da III.2: o `MuClient` grava e reproduz, e o jogo não pode depender de `Tools/` (INV-02). Um programa à parte, `Tools/CaptureReplay`, só nasce quando houver uma necessidade dele, e usaria as mesmas peças do módulo.

7. **A gravação e o replay só existem no build de desenvolvimento.** As opções `--record`, `--server-version` e `--replay` só são ligadas ao `MuClient` com `NEXUM_AUTHORING`. O build de jogador recusa as três, como já recusa o `--capture` (III.8 e INV-19).

## Consequências

- **Melhor:** o replay passa pelas mesmas peças da sessão ao vivo a partir da tradução. Um erro na tradução dos pacotes aparece num teste, sem servidor, que é o nível "Replay" da [V.1](../referencia/especificacao-engine-propria-caminho-b.md#v1-pirâmide-de-testes).
- **Melhor:** a troca de mapa (M4) e a reconexão (M7) só precisam mudar a época dentro do `LiveServer`. O tick já descarta os eventos velhos, e a gravação já guarda as mudanças de época que acontecem no passo 2 do quadro, as únicas de hoje. Uma mudança em outro passo pede que o gravador saiba da época também antes de cada tick ([TD-0040](../TECH_DEBT.md)).
- **Melhor:** um servidor diferente do OpenMU seria só outra implementação do contrato. Isso continua fora do produto ([VISÃO](../VISION.md)), mas a porta fica aberta.
- **Pior:** há uma camada a mais entre o cliente online e a sessão. Os testes do cliente online da fatia 1 passam a montar o `LiveServer`, sem perder nenhuma conferência.
- **Pior:** o código de `Mu/Recording` e do `ReplayServer` é compilado no módulo MU também no build de jogador. Só a ligação com o `MuClient` fica fora dele, e nada no build de jogador chama esse código.
- **Exigido:** um teste com um adaptador de teste prova que o cliente online aplica os eventos em ordem sem saber de onde vêm. Um teste muda a época entre a tradução e o tick e confere que os eventos velhos não chegam à visão; com a conferência desligada de propósito, ele falha.
- **Exigido:** o gravador nunca vê o que o cliente envia e começa só na entrada no mundo, para a conta e a senha nunca chegarem ao arquivo (INV-18). Um teste confere isso.
- **Exigido:** o build de jogador recusa as três opções novas, com um teste.

## Alternativas consideradas

- **Um segundo cliente online, só para o replay:** duplicaria a fila, o tick e a visão do mundo. As duas cópias poderiam ficar diferentes, e esconder justamente o erro que o replay quer achar.
- **O replay por baixo da sessão, como um transporte que entrega bytes:** a gravação teria de guardar o que vem antes do mundo, inclusive o login com a conta, e o replay precisaria fingir ser o servidor durante o login.
- **A conta e a senha no começo do contrato,** como dizia a P-0018: o replay receberia uma senha que não usa, e cada implementação nova também.
- **A gravação e o replay em `Tools/CaptureReplay`,** como na III.2: o `MuClient` passaria a depender de `Tools/`, o que a INV-02 proíbe.
- **A época só nas mensagens,** como hoje: os eventos já traduzidos de uma conexão que caiu continuariam chegando à visão do mundo no tick seguinte.

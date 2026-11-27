---
id: 0002
titulo: A gravação e o replay
status: aprovada
aprovada-em: 2026-11-26
marco: M2
---

# Fatia 2: A gravação e o replay

## Resumo para o dono

No fim desta fatia, o `MuClient` grava a sessão no mundo num arquivo `.mucap` e depois a reproduz sem nenhum servidor ligado. O replay passa pelas mesmas peças da sessão ao vivo e confere, de tempos em tempos, se o mundo ficou igual ao que foi gravado. No fim, o terminal diz se tudo bateu. Uma gravação curta de um OpenMU de verdade fica no repositório, e a CI a reproduz nos 3 sistemas a cada mudança. Assim, um erro na tradução dos pacotes aparece sem precisar de servidor.

Esta fatia fecha a parte 1 do M2 ([ADR 0032](../../docs/adr/0032-rede-antes-do-importador.md)). Ela traz também o encaixe único entre o servidor e o jogo (P-0018), a época dentro dos eventos ([TD-0037](../../docs/TECH_DEBT.md)) e a conferência dos `NOLINT` novos (P-0015). A janela continua com o cartão de teste da fatia 0.

## Decisões desta fatia

Estes pontos pedem o seu "sim" junto com a spec. Cada um traz a opção recomendada.

1. **O adaptador de servidor ([P-0018](../../docs/PROPOSALS.md)).** O cliente online passa a falar com o servidor só por um contrato, com duas implementações: o OpenMU ao vivo (a sessão, a conexão e a tradução de hoje) e o replay. A conta e a senha entram só na implementação ao vivo, quando ela nasce, e não no contrato: o replay nunca vê uma senha. Isso é um pequeno desvio do texto da P-0018, que punha as credenciais no começo do contrato. Os comandos do jogador (andar, atacar) entram no contrato com o andar, na parte 2 do M2 (item provisório). A decisão vira o ADR 0035.
2. **O formato `.mucap`, versão 1.** É um formato novo da engine, e por isso pede o seu "sim". A gravação guarda, a partir da entrada no mundo:
   - as mensagens do mundo que chegam do servidor, já decifradas, com o tempo e a época;
   - as mudanças de época;
   - uma marca de cada tick que aplicou eventos, com quantos eram;
   - os pontos de controle, um a cada 5 segundos e outro no fim;
   - o fim, com o jeito como a sessão acabou.

   O cabeçalho tem a versão do formato, a do Nexum, a do dialeto (`20404`) e um texto sobre o servidor. O que o cliente envia nunca entra, e nada de antes do mundo também não: a lista de servidores, o login e a lista de personagens ficam de fora. Assim, a conta e a senha nunca chegam ao arquivo (D-17 e INV-18). A V.2.2 da spec de referência fala também dos comandos do jogador; eles entram numa versão 2, com o andar.
3. **Só no build de desenvolvimento, dentro do módulo MU.** A gravação e o replay são ferramentas de desenvolvimento (III.8 e IV.18 da spec de referência). O build de jogador recusa as opções novas, como já recusa o `--capture`. Elas ficam no módulo MU, e não em `Tools/CaptureReplay`, como sugere a árvore da III.2: o `MuClient` precisa gravar e reproduzir, e o jogo não pode depender de `Tools/` (INV-02). O ADR 0035 registra esse desvio.
4. **A gravação fica na memória até o fim.** O `MuClient` reserva 16 MiB quando a gravação começa e escreve o arquivo só quando termina, fora do laço dos quadros. Assim, o quadro nunca espera pelo disco (INV-06). Se o espaço encher, a gravação para com um aviso, e o arquivo continua válido. Se o programa travar, a gravação se perde. A escrita durante a sessão fica como item provisório.
5. **A gravação de referência no repositório.** É uma sessão de cerca de 20 segundos no OpenMU 0.10.1 local: a conta `test0` no mundo, e a `test1` entrando na visão e saindo. Eu gravo com o servidor de teste da fatia 1, cuja imagem já está no seu computador. O conteúdo vem do OpenMU (licença MIT): os números de itens e de NPCs e os nomes dos personagens de teste. Não há nada da Webzen, nem conta, nem senha.
6. **A trava dos `NOLINT` ([P-0015](../../docs/PROPOSALS.md)).** Ela muda a trava `scripts/checks/`. A checagem `cpp-rules` passa a exigir que cada `NOLINT` diga qual regra desliga e tenha o motivo escrito na linha de cima. O resumo de segurança do PR passa a listar cada `NOLINT` novo, inclusive nos arquivos de `Tools/`. Isso é necessário porque hoje um `NOLINT` novo desliga uma conferência em silêncio: na fatia 1, um deles entrou em todos os cabeçalhos gerados, e só a revisão viu.

## Escopo

- **O adaptador de servidor** (`Mu/Online`): o contrato, a implementação do OpenMU ao vivo com as peças de hoje, e o cliente online usando só o contrato.
- **A época nos eventos** ([TD-0037](../../docs/TECH_DEBT.md)): cada evento sai da tradução com a época da mensagem, e o tick descarta os de uma época velha.
- **O formato `.mucap`** (`Mu/Recording`, área nova): a escrita, a leitura com todas as conferências e a impressão do estado do mundo.
- **A gravação:** a opção `--record` do `MuClient`.
- **O replay:** a opção `--replay` do `MuClient` e a implementação do adaptador que lê a gravação.
- **Peças da engine, sem nada do MU:** ler um arquivo inteiro com limite de tamanho e escrever um arquivo inteiro (`Engine/Platform`); o FNV-1a de 64 bits, para a impressão do estado (`Engine/Core`).
- **Fuzzing:** um alvo para a leitura da gravação, que também faz o replay das gravações válidas.
- **A gravação de referência** e os testes do replay nos 3 sistemas.
- **Integração:** um teste que grava e reproduz contra o OpenMU local.
- **Os `NOLINT`** ([P-0015](../../docs/PROPOSALS.md)): a checagem `cpp-rules` e o resumo de segurança do PR.
- **Documentação:** a ARCHITECTURE, as páginas dos módulos (com o formato `.mucap` byte a byte), o ADR 0035, o guia de desenvolvimento e os itens provisórios.

## Fora do escopo

- Os comandos do jogador, no adaptador e na gravação: parte 2 do M2, com o andar.
- A pausa, a velocidade e o avanço tick a tick do replay, e o inspetor de pacotes: com o ImGui, depois do M1 (IV.18 da spec de referência).
- Gravar o que vem antes do mundo (a lista de servidores, o login e os personagens): fica de fora de propósito, pela segurança (decisão 2).
- A escrita da gravação durante a sessão e as gravações longas: item provisório.
- O programa `Tools/CaptureReplay`: só quando houver uma necessidade dele (decisão 3).
- Apoiar outros servidores: fora, como diz a [P-0018](../../docs/PROPOSALS.md).
- A visão 2D da grade e o andar: parte 2 do M2, depois do M1.

## Critérios de aceitação

- CA-01: O cliente online (`OnlineClient`) só fala com o servidor por um contrato único, o adaptador de servidor. O contrato tem o começo, a entrega dos eventos do quadro com a época, os relógios de parede, o aviso de que o tick aplicou os eventos, o estado e o resumo da sessão. O estado é um de quatro: conectando, no mundo, terminou ou falhou, este com o motivo. O OpenMU ao vivo e o replay são as duas implementações, e a conta e a senha só entram na implementação ao vivo. Os testes do cliente online da fatia 1 continuam passando, sem perder nenhuma conferência. Um teste com um adaptador de teste mostra que o cliente aplica os eventos em ordem sem saber de onde eles vêm.
- CA-02: Cada evento neutro sai da tradução com a época da mensagem. No tick, um evento de uma época mais velha que a atual do adaptador é descartado e contado, sem chegar à visão do mundo, e o log registra o descarte uma vez por tick. Um teste muda a época entre a tradução e o tick e confere que os eventos velhos não chegam. Com a conferência desligada de propósito, esse teste falha.
- CA-03: A gravação `.mucap` versão 1 tem o cabeçalho e os registros descritos no plano: a entrada no mundo, as mensagens, as mudanças de época, as marcas de tick, os pontos de controle e o fim. Um teste fixa os bytes de uma gravação pequena, que é escrita e lida de novo. A leitura recusa cada caso inválido, com o motivo exato:
  - a marca errada, uma versão desconhecida, ou um texto do cabeçalho longo demais;
  - um registro cortado, de tipo desconhecido ou com o tamanho acima do limite;
  - o tempo voltando, ou uma mensagem antes da entrada no mundo;
  - um ponto de controle que não vem logo depois de uma marca de tick, ou que conta errado as mensagens;
  - qualquer coisa depois do fim, ou a falta do fim;
  - um arquivo acima de 64 MiB, recusado antes de o conteúdo ser lido.
- CA-04: Com `--record`, o `MuClient` grava desde a entrada no mundo. A gravação tem:
  - cada mensagem do mundo que a sessão entrega à tradução, com o tempo do quadro em microssegundos e a época;
  - a marca de cada tick que aplicou eventos;
  - um ponto de controle a cada 5 segundos no mundo e outro no fim.

  Nada do que o cliente envia entra na gravação. Um teste com uma conta e uma senha conhecidas confere que nenhuma das duas aparece no arquivo. A gravação usa um espaço reservado de 16 MiB. Quando ele enche, a gravação para com um aviso no log, e o arquivo continua válido, com o fim marcado como "cheio". O arquivo é escrito quando o cliente termina, fora do laço dos quadros. Se não der para escrever no caminho pedido, o cliente recusa antes de conectar, com um erro claro.
- CA-05: Com `--replay`, o `MuClient` roda sem servidor. O replay faz isto:
  - lê e confere a gravação inteira antes da janela abrir;
  - entrega as mensagens à tradução no ritmo em que foram gravadas, com o mesmo agrupamento por tick da sessão gravada;
  - confere a quantidade de eventos de cada tick e cada ponto de controle, depois do tick que aplicou os eventos até ali.

  Uma diferença encerra com erro. A mensagem diz em qual tick ou ponto de controle ela aconteceu e mostra o esperado e o obtido. Sem diferença, a última linha diz quantas mensagens, ticks e pontos de controle foram conferidos, e o programa encerra com sucesso. Dois replays da mesma gravação dão o mesmo resultado. Um teste com um relógio de teste reproduz uma gravação em milissegundos. Outros testes, com gravações montadas, cobrem um quadro sem tick, um quadro com vários ticks e um tick sem eventos.
- CA-06: A impressão do estado da visão do mundo, versão 1, cobre o herói, cada objeto na visão, o inventário e as duas contagens: as mudanças do terreno e os eventos ignorados. Ela usa o FNV-1a de 64 bits sobre os campos, com os números em little-endian. A ordem dos objetos e do inventário na tabela não muda a impressão, e mudar qualquer campo muda. O FNV-1a passa nos valores de teste publicados do algoritmo.
- CA-07: Uma gravação curta do OpenMU 0.10.1 local fica no repositório, sem conta e sem senha. São cerca de 20 segundos no mundo com a conta `test0`, e a `test1` entra na visão e sai. O replay dela passa em todos os ticks e pontos de controle, nos 3 sistemas, de dois jeitos: num teste da biblioteca, sem relógio de parede, e num teste do `MuClient --replay`, no ritmo gravado.
- CA-08: As opções novas do `MuClient` seguem estas regras:
  - `--record ARQUIVO` vem com `--connect`;
  - `--server-version TEXTO`, com até 64 bytes, vem só com `--record`;
  - `--replay ARQUIVO` não vem com as opções do caminho até o mundo, nem com `--record` ou `--capture`.

  O build de jogador recusa as três, como recusa o `--capture`, e o código que as liga ao `MuClient` só é compilado no build de desenvolvimento. Uma gravação inválida é recusada antes da janela abrir, com o motivo da leitura.
- CA-09: A leitura da gravação tem um alvo de fuzzing, que também faz o replay inteiro de cada gravação válida. As sementes, inclusive a gravação de referência, rodam nos testes normais dos 3 sistemas. A CI roda o alvo por 60 segundos no Linux com Clang, sem nenhuma falha.
- CA-10: Depois do aquecimento, 1.000 quadros gravando e 1.000 quadros reproduzindo uma gravação, com mensagens em cada quadro, não pedem nenhuma memória ao sistema na thread principal. Isso vale também para os quadros com ponto de controle.
- CA-11: A checagem `cpp-rules` recusa um `NOLINT`, `NOLINTNEXTLINE` ou `NOLINTBEGIN` que não diga entre parênteses qual regra desliga, ou que não tenha um comentário com o motivo na linha de cima. O resumo de segurança do PR lista cada linha nova com `NOLINT`, com o arquivo e o número da linha. Isso vale para qualquer arquivo fora dos documentos, inclusive os de `Tools/`. Os testes das checagens cobrem os dois casos, e os três `NOLINT` que já existem passam.
- CA-12: No seu computador, com o OpenMU 0.10.1 local, o `MuClient` grava uma sessão de 60 segundos no mundo. Com o servidor desligado, o replay dessa gravação passa em todos os ticks e pontos de controle, e o terminal mostra as mesmas entradas na visão. Um teste de integração faz o mesmo com 20 segundos e roda sozinho quando o endereço do servidor está configurado. É uma verificação manual, registrada no report.
- CA-13: Estes documentos ficam atualizados:
  - a [ARCHITECTURE](../../docs/ARCHITECTURE.md);
  - as páginas dos módulos: a nova `mu-recording`, com o formato `.mucap` byte a byte, e a `mu-online`, a `mu-world`, a `platform` e a `core`;
  - o ADR 0035;
  - o guia de desenvolvimento, com o jeito de gravar e de reproduzir;
  - o [TECH_DEBT](../../docs/TECH_DEBT.md): o TD-0037 sai, e cada simplificação nova entra com destino.

## Paridade

A fatia não mede um nível novo de paridade. O replay é a ferramenta do nível P1 (comportamento), e aqui ele serve de teste de regressão do P0 (protocolo). Ele confere que o Nexum chega ao mesmo estado com as mesmas mensagens de um OpenMU de verdade. Ainda não confere que esse estado é igual ao do cliente original. O P0 do M2 inteiro só fica atingido no fim da parte 2, com o andar.

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0, e a prova visual dele continua valendo.

## Como o dono testa

No Windows, com o Docker Desktop aberto e um terminal na pasta do Nexum:

1. Compile o build de desenvolvimento:

   ```powershell
   py scripts/dev.py build
   ```

2. Suba o OpenMU de teste. É a mesma imagem da fatia 1, então o Docker não baixa nada. Se o servidor do laboratório da central estiver ligado, desligue-o antes, porque os dois usam as mesmas portas. Espere cerca de 1 minuto até o servidor ficar pronto:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml up -d
   ```

3. Grave 60 segundos no mundo. Quando o terminal pedir a senha, digite `test0` (ela não aparece) e tecle Enter:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0 --world-seconds 60 --record build\minha-sessao.mucap --server-version "OpenMU 0.10.1"
   ```

4. Opcional: durante a gravação, abra outro terminal e entre por uns 20 segundos com a conta `test1` (senha `test1`). Na gravação do primeiro terminal, o outro jogador entra na visão e sai:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test1 --world-seconds 20
   ```

5. Quando a gravação acabar, confira a última linha do primeiro terminal: o tempo no mundo, os pings e quantas mensagens e pontos de controle foram gravados no arquivo.
6. Desligue o servidor. Daqui em diante, nada conversa com o OpenMU:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml down
   ```

7. Reproduza a sua gravação. O terminal mostra as mesmas entradas na visão, no mesmo ritmo, e a última linha diz que todos os ticks e pontos de controle bateram:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay build\minha-sessao.mucap
   ```

8. Confira que o programa terminou com sucesso. O número que aparece é 0:

   ```powershell
   $LASTEXITCODE
   ```

9. Reproduza a gravação de referência, a mesma que a CI usa:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay Games\MuOnline\Tests\Recordings\openmu-two-players.mucap
   ```

10. Teste um arquivo que não é uma gravação. O terminal mostra o motivo da recusa, e o programa encerra com erro, sem abrir a janela:

    ```powershell
    .\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay README.md
    ```

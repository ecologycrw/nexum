---
id: 0001
titulo: A rede até o mundo
status: concluida
aprovada-em: 2026-11-25
marco: M2
---

# Fatia 1: A rede até o mundo

## Resumo para o dono

No fim desta fatia, o `MuClient` conversa com um OpenMU de verdade, sem nenhuma modificação, rodando no seu computador. Ele pega a lista de servidores, faz o login com uma conta de teste, escolhe o personagem e entra no mundo. Lá ele fica, sem cair, mandando o sinal de presença (o ping) a cada 20 segundos. Tudo o que acontece aparece no terminal: quem entra e quem sai da visão, quem anda e a vida do herói.

Ainda não há imagem do mundo: a janela continua com o cartão de teste da fatia 0. Esta é a primeira de duas fatias da parte 1 do M2. A gravação da sessão (`.mucap`) e o replay ficam para a fatia 2, que fecha a parte 1.

## Decisões desta fatia

A conferência das specs da central levantou estes pontos. O dono aprovou a opção recomendada de cada um junto com a spec, em 2026-11-25.

1. **A parte 1 do M2 vira duas fatias.** Esta leva a conversa com o servidor até o mundo. A fatia 2 traz a gravação `.mucap` e o replay, usando as sessões que esta já faz. Assim, cada fatia termina em algo que roda e se mede, e esta já tem uma prova real: o login num OpenMU sem modificações.
2. **O servidor de teste é a imagem `munique/openmu:0.10.1`, fixada pela impressão digital** `sha256:9df9ec309e6ae3dbeab1695f9923e6890c847db7ec8d4deb888927921599d3ab`. É a imagem publicada mais próxima da versão que a central estudou, e nenhum pacote desta fatia muda entre as duas. Os avisos de posição que só essa imagem dá aparecem ao andar, e o andar fica para a parte 2.
3. **Das capturas reais, entram no repositório só os vetores que já estão escritos nos documentos da central,** cada um com a fonte. As duas sessões completas e o arquivo de 48 vetores do laboratório ficam fora: elas têm o login da conta de teste e o número de série do MuMain. Além disso, a pasta do laboratório guarda uma cópia do código do MuMain, que o Nexum nunca abre. A fatia 2 grava as sessões do próprio Nexum, sem credenciais, e as curtas entram como referência do replay.
4. **O número de série do login é próprio: `NEXUM-CLIENT-001`** (16 caracteres). O servidor não confere esse número. Ele fica junto da versão `20404` numa configuração do dialeto, num lugar só do código.
5. **O [TD-0015](../../docs/TECH_DEBT.md) entra nesta fatia** (item 2 da [P-0014](../../docs/PROPOSALS.md)). Hoje, com a janela minimizada, o `MuClient` gasta 92% de um núcleo do processador, e esta fatia pede 30 minutos no mundo, quase sempre com a janela minimizada.

## Escopo

- **Codec gerado** (`Tools/PacketGen`, ferramenta nova): o `packetgen` lê os XML de pacotes do OpenMU (MIT) e gera o código que lê e escreve todos os pacotes, nas duas direções. Uma tabela do dialeto 106.3, escrita à mão, diz qual variante de cada pacote o Nexum usa.
- **Transformação de bytes**, na thread de rede: o enquadramento (`C1` a `C4`), o SimpleModulus com o contador de cada direção, o Xor32 em tudo o que o cliente envia e o Xor3 nos campos do login.
- **Estruturas escritas à mão:** o item estendido (de 5 a 15 bytes) e a aparência estendida (27 bytes), cada uma convertida num componente neutro.
- **Transporte** (`Engine/Net`, módulo novo): conexão TCP com o Asio, numa thread por conexão, com uma fila de entrada e uma única fila de saída. A engine só vê bytes.
- **Sessão** (módulo MU): do Connect Server ao Game Server, o login, a lista de personagens, a seleção, o carregamento e "No mundo". Cada resposta esperada tem um prazo. A época aumenta a cada conexão, e o que chega durante o carregamento fica guardado até o herói entrar no mundo.
- **Ping** a cada 20 segundos, pelo relógio de parede.
- **Tradução para eventos neutros e a visão do mundo:** o herói e os jogadores, NPCs e monstros que entram, saem, começam a andar ou têm a posição corrigida; os stats do herói; o inventário, só lido; a mudança dos atributos do terreno, só lida e registrada.
- **Opções do `MuClient`:** conectar a um Connect Server, a conta, a senha digitada no terminal sem aparecer, o servidor e o personagem (o primeiro da lista, se nada for dito), e fechar sozinho depois de N segundos no mundo. O log no terminal conta o que acontece.
- **Fuzzing:** a estrutura e um alvo para cada leitor novo e para os dois leitores da fatia 0 ([TD-0023](../../docs/TECH_DEBT.md)). O `LoadPng` passa a conferir o tamanho da imagem pelo cabeçalho ([TD-0020](../../docs/TECH_DEBT.md)).
- **A janela minimizada** ([TD-0015](../../docs/TECH_DEBT.md)): o laço espera pelo próximo tick em vez de girar sem pausa.
- **Ambiente de teste:** o OpenMU 0.10.1, fixado pela impressão digital, num arquivo do Docker no repositório, e um teste de integração que roda quando o servidor está no ar.
- **Documentação:** a ARCHITECTURE, as páginas dos módulos novos, o THIRD_PARTY, um ADR para o `packetgen` e os itens provisórios.

## Fora do escopo

- A gravação `.mucap` e o replay: fatia 2, que fecha a parte 1 do M2.
- O andar (o `WalkRequest` com o A\*), a visão 2D da grade e os atributos do `.att`: parte 2 do M2, depois do M1 ([ADR 0032](../../docs/adr/0032-rede-antes-do-importador.md)). O codec já gera o `WalkRequest`, mas o cliente ainda não o envia.
- A troca de mapa e o aviso de pronto (`C1 F3 12`): M4.
- A reconexão automática e a nova tentativa depois de "conta já conectada": M7.
- A tela de login e a escolha do servidor e do personagem pela interface: M6.
- O combate, os itens no chão e o chat: M5 e M12.
- O programa em C# que gera vetores com a biblioteca do OpenMU (V.2.1 da spec de referência): esta fatia usa os vetores que a central já tirou dos testes do OpenMU e das capturas reais.
- O teste de integração na CI: nesta fatia, ele roda só no computador do dono.
- A 1 hora de fuzzing por alvo: antes de fechar o M2, na parte 2 (V.3 da spec de referência).
- O inspetor de pacotes, que usa o ImGui: depois do M1.

## Critérios de aceitação

- CA-01: O leitor de mensagens separa mensagens coladas no mesmo pedaço e junta mensagens partidas, mesmo quando os bytes chegam um por vez, nos quatro tipos (`C1` a `C4`). Um primeiro byte desconhecido, um tamanho 0 ou um tamanho menor que o cabeçalho fecham a conexão com um erro que diz o motivo.
- CA-02: Todos os vetores da cifra da central passam: o bloco de 8 bytes que vira 11; o ping do cliente com os contadores 0, 1, 2, 3, 254, 255 e de novo 0; os pacotes do servidor em `C3` e `C4` com os contadores 0, 1, 2 e 255; os `C1` do cliente, só com o Xor32; os vetores das capturas reais (contadores 1, 7, 10 e 26 do cliente e 7 do servidor); e o Xor3 do usuário e da senha. Um `C1` ou um `C2` no meio não gasta contador. Um contador errado, um checksum errado, um bloco que diz ter mais de 8 bytes ou um tamanho que não é múltiplo de 11 fecham a conexão, sem tentar ressincronizar.
- CA-03: O `packetgen` gera o código de leitura e de escrita de todos os pacotes dos XML do OpenMU, com a ordem dos bytes e a posição de bits de cada campo como o XML define. Cada pacote gerado tem um teste dos bytes fixos (tipo, código e subcódigo) e do tamanho mínimo, e um pacote menor que esse mínimo é recusado sem nenhuma leitura além do fim. A tabela do dialeto 106.3 tem um teste para cada linha da tabela de variantes da spec de protocolo da central (§5.2).
- CA-04: O decodificador do item estendido passa nas 32 combinações de opções da central, nas outras 224 (que só mudam o byte 4) e nos casos de borda, e calcula o tamanho de cada item pelas opções. O da aparência passa nos exemplos da central, lê o número das asas com o deslocamento de 8 bits e lê a marca de GM. Com dados cortados, os dois leem um byte que falta como 0, como a spec descreve, e nunca leem além do fim.
- CA-05: Os pacotes de exemplo da central são lidos com os valores certos e, escritos de novo, dão os mesmos bytes: o Hello, o pedido e a resposta da lista de servidores, o pedido e a resposta do endereço do Game Server, o `GameServerEntered`, o `LoginResponse`, o pedido da lista de personagens, a seleção, o ping e o começo do `C2 12` do próprio jogador; os andares V1 a V6 da central e o eco do andar da captura. O login sai igual ao da captura real, a não ser pelo número de série, que é o próprio. O report mostra quantos vetores passam em cada grupo (CA-02 a CA-05), e o resultado é 100%.
- CA-06: Nos testes do `Engine/Net`, com um servidor de teste local, a conexão abre, as mensagens chegam na ordem em que foram mandadas e saem na ordem da fila única. Um servidor fora do ar, uma conexão recusada e uma conexão fechada por qualquer um dos lados viram um aviso claro na fila de entrada. A thread principal só põe e tira mensagens das filas, sem nunca esperar pela rede. O Asio aparece só no `Engine/Net` (checagem `architecture`), e o `Engine/Net` não sabe nada do MU (checagem `terms`).
- CA-07: A sessão, alimentada com mensagens de teste e sem rede, vai de "Desconectado" até "No mundo" na ordem da spec de protocolo (§6): a lista de servidores, a escolha, o endereço do Game Server, o fechamento do Connect Server, o `GameServerEntered`, o login, a lista de personagens, a seleção e os dados do personagem. O que chega durante o carregamento fica guardado e é aplicado, na ordem, quando o herói entra no mundo. Uma recusa de login mostra o significado do código e encerra com erro. Cada resposta esperada tem um prazo de 10 segundos, e um prazo vencido dá um erro que diz o que faltou. Dados do personagem com menos de 92 bytes dão erro, em vez de travar. A época aumenta a cada conexão, e um evento de uma época velha é descartado.
- CA-08: Com um relógio de teste, o ping sai a cada 20 segundos de relógio de parede, sem depender dos quadros nem dos ticks, a partir da entrada no Game Server. Ele leva o tempo em ms e as velocidades de ataque e de magia do último estado conhecido (0 e 0 antes de o personagem entrar), e passa pela mesma fila de saída dos outros pacotes.
- CA-09: Cada pacote do mundo da spec de protocolo (§8) vira um evento neutro, aplicado um a um, na ordem de chegada, dentro do tick. Entram na visão os jogadores (`C2 12`, inclusive o próprio herói) e os NPCs e monstros (`C2 13`); objetos saem da visão (`C1 14`), começam a andar (o eco `C1 D4`, com a origem e o destino) e têm a posição corrigida (`C1 15`). Os stats do herói mudam (`C1 26 FF`), o inventário é lido (`C4 F3 10`) e a mudança dos atributos do terreno (`C1 46`) é lida e registrada. A marca de "acabou de surgir" é separada do id. Um id desconhecido é ignorado e registrado. Um código desconhecido é ignorado e registrado uma vez só, e os códigos conhecidos que a fatia não usa (os da §6.12 e o `C1 B2 1A`) não geram aviso. Os testes da visão do mundo usam só eventos, sem rede (INV-01).
- CA-10: Cada leitor e decodificador novo (o enquadramento, a cifra, o item, a aparência e a leitura dos pacotes do dialeto) e os dois leitores da fatia 0 (o `ParseVersion` e o `LoadPng`) têm um alvo de fuzzing. As sementes de cada alvo rodam nos testes normais dos 3 sistemas, e a CI roda cada alvo por 60 segundos, sem nenhuma falha, no Linux com Clang. O `LoadPng` recusa pelo cabeçalho uma imagem grande demais, antes de ler os pixels.
- CA-11: Depois de 3 quadros de aquecimento, 1.000 quadros completos com mensagens de rede chegando a cada quadro, por um transporte de teste, não pedem nenhuma memória ao sistema na thread principal.
- CA-12: A senha é digitada no terminal sem aparecer, e nunca vai para o disco, para a linha de comando nem para o log. O log de pacotes mascara o usuário e a senha. O login manda a versão `20404` e o número de série `NEXUM-CLIENT-001`, que ficam juntos numa configuração do dialeto, num lugar só do código.
- CA-13: Com a janela minimizada, o laço do `MuClient` espera pelo próximo tick em vez de girar sem pausa, e os ticks, o ping e a rede continuam no ritmo. Um teste com um relógio de teste confere a espera. No computador do dono, no mundo e com a janela minimizada, o `MuClient` gasta menos de 10% de um núcleo do processador; antes desta fatia, eram 92%. Medição manual, registrada no report.
- CA-14: No computador do dono, com o OpenMU 0.10.1 fixado pela impressão digital e iniciado pelo arquivo do Docker do repositório, o `MuClient` entra com a conta `test0`, escolhe o primeiro personagem e fica 30 minutos no mundo, com a janela minimizada, sem queda. O log mostra a entrada no mundo, os objetos na visão e cerca de 90 pings, e o log do servidor não mostra nenhum erro causado pelo cliente. Uma senha errada mostra "senha inválida" e encerra. Um teste de integração mais curto, com 3 minutos no mundo, roda sozinho quando o endereço do servidor está configurado. Verificação manual, registrada no report.
- CA-15: A [ARCHITECTURE](../../docs/ARCHITECTURE.md), as páginas dos módulos novos em [docs/modules](../../docs/modules/README.md), o [THIRD_PARTY](../../docs/THIRD_PARTY.md) (o Asio e os XML do OpenMU, com as licenças), o ADR do `packetgen` e o [TECH_DEBT](../../docs/TECH_DEBT.md) ficam atualizados: o TD-0015, o TD-0020 e o TD-0023 saem, e cada simplificação nova entra com destino.

## Paridade

A fatia toca o nível **P0 (protocolo)**:

- os vetores (CA-02 a CA-05) medem a cifra, os formatos e as estruturas contra os vetores da central, que vêm dos testes do próprio OpenMU e de capturas reais;
- a sessão real (CA-14) mede a conversa inteira contra um OpenMU sem modificações.

O P0 do M2 inteiro só fica atingido no fim da parte 2, com o andar. Os níveis P1 a P3 não se aplicam: nenhum comportamento visível do jogo muda.

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0, e a prova visual dele continua valendo.

## Como o dono testa

No Windows, com o Docker Desktop aberto e um terminal na pasta do Nexum:

1. Compile o build de desenvolvimento:

   ```powershell
   py scripts/dev.py build
   ```

2. Suba o OpenMU de teste. A imagem é a mesma do laboratório da central: se ela ainda estiver no seu computador, o Docker não baixa nada; se não estiver, baixa cerca de 425 MB. Se o servidor do laboratório estiver ligado, desligue-o antes, porque os dois usam as mesmas portas. Espere cerca de 1 minuto até o servidor ficar pronto:

   ```powershell
   docker compose -f tests/integration/openmu/compose.yaml up -d
   ```

3. Entre no mundo. Quando o terminal pedir a senha, digite `test0` (ela não aparece) e tecle Enter:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0
   ```

4. Confira no terminal, em ordem: a lista com 3 servidores, a conexão com o Game Server, o login aceito, os personagens da conta, a entrada no mundo (com o mapa e a posição) e os jogadores, NPCs e monstros que entraram na visão.
5. Minimize a janela e deixe o programa rodando por 30 minutos. No Gerenciador de Tarefas, a coluna CPU do `MuClient` fica perto de 0%. Antes desta fatia, minimizado, ele ficava perto de 8% (92% de um núcleo, dividido pelos 12 núcleos lógicos).
6. Depois dos 30 minutos, restaure a janela e feche. A última linha do terminal resume a sessão: o tempo no mundo, os pings enviados (cerca de 90) e os objetos na visão.
7. Opcional: com o primeiro ainda no mundo, abra outro terminal e entre com a conta `test1` (senha `test1`). Se os dois personagens estiverem perto, cada terminal mostra o outro jogador entrando na visão.
8. Teste uma senha errada: rode o comando do passo 3 e digite qualquer outra senha. O terminal mostra "senha inválida", e o programa encerra com erro.
9. No fim, eu confiro com você o log do servidor. Um erro do Castle Siege logo no início é conhecido, vem do próprio OpenMU em modo de demonstração e não tem a ver com o cliente.
10. Desligue o servidor:

    ```powershell
    docker compose -f tests/integration/openmu/compose.yaml down
    ```

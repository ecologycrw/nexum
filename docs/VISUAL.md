# Guia visual

> Documento vivo. Mudanças exigem a aprovação do dono ([ADR 0028](adr/0028-prova-visual-e-paridade.md)). A fonte são as seções 0.3, D-09, IV.9.7 e V.2.3 da [especificação de referência](referencia/especificacao-engine-propria-caminho-b.md#v23-imagens-de-referência).

## A ideia central

O Nexum tem **dois modos de aparência**, e cada um tem um objetivo diferente:

| Modo | Objetivo | Paridade |
|---|---|---|
| **Classic** | parecer o original: espaço gama, luz por vértice a partir de uma grade, materiais clássicos como pilhas de passes, sombra planar | **P2**: medida contra o original |
| **Enhanced** | melhorar sem trair o estilo, tendo como referência o pacote que o MuMain criou: filtragem de textura, iluminação aprimorada, luzes dinâmicas, luz do sol (com nuvens e o tom do céu), light mood, pós-processamento (bloom, LUT de cor e AO) e antialiasing | **P3**: revisão visual, sem compromisso de paridade |

Cada recurso do Enhanced tem a sua própria chave e pode ser desligado sozinho, inclusive o AO separado do bloom. Os valores ficam em dados, por mapa e por ambiente, com a preferência do jogador por cima ([ADR 0030](adr/0030-render-forward-classic-e-enhanced.md)). O som não faz parte do Enhanced: o áudio 3D é a opção "Áudio 3D", à parte ([ADR 0031](adr/0031-audio-paridade-primeiro-e-audio-3d.md)).

O modo Classic é o padrão de referência de cada fatia de render. Nele, o light mood fica desligado e o antialiasing (MSAA) vem desligado por padrão, como no MuMain.

## De onde vêm as referências

- **O original é uma caixa preta.** A imagem de referência de uma cena é capturada **rodando** o MuMain, que tem captura sem janela e socket de automação. O código dele nunca é aberto (sala limpa).
- **As imagens com a arte do jogo ficam fora do repositório** ([ADR 0020](adr/0020-conteudo-original-e-distribuicao.md)). No repositório ficam só os parâmetros das cenas (mapa, posição, câmera, hora) e as tolerâncias.
- **As provas das fatias usam conteúdo próprio** (malhas simples, texturas geradas, um mapa pequeno), que fica em `Content/`. Essas imagens podem entrar no repositório, porque não têm nada da Webzen.

## Cenas de paridade

Cenas fixas, sempre no modo Classic e sem MSAA (no Nexum e no MuMain), sem efeitos aleatórios ou com eles congelados:

- Lorencia, numa posição e câmera conhecidas, ao meio-dia;
- um personagem de cada classe parado, com equipamento +0, +9, +13 e +15;
- Atlans, com a água;
- Hellas, com a superfície ondulante.

A comparação usa uma métrica perceptual (SSIM, por exemplo) e a diferença por pixel com tolerância. A tolerância de cada cena é definida e versionada junto com ela. Até a primeira cena de paridade, só existe a diferença por pixel ([TD-0016](TECH_DEBT.md)).

## O que reprova uma imagem

- Tela vazia, preta, de uma cor só ou sem o objeto que a spec pede.
- No Classic: diferença da referência acima da tolerância da cena, ou um erro que espelha o mundo (lateralidade trocada).
- No Classic: cores lavadas por iluminação em espaço linear. O original ilumina em espaço gama.
- Brilho aditivo desenhado na ordem errada: os transparentes seguem a ordem em que foram enviados.
- Texturas de cabeça para baixo ou com a origem errada.
- Qualquer arte que não veio do cache convertido do usuário ou do conteúdo próprio.

## Prova visual

Vale para toda fatia que muda o que aparece na tela.

1. **Cena de prova.** A spec define uma cena fixa: mesma câmera, mesmos objetos, mesmo tick e mesma semente dos sorteios (INV-07). A mesma cena gera sempre a mesma imagem.
2. **A IA olha.** A cada tarefa visual, a IA gera a imagem, abre o arquivo e compara com este guia. Ela anota no `tasks.md` o que viu, inclusive o que ficou ruim. Rodar o comando sem abrir a imagem não conta.
3. **Os números conferem.** Nas cenas de paridade, a comparação com a referência fica dentro da tolerância. Nas cenas de conteúdo próprio, a imagem é comparada com a última imagem aprovada.
4. **O revisor olha sozinho.** Ele abre as imagens sem confiar na descrição de quem fez o trabalho.
5. **O dono aprova.** Só o dono copia uma imagem para a pasta das referências aprovadas de conteúdo próprio, que é uma trava. Depois disso, uma mudança que altere o resultado sem aprovação é recusada.

**Como funciona hoje:**

- **Captura.** O `MuClient` do build de desenvolvimento grava a cena de prova sem janela, com `--capture ARQUIVO.png`. A cena não usa o relógio de verdade: ela recebe tempos de quadro fixos, e por isso sai sempre igual. O build de jogador recusa a opção.
- **Comparação.** A ferramenta `visualproof` (`Tools/VisualProof`, só no build de desenvolvimento) compara a captura com a imagem aprovada: `visualproof CAPTURA.png APROVADA.png --tolerance NÍVEIS`. A tolerância é obrigatória, porque cada cena declara a sua. A ferramenta termina com 0 quando aprova, 1 quando recusa e 2 quando a linha de comando está errada. A recusa diz o motivo: tamanhos diferentes, os pixels acima da tolerância (com o primeiro deles), arquivo ilegível ou imagem vazia ou de uma cor só.
- **Imagens aprovadas.** Ficam em `tests/visual/approved/`, com o número da fatia no nome (`0000-test-card.png`). Um teste do CTest para cada cena faz a captura e a comparação em sequência, nos 3 sistemas da CI.

## Fluidez

Metas da spec (V.5), medidas a partir do M3:

| Item | Meta |
|---|---|
| Tempo do tick (cena cheia, cerca de 100 entidades) | até 4 ms |
| Quadro na GPU, Classic, 1080p, GPU integrada recente | até 6 ms |
| Quadro na GPU, Enhanced, 1440p, GPU dedicada média | até 6 ms |
| Quadro mais longo durante a carga de um mapa | até 33 ms |
| Carga de um mapa (SSD, cache convertido) | até 2 s |
| Memória do processo (mapa grande, 100 entidades) | até 1,5 GB |
| Atraso entre o clique e o envio do comando | até 1 tick + 1 quadro |

Mede-se o tempo médio e o do pior 1% dos quadros, porque a média sozinha esconde os engasgos. O computador de desenvolvimento mostra a tendência, mas não comprova o piso de hardware ([TD-0018](TECH_DEBT.md)).

## Quando cada parte chega

| Marco | O que passa a valer |
|---|---|
| M0 | a janela mostra o cartão de teste; a ferramenta de prova visual nasce com a primeira cena de conteúdo próprio (o cartão no tick 50, com α 0,5), comparada pixel a pixel com tolerância de 1 nível por canal |
| M1 | o visualizador de modelos no modo de autoria; a pose do glTF exportado confere com a do visualizador |
| M3 | a primeira cena de paridade (Lorencia), com a comparação contra a referência |
| M4 e M5 | personagens, skinning, sombra planar, cores de estado, contorno de seleção |
| M8 | as pilhas de reflexo por nível de item |
| M14 | o modo Enhanced completo: os 7 recursos, os ambientes por mapa e os ajustes do jogador; à parte, a opção "Áudio 3D" |

# Guia visual

> Documento vivo. Mudanças exigem a aprovação do dono ([ADR 0028](adr/0028-prova-visual-e-paridade.md)). A fonte são as seções 0.3, D-09 e V.2.3 da [especificação de referência](referencia/especificacao-engine-propria-caminho-b.md#v23-imagens-de-referência).

## A ideia central

O Nexum tem **dois modos de aparência**, e cada um tem um objetivo diferente:

| Modo | Objetivo | Paridade |
|---|---|---|
| **Classic** | parecer o original: espaço gama, luz por vértice a partir de uma grade, materiais clássicos como pilhas de passes, sombra planar | **P2**: medida contra o original |
| **Enhanced** | melhorar sem trair o estilo: sombras do sol, AO, bloom, LUT de cor, luz por pixel, áudio 3D | **P3**: revisão visual, sem compromisso de paridade |

Cada melhoria do Enhanced é uma opção que pode ser desligada. O modo Classic é o padrão de referência de cada fatia de render.

## De onde vêm as referências

- **O original é uma caixa preta.** A imagem de referência de uma cena é capturada **rodando** o MuMain, que tem captura sem janela e socket de automação. O código dele nunca é aberto (sala limpa).
- **As imagens com a arte do jogo ficam fora do repositório** ([ADR 0020](adr/0020-conteudo-original-e-distribuicao.md)). No repositório ficam só os parâmetros das cenas (mapa, posição, câmera, hora) e as tolerâncias.
- **As provas das fatias usam conteúdo próprio** (malhas simples, texturas geradas, um mapa pequeno), que fica em `Content/`. Essas imagens podem entrar no repositório, porque não têm nada da Webzen.

## Cenas de paridade

Cenas fixas, sem efeitos aleatórios ou com eles congelados:

- Lorencia, numa posição e câmera conhecidas, ao meio-dia;
- um personagem de cada classe parado, com equipamento +0, +9, +13 e +15;
- Atlans, com a água;
- Hellas, com a superfície ondulante.

A comparação usa uma métrica perceptual (SSIM, por exemplo) e a diferença por pixel com tolerância. A tolerância de cada cena é definida e versionada junto com ela.

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

A ferramenta de comparação ainda não existe: ela entra no M0, com a primeira janela ([TD-0006](TECH_DEBT.md)).

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

Mede-se o tempo médio e o do pior 1% dos quadros, porque a média sozinha esconde os engasgos. O computador de desenvolvimento mostra a tendência, mas não comprova o piso de hardware ([TD-0005](TECH_DEBT.md)).

## Quando cada parte chega

| Marco | O que passa a valer |
|---|---|
| M0 | a janela limpa a tela; a ferramenta de prova visual nasce com uma cena de conteúdo próprio |
| M1 | o visualizador de modelos no modo de autoria; a pose do glTF exportado confere com a do visualizador |
| M3 | a primeira cena de paridade (Lorencia), com a comparação contra a referência |
| M4 e M5 | personagens, skinning, sombra planar, cores de estado, contorno de seleção |
| M8 | as pilhas de reflexo por nível de item |
| M14 | o modo Enhanced completo |

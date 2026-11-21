---
id: 0000
titulo: Fundação
status: em-andamento
aprovada-em: 2026-11-20
marco: M0
---

# Fatia 0: Fundação

## Resumo para o dono

No fim desta fatia, o `MuClient` abre uma janela e desenha pela placa de vídeo um **cartão de teste**: faixas de cores, uma escala de cinza, uma marca em "L" no canto de cima à esquerda e um quadrado que anda de um lado para o outro. A lógica anda em ticks fixos de 40 ms (25 por segundo), e o quadrado desliza liso no ritmo do monitor. Ele troca de cor uma vez por segundo, o que mostra a olho que os ticks estão no ritmo certo.

A engine também ganha as peças de base que todo o resto vai usar (log, asserts, handles, arenas e o profiler) e a ferramenta de prova visual. A CI fica verde nos 3 sistemas, já usando a placa de vídeo.

## Escopo

- **Laço de ticks** (`Engine/Time`, módulo novo): ticks de 40 ms, no máximo 5 ticks por quadro, o excesso descartado e contado, o α da interpolação, pausa e avanço tick a tick, temporizadores de parede e de tick, e o diagnóstico (ticks por segundo, ticks descartados, maior quadro e histograma do tempo do tick).
- **Peças de base** (`Engine/Core`): o `NEXUM_ASSERT`, os handles geracionais com um pool de capacidade fixa, a arena de memória por quadro e uma imagem RGBA simples.
- **Log e profiler** (`Engine/Debug`, módulo novo): log por categoria e por nível, com a saída trocável nos testes; o profiler Tracy atrás de macros, desligado por padrão e disponível só no build de desenvolvimento.
- **Plataforma** (`Engine/Platform`): o ponto de entrada pelos *main callbacks* do SDL, a janela (abrir, redimensionar, minimizar, restaurar e fechar), o relógio monotônico, a leitura e a gravação de PNG, e o Linux com janelas de verdade (X11 e Wayland).
- **Placa de vídeo** (`Engine/Render`, módulo novo): o dispositivo do SDL GPU com a ordem de escolha do [ADR 0029](../../docs/adr/0029-sdl3-e-sdl-gpu.md) (no Windows, Vulkan e depois Direct3D 12), a opção de forçar um backend, limpar a tela, enviar uma textura gerada e copiá-la para a tela, e a captura de um quadro sem janela.
- **Tela da fundação** no `MuClient`: o cartão de teste e o quadrado que anda por tick, desenhado com interpolação. Opções `--frames N` (fecha sozinho depois de N quadros) e `--gpu-driver` (força um backend). No build de desenvolvimento, `--capture arquivo.png` grava a cena de prova sem abrir janela.
- **Prova visual** (`Tools/visualproof`, ferramenta nova): compara duas imagens com tolerância e reprova imagens de uma cor só. A primeira imagem aprovada fica em `tests/visual/approved/`.
- **Teste de alocação do quadro:** prova que o quadro não pede memória ao sistema.
- **CI:** a placa de vídeo nos 3 sistemas (por software nas máquinas do GitHub), a janela no Linux num servidor de vídeo virtual e uma compilação com o profiler ligado.
- **Documentação:** a lista exata de requisitos do SDL GPU no piso de hardware da [VISÃO](../../docs/VISION.md), o estado do defeito do D3D12, o Tracy no [THIRD_PARTY](../../docs/THIRD_PARTY.md) e os itens provisórios resolvidos.

## Fora do escopo

- Shaders e o `SDL_shadercross`: a fatia só limpa a tela e copia texturas, o que não precisa de shader próprio. Entram com o primeiro modelo (M1).
- Modelos, câmera, texturas lidas de arquivo e o modo de autoria (ImGui): M1.
- Os workers (`Engine/Jobs`): M1, junto com a carga de arquivos.
- A contagem de samplers por quadro do [ADR 0029](../../docs/adr/0029-sdl3-e-sdl-gpu.md): ainda não existe nenhum sampler próprio. Entra com o primeiro material (M1).
- Teclado, mouse, a camada de ações e o IME: na fatia que precisar deles.
- Configuração em JSON, a pasta do usuário, o arquivo de log e o relatório de falhas: M7.
- O gerador de aleatoriedade (ADR 0015): na fatia do primeiro sorteio.
- A comparação perceptual (SSIM): M3, com a primeira cena de paridade.
- A troca da conta de máquina ([TD-0009](../../docs/TECH_DEBT.md)): depende do suporte do GitHub, não de código.

## Critérios de aceitação

- CA-01: O teste de tempo da spec (IV.2) passa: 10 minutos simulados com quadros irregulares de 5 a 200 ms dão exatamente 15.000 ticks, e um objeto que anda 3 unidades por tick termina exatamente na posição 45.000.
- CA-02: O laço segue as regras do [ADR 0005](../../docs/adr/0005-tick-fixo-de-25-hz.md), conferidas pelos exemplos do plano: no máximo 5 ticks por quadro; o tempo acima disso é descartado e contado; o α é o resto do acumulador dividido por 40 ms; com a simulação pausada não roda nenhum tick, o α fica parado e o tempo pausado não vira ticks depois; o avanço tick a tick roda exatamente um tick por pedido.
- CA-03: Os temporizadores de parede disparam pelo relógio monotônico, também com a simulação pausada, e os de tick contam só ticks. O diagnóstico informa ticks por segundo, ticks descartados, o maior quadro e o histograma do tempo do tick, com uma faixa para os ticks acima de 8 ms.
- CA-04: Um handle de um objeto removido é recusado, mesmo depois de o lugar ser reaproveitado, e um pool cheio devolve um erro em vez de crescer. A arena entrega memória alinhada, devolve um erro quando a capacidade acaba e volta a zero a cada quadro. Um `NEXUM_ASSERT` falso no build de desenvolvimento encerra o programa com uma mensagem que diz a condição, o arquivo e a linha.
- CA-05: O log filtra por nível, marca a categoria de cada mensagem e escreve na saída escolhida. As mensagens do próprio SDL passam pelo log da engine. Com o profiler desligado, as macros não geram código; com ele ligado, o projeto compila e liga o Tracy (um passo da CI). O build de jogador recusa o profiler ligado.
- CA-06: A ordem de escolha do backend é a do [ADR 0029](../../docs/adr/0029-sdl3-e-sdl-gpu.md): no Windows, Vulkan e, se ele faltar, Direct3D 12; no Linux, Vulkan; no macOS, Metal. A opção `--gpu-driver` força um backend, e um nome desconhecido dá um erro claro. O `MuClient` registra no log o backend e a placa usados.
- CA-07: A cena de prova (o cartão de teste em 640×480, no tick 50, com α de 0,5), capturada sem janela, confere com a imagem aprovada pelo dono em `tests/visual/approved/0000-test-card.png`, com no máximo 1 nível de diferença por canal e nenhum pixel fora disso, nos 3 sistemas da CI.
- CA-08: A ferramenta `visualproof` aprova imagens iguais dentro da tolerância e reprova, cada caso com uma mensagem própria: tamanhos diferentes, um único pixel acima da tolerância, arquivo ilegível e imagem de uma cor só.
- CA-09: Depois de 3 quadros de aquecimento, 1.000 quadros completos da tela da fundação (ticks, retrato e desenho sem janela) não pedem nenhuma memória ao sistema.
- CA-10: `MuClient --frames 300` abre a janela, desenha 300 quadros e fecha sozinho, com sucesso, nos 3 sistemas da CI. No Linux, a janela usa X11 num servidor de vídeo virtual.
- CA-11: No computador do dono, o `MuClient` mostra o cartão de teste com o quadrado andando liso e trocando de cor uma vez por segundo. Redimensionar, minimizar e restaurar a janela não trava nem distorce o cartão, e fechar a janela encerra o programa. O log diz `vulkan`. Verificação manual, registrada no report.
- CA-12: O critério de pronto do M0 na spec de referência fica provado no report: a CI verde nos 3 sistemas, o teste de tempo (CA-01) e um `#include` proibido de propósito reprovado pela checagem `architecture`.
- CA-13: A [VISÃO](../../docs/VISION.md) lista os requisitos exatos do SDL GPU na versão adotada. O [TD-0005](../../docs/TECH_DEBT.md) fica só com o defeito do D3D12, com destino M14. Os itens TD-0003 e TD-0006 saem, e o que sobra deles vira item novo com destino. O Tracy fica registrado no [THIRD_PARTY](../../docs/THIRD_PARTY.md).

## Paridade

Não se aplica: nenhum nível de P0 a P3 é medido nesta fatia. O ritmo de 25 ticks por segundo, que vem do original ([I.2 da spec](../../docs/referencia/especificacao-engine-propria-caminho-b.md#i2-tempo-os-25-fps-de-referência)), é conferido pelo teste de tempo (CA-01).

## Prova visual

- **Cena de prova:** o cartão de teste numa imagem de 640×480, no tick 50, com α de 0,5. Não há câmera: tudo é desenhado em pixels, em tamanho real.
  - Nos 400 pixels de cima: 8 faixas verticais de 80 pixels (branco, amarelo, ciano, verde, magenta, vermelho, azul e preto), com uma escala de cinza de 0 a 255 embaixo delas e uma marca em "L" branca no canto de cima à esquerda.
  - Nos 80 pixels de baixo: a cor de fundo e o quadrado de 32×32 pixels, que anda 4 pixels por tick entre as bordas e troca de cor a cada 25 ticks.
- **O que a imagem precisa mostrar:** as faixas na ordem certa; a escala de cinza indo do preto ao branco sem cores lavadas; a marca em "L" no canto de cima à esquerda, sem estar de cabeça para baixo nem espelhada; o quadrado no ponto calculado entre o tick 49 e o tick 50 (x = 198), com a terceira cor. Uma imagem preta, vazia ou de uma cor só reprova ([guia visual](../../docs/VISUAL.md#o-que-reprova-uma-imagem)).
- **Comparação:** com a última imagem aprovada pelo dono, `tests/visual/approved/0000-test-card.png`, com no máximo 1 nível por canal e nenhum pixel fora. Na primeira vez ainda não há imagem aprovada: a IA gera a imagem, abre e descreve o que viu, e o dono aprova antes de ela entrar na pasta das aprovadas, que é uma trava.
- **Fluidez:** as metas de fluidez só valem a partir do M3. Esta fatia registra no report, só como referência, o tempo médio e o pior 1% dos quadros no computador do dono.

## Como o dono testa

No Windows, num terminal aberto na pasta do Nexum:

1. Compile o build de desenvolvimento:

   ```powershell
   py scripts/dev.py build
   ```

2. Abra o jogo:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe
   ```

3. Confira na janela:
   - as 8 faixas de cores, a escala de cinza e a marca em "L" no canto de cima à esquerda;
   - o quadrado andando liso de um lado para o outro, sem tremer;
   - a cor do quadrado trocando uma vez por segundo (em 10 segundos, 10 trocas).
4. Redimensione a janela, minimize e restaure. O cartão continua igual, e o quadrado continua andando.
5. Feche a janela. O programa termina, e o terminal mostra uma linha com `vulkan` e o nome da sua placa de vídeo.
6. Opcional: abra com o outro backend e confira que a janela fica igual:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --gpu-driver direct3d12
   ```

7. Abra a imagem da prova visual, `build\windows-dev\tests\visual\0000-test-card.png`, e diga se ela mostra o mesmo que a janela. Ao aprovar, ela vira a imagem de referência.

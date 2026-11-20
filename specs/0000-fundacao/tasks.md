# Tarefas da fatia 0

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

## Sondagem da CI (o maior risco, primeiro)

- [ ] T01: Sondagem sem janela. Cria o módulo `Engine/Render` com o `GpuDevice`: a ordem de escolha do backend do ADR 0029, a opção de forçar um backend e o erro para um nome desconhecido. Também cria um alvo sem janela que limpa a imagem e lê os pixels de volta, e informa o backend e a placa. O `Engine/Platform` ganha o `VideoSystem`, que liga o vídeo do SDL (CA-06). Verificação: `py scripts/dev.py verify` passa aqui, com a sondagem rodando na placa deste computador.
- [ ] T02: Sondagem com janela. A `Window` mínima em `Engine/Platform` e um teste que abre a janela, limpa e apresenta alguns quadros. No Linux, o SDL com X11 e Wayland (`vcpkg.json`, TD-0003), os pacotes e o Xvfb no `setup-build`, e os testes dentro do Xvfb no `ci.yml` (CA-10). Verificação: `verify` aqui.
- [ ] T03: A sondagem na CI dos 3 sistemas, por um PR em rascunho, porque a CI só roda em PR. Verificação: todos os checks verdes, com o lavapipe no Linux, o WARP no Windows e o Metal no macOS. Se um sistema falhar, a fatia para e as opções vão para o dono.

## Peças sem tela

- [ ] T04: `Engine/Time`: o `TickClock`, com os 4 exemplos do plano como testes (CA-01, CA-02). Verificação: `verify`.
- [ ] T05: Os temporizadores de parede e de tick e o diagnóstico do laço: ticks por segundo, ticks descartados, maior quadro e histograma do tick (CA-03). Verificação: `verify`.
- [ ] T06: O `NEXUM_ASSERT`, ligado no build de desenvolvimento e desligado no de jogador, com um teste do CTest que espera a falha e a mensagem (CA-04). Verificação: `verify`.
- [ ] T07: O `Handle` geracional e o `HandlePool` de capacidade fixa (CA-04). Verificação: `verify`.
- [ ] T08: A `FrameArena` e a `Image` RGBA (CA-04). Verificação: `verify`.
- [ ] T09: `Engine/Debug`: o `Logger` com categorias, níveis e saída trocável, e as mensagens do SDL passando pelo log (CA-05). Verificação: `verify`.
- [ ] T10: O profiler: as macros, o Tracy com `NEXUM_PROFILER`, a recusa no build de jogador e um passo da CI que compila com ele ligado (CA-05). Verificação: `verify` e o build com o profiler ligado aqui.

## Parte gráfica

- [ ] T11: Plataforma: a entrada pelos *main callbacks* do SDL, os eventos da janela (redimensionar, minimizar, restaurar e fechar), o relógio monotônico e a leitura e a gravação de PNG (CA-10, CA-11). Verificação: `verify`.
- [ ] T12: Render: texturas por handle, o envio de uma textura gerada, a cópia para a tela e a leitura do alvo sem janela como `Image` (CA-07). Verificação: `verify`.
- [ ] T13: A tela da fundação no `MuClient`: o cartão de teste, o quadrado que anda por tick com interpolação, o retrato na arena do quadro, `--frames`, `--gpu-driver` e o log do backend e da placa (CA-06, CA-10, CA-11). Verificação: `verify` e a janela aberta aqui.
- [ ] T14: A prova visual: `--capture`, a ferramenta `Tools/visualproof` e o teste do CTest que junta os dois. A primeira imagem vai para a aprovação do dono antes de entrar em `tests/visual/approved/` (CA-07, CA-08). Verificação: `verify` e a imagem conferida.
- [ ] T15: O teste de alocação do quadro: 1.000 quadros sem pedir memória ao sistema (CA-09). Verificação: `verify`.

## Fechamento

- [ ] T16: O `other_game` passa a usar o relógio de ticks. A documentação viva fica em dia: VISÃO, VISUAL, ARCHITECTURE, CONVENTIONS, THIRD_PARTY, DEV.md e os itens provisórios TD-0003, TD-0005 e TD-0006 (CA-12, CA-13). Verificação: `verify` e a checagem `docs`.
- [ ] T17: A CI completa verde nos 3 sistemas (CA-10, CA-12). Verificação: os checks do PR.

O CA-11 é a verificação manual do dono, registrada no report.

## Bloqueios

*(nenhum)*

## Prova visual

O que a IA viu em cada imagem da cena de prova, inclusive o que ficou ruim.

## Revisão

Achados do revisor e o que foi feito com cada um. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

- O `CMakePresets.json` tira dos testes os que têm a etiqueta `gpu`. A fatia não usa essa etiqueta: o plano manda os testes da placa de vídeo rodarem em todo `verify` e na CI, que usa placas por software. Mexer no filtro é mudar uma trava.

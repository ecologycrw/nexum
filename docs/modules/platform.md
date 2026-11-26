# Módulo: Platform

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

Tudo o que fala com o sistema operacional pelo SDL, menos a placa de vídeo: a entrada do programa, os eventos, o vídeo, a janela, a leitura e a gravação de PNG, o log do SDL e o modo das falhas no Windows. O SDL só aparece aqui e no [Render](render.md). Teclado, mouse e a camada de ações ainda não existem.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_platform` (`Nexum::Platform`) | Engine | biblioteca; liga o `Nexum::Core` e o `Nexum::Debug`, e o SDL3 por dentro |

Cabeçalhos públicos em `Engine/Platform/Public/Nexum/Platform/`.

## Conceitos

- **`Application` e `RunApplication`** (`Application.hpp`): o programa é uma `Application`, com uma chamada por quadro (`Iterate`) e uma por evento (`OnEvent`). As duas respondem continuar, sucesso ou falha. O `RunApplication` entrega o programa ao SDL pelos *main callbacks* e devolve o código de saída: 0 no sucesso e 1 na falha. Só um programa roda por vez.
- **Eventos** (`Event`): `QuitRequested`, `WindowResized` (com o tamanho em pixels), `WindowMinimized`, `WindowRestored` e `WindowCloseRequested`, cada evento de janela com o id da janela.
- **`VideoSystem`** (`VideoSystem.hpp`): liga o vídeo do SDL enquanto existe, e diz o driver de vídeo (`windows`, `x11`...).
- **`Window`** (`Window.hpp`): abre a janela, dá o tamanho em pixels e o id, muda o tamanho e pede o fechamento, com o mesmo efeito do botão de fechar. Lados zero ou maiores que 16.384 são recusados. O `IsVisible` diz se a janela aparece na tela: não aparece minimizada, escondida ou coberta por outras (nem todo sistema avisa quando ela está coberta). A alça nativa da janela só é aberta pelo [Render](render.md).
- **`WaitFor`** (`Wait.hpp`): para a thread por pelo menos o tempo pedido, com o temporizador mais preciso do sistema (o `SDL_DelayNS`). Quando a janela não aparece na tela, o laço do `MuClient` não envia o quadro para a placa e usa essa espera até o próximo tick.
- **PNG** (`Png.hpp`): o `LoadPng` e o `SavePng` convertem para `Image` RGBA e de volta. As imagens vazias e as maiores que 16.384 pixels de lado (`kMaxPngSide`) são recusadas. É só para arquivos de confiança: as imagens aprovadas e as capturas do próprio Nexum.
- **`SdlLogRoute`** (`SdlLogRoute.hpp`): enquanto existe, as mensagens do SDL vão para o log da engine, na categoria `sdl` ou numa por área (`sdl.gpu`, `sdl.video`...). As rotas podem se aninhar e terminar fora de ordem.
- **Falhas como texto** (`FailureReports.hpp`): com a variável `NEXUM_FAILURES_AS_TEXT` ligada, que os presets de teste ligam, o Windows escreve as falhas do `abort` e dos asserts da biblioteca do C como texto, em vez de abrir uma janela e ficar parado. Nos outros sistemas, não faz nada.
- **`PlatformInfo`** (`PlatformInfo.hpp`): o sistema operacional atual e a versão do SDL.

## Como usar

O `MuClient` (`Games/MuOnline/App/Private/Main.cpp`) chama o `WriteFailuresAsTextWhenAsked` na primeira linha e depois o `RunApplication` com a `FoundationApp`. O começo do vídeo e da placa, que a janela e a captura usam, fica em `Games/MuOnline/App/Private/ClientStart.cpp`.

## Fontes da central

Não se aplica: o módulo não reproduz nada do original.

## Invariantes relevantes

- **INV-10:** o SDL fica atrás deste módulo e do Render.
- **INV-08:** o leitor de PNG nunca abre um arquivo original do jogo.
- **INV-12:** os tamanhos de janela e de imagem e os argumentos são validados.
- **INV-14:** os erros voltam por valor (`Result`).
- **INV-15:** a janela é testada no Windows, no Linux (X11, dentro do Xvfb) e no macOS.

## Testes

Em `Engine/Platform/Tests/` (`nexum_platform_tests`): `ApplicationTest.cpp` (os quadros, os códigos de saída e os eventos com uma janela de verdade), `WindowTest.cpp`, `WaitTest.cpp`, `VideoSystemTest.cpp`, `PngTest.cpp` (inclusive um PNG feito fora do Nexum, que confere a ordem dos canais), `SdlLogRouteTest.cpp`, `FailureReportsTest.cpp` e `PlatformInfoTest.cpp`. Minimizar e restaurar não têm teste automático, porque o Xvfb da CI não tem gerenciador de janelas; o dono confere no teste da fatia.

## Itens provisórios

- [TD-0010](../TECH_DEBT.md): no Linux, nenhum teste abre janela no Wayland.
- [TD-0013](../TECH_DEBT.md): no Windows, os argumentos chegam na página de código do sistema.
- [TD-0015](../TECH_DEBT.md): com a janela minimizada e o Vulkan, o laço gira sem pausa.
- [TD-0020](../TECH_DEBT.md): o `LoadPng` confere o tamanho só depois de ler o arquivo, e não tem fuzzing.

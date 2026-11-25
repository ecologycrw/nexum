# Módulo: Render

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A placa de vídeo pelo SDL GPU: escolher e ligar o backend, guardar texturas por handle, desenhar uma lista de cópias de textura numa janela ou num alvo sem janela, e ler de volta a imagem desenhada. Na fatia 0, o desenho é só limpar e copiar texturas inteiras, sem shader próprio, sem escala e sem filtro. Shaders, modelos, câmera e materiais chegam no M1.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_render` (`Nexum::Render`) | Engine | biblioteca; liga o `Nexum::Core` e o `Nexum::Platform`, e o SDL3 por dentro |

Cabeçalhos públicos em `Engine/Render/Public/Nexum/Render/`.

## Conceitos

- **Backends** (`GpuBackend.hpp`): Vulkan, Direct3D 12 e Metal. A ordem de tentativa segue o [ADR 0029](../adr/0029-sdl3-e-sdl-gpu.md): no Windows, Vulkan e depois Direct3D 12; no Linux, Vulkan; no macOS, Metal. Um nome desconhecido dá um erro claro.
- **`GpuDevice`** (`GpuDevice.hpp`): liga a placa na ordem do sistema, ou o backend forçado, e descreve o que começou: o backend, a placa e o driver.
- **`TextureStore`** (`TextureStore.hpp`): guarda um número fixo de texturas RGBA, num `HandlePool` do [Core](core.md). O `Upload` envia uma `Image` à placa; o `Release` libera, e o handle antigo passa a ser recusado. O lado máximo é 16.384 (`kMaxTextureSide`), o limite que o Direct3D 12 garante.
- **`DrawList`** (`DrawList.hpp`): o que desenhar num quadro. É uma cor de limpeza e até 64 cópias de texturas inteiras (`kMaxBlits`), cada uma com o canto de cima à esquerda numa posição em pixels. O tamanho é fixo, a lista nunca pede memória, e o desenho só a lê.
- **`OffscreenTarget`** (`OffscreenTarget.hpp`): um alvo sem janela, usado na captura e nos testes. O `ReadImage` espera a placa terminar e devolve a `Image`. Ele deixa no máximo 2 quadros na fila (`kFramesInFlight`): antes de gravar um quadro novo, espera a placa terminar o mais antigo.
- **`WindowTarget`** (`WindowTarget.hpp`): desenha na janela. Quando a janela não pode mostrar nada, por exemplo minimizada, o quadro é pulado (`FrameOutcome::Skipped`). A espera pela tela é a do SDL, que também deixa no máximo 2 quadros na fila.
- **Recusas antes do trabalho:** uma lista com uma textura que não está no depósito, ou com texturas de outro dispositivo, é recusada antes de qualquer trabalho na placa, e nada é desenhado. As partes de uma cópia que ficam fora do alvo são cortadas.
- **`MakeTestCard`** (`TestCard.hpp`): o cartão de teste de 640×480 da fatia 0, feito na CPU. Tem as 8 faixas, a escala de cinza, o "L" sobre o quadrado preto e a faixa livre embaixo.

## Como usar

A tela da fundação envia o cartão uma vez ao `TextureStore` e, a cada quadro, monta uma `DrawList` com o cartão e o quadrado. A janela usa o `WindowTarget::Present`; a captura e o teste de alocação usam o `OffscreenTarget::Draw` (`Games/MuOnline/Private/Foundation/`).

## Fontes da central

Não se aplica: o módulo não reproduz nada do original. Os requisitos do SDL GPU 3.4.20 vêm da documentação do SDL e estão na [VISÃO](../VISION.md#piso-de-hardware).

## Invariantes relevantes

- **INV-04:** o desenho só lê a lista e o retrato do quadro.
- **INV-10:** o SDL GPU fica atrás deste módulo.
- **INV-11:** as texturas são acessadas por handle.
- **INV-13:** a lista de desenho e a fila de quadros não pedem memória no quadro.
- **INV-15:** os testes que usam a placa rodam nos 3 sistemas. Na CI, a placa é feita por software.

## Testes

Em `Engine/Render/Tests/` (`nexum_render_tests`, com as peças comuns em `GpuFixture.hpp`):

- `GpuDriverOrderTest.cpp`, `DrawListTest.cpp` e `TestCardTest.cpp` não usam a placa;
- `GpuDeviceTest.cpp`, `TextureStoreTest.cpp`, `TextureCopyTest.cpp`, `OffscreenTargetTest.cpp` e `WindowTargetTest.cpp` usam a placa de verdade, ou a placa por software na CI.

As cópias são comparadas pixel a pixel com a mesma cena calculada na CPU, em cada backend que o sistema oferece.

## Itens provisórios

- [TD-0005](../TECH_DEBT.md): o defeito do Direct3D 12 com mais de 2.048 samplers; por isso o Vulkan vem primeiro no Windows.
- [TD-0017](../TECH_DEBT.md): os samplers de cada quadro ainda não são contados.
- [TD-0021](../TECH_DEBT.md): um handle de textura não diz de qual depósito veio.
- [TD-0018](../TECH_DEBT.md): a CI só tem placas por software.

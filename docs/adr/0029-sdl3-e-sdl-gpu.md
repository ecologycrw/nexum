---
id: 0029
titulo: Plataforma e GPU com SDL3 e SDL GPU, com o Vulkan como padrão no Windows
status: aceita
data: 2026-11-20
substitui: 0004
substituida-por:
---

# ADR 0029: Plataforma e GPU com SDL3 e SDL GPU, com o Vulkan como padrão no Windows

Fonte: [D-04 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-04--plataforma-e-gpu-sdl3--sdl-gpu) e [IV.9.7.2](../referencia/especificacao-engine-propria-caminho-b.md#iv972-o-que-o-nexum-faz). Substitui o [ADR 0004](0004-sdl3-e-sdl-gpu.md): a decisão é a mesma, e a spec revista acrescentou um defeito conhecido do D3D12 e a escolha do Vulkan como padrão no Windows.

## Contexto

A engine precisa de janela, entrada, texto, pastas do usuário e acesso à GPU nos três sistemas. O SDL GPU esconde as diferenças entre Direct3D 12, Vulkan e Metal, e o cliente moderno estudado pela central prova que ele funciona nos três.

A doc oficial desse cliente registra um defeito do SDL no Direct3D 12: com o modo Enhanced ligado, um quadro pode precisar de mais descritores de sampler do que um heap do backend D3D12 comporta (2048). A troca de heap no meio do quadro tem defeito, e o jogo pode fechar ou o Windows pode reiniciar a GPU. O Vulkan não é afetado.

## Decisão

- **SDL3** para janela, eventos, entrada, texto e IME, e pastas do usuário.
- **SDL GPU** como RHI, usado diretamente atrás do módulo `Engine/Render`. A engine não cria uma RHI própria por cima.
- **Shaders em HLSL**, compilados no build com o `SDL_shadercross` para SPIR-V, DXIL e MSL. No build de desenvolvimento, também em tempo de execução, para o hot-reload.
- **O SDL só aparece** nos módulos `Engine/Platform` e `Engine/Render` (INV-10).
- **No Windows, o padrão é o Vulkan** onde ele funcionar, até que o defeito do D3D12 esteja corrigido na versão do SDL adotada. O Direct3D 12 continua disponível como alternativa.
- **O build de desenvolvimento conta os samplers** usados em cada quadro, para mostrar quando um quadro se aproxima do limite.

## Consequências

- **Hardware mínimo:** GPU e sistema com Direct3D 12, Vulkan ou Metal. Máquinas que só têm OpenGL ficam de fora. A lista exata de requisitos é conferida no M0 ([TD-0005](../TECH_DEBT.md)).
- **Navegador (WebAssembly)** fica fora do escopo: o SDL GPU não tem um backend estável para ele.
- **Celular** fica fora do escopo até o fim da Fase 1.
- **O defeito do D3D12 é conferido no M0 e no M14** na versão do SDL adotada ([TD-0005](../TECH_DEBT.md)). Quando ele estiver corrigido, o padrão do Windows pode voltar a ser revisto.

## Alternativas consideradas

- **RHI própria sobre Vulkan e Direct3D 12:** meses de trabalho de baixo nível, sem ganho para este jogo.
- **bgfx:** maduro, mas com o próprio modelo de shaders e menos alinhado com o que o cliente moderno prova.
- **Direct3D 12 como padrão no Windows:** é o caminho mais comum no Windows, mas hoje pode travar a GPU com o Enhanced ligado.

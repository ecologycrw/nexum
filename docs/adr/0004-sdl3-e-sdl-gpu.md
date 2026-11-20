---
id: 0004
titulo: Plataforma e GPU com SDL3 e SDL GPU
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0004: Plataforma e GPU com SDL3 e SDL GPU

Fonte: [D-04 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-04--plataforma-e-gpu-sdl3--sdl-gpu).

## Contexto

A engine precisa de janela, entrada, texto, pastas do usuário e acesso à GPU nos três sistemas. O SDL GPU esconde as diferenças entre Direct3D 12, Vulkan e Metal, e o cliente moderno estudado pela central prova que ele funciona nos três.

## Decisão

- **SDL3** para janela, eventos, entrada, texto e IME, e pastas do usuário.
- **SDL GPU** como RHI, usado diretamente atrás do módulo `Engine/Render`. A engine não cria uma RHI própria por cima.
- **Shaders em HLSL**, compilados no build com o `SDL_shadercross` para SPIR-V, DXIL e MSL. No build de desenvolvimento, também em tempo de execução, para o hot-reload.
- **O SDL só aparece** nos módulos `Engine/Platform` e `Engine/Render` (INV-10).

## Consequências

- **Hardware mínimo:** GPU e sistema com Direct3D 12, Vulkan ou Metal. Máquinas que só têm OpenGL ficam de fora. A lista exata de requisitos é conferida no M0 ([TD-0005](../TECH_DEBT.md)).
- **Navegador (WebAssembly)** fica fora do escopo: o SDL GPU não tem um backend estável para ele.
- **Celular** fica fora do escopo até o fim da Fase 1.

## Alternativas consideradas

- **RHI própria sobre Vulkan e Direct3D 12:** meses de trabalho de baixo nível, sem ganho para este jogo.
- **bgfx:** maduro, mas com o próprio modelo de shaders e menos alinhado com o que o cliente moderno prova.

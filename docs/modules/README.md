# Módulos

> Documentos vivos: cada página nasce na fatia que cria o sistema, a partir de `templates/module.md`, e é atualizada no mesmo PR que muda o sistema ([WORKFLOW](../WORKFLOW.md)).

| Módulo | Camada | O que faz | Desde |
|---|---|---|---|
| [Core](core.md) | Engine | assert, erros por valor, handles, memória do quadro e imagem RGBA | preparação; peças da fatia 0 |
| [Time](time.md) | Engine | relógio monotônico, laço de ticks de 40 ms, temporizadores e diagnóstico | fatia 0 |
| [Debug](debug.md) | Engine | log por categoria e nível, e o profiler Tracy atrás de macros | fatia 0 |
| [Platform](platform.md) | Engine | entrada do programa, eventos, janela, vídeo, PNG e o log do SDL | preparação; peças da fatia 0 |
| [Render](render.md) | Engine | placa de vídeo pelo SDL GPU, texturas, lista de desenho e alvos | fatia 0 |
| [VisualProof](visualproof.md) | Tools | compara a captura com a imagem aprovada | fatia 0 |

O módulo MU (`Games/MuOnline`) ainda só tem a tela da fundação, que é provisória ([TD-0014](../TECH_DEBT.md)). A página dele nasce quando a primeira peça de verdade do jogo chegar. O kit (`Kit/GridRpg`, alvo `nexum_gridrpg`) está no mesmo caso: hoje ele só tem a identificação do kit (`KitIdentity`), que o "outro jogo" dos testes de arquitetura usa, e a página dele nasce com o primeiro sistema do gênero.

# Módulos

> Documentos vivos: cada página nasce na fatia que cria o sistema, a partir de `templates/module.md`, e é atualizada no mesmo PR que muda o sistema ([WORKFLOW](../WORKFLOW.md)).

| Módulo | Camada | O que faz | Desde |
|---|---|---|---|
| [Core](core.md) | Engine | assert, erros por valor, handles, memória do quadro, imagem RGBA e o FNV-1a | preparação; peças das fatias 0 e 2 |
| [Time](time.md) | Engine | relógio monotônico, laço de ticks de 40 ms, temporizadores e diagnóstico | fatia 0 |
| [Debug](debug.md) | Engine | log por categoria e nível, a linha de log sem pedir memória e o profiler Tracy atrás de macros | fatia 0; o `LogLine` da fatia 1 |
| [Platform](platform.md) | Engine | entrada do programa, eventos, janela, vídeo, espera, linha sem eco, PNG, arquivo inteiro e o log do SDL | preparação; peças das fatias 0, 1 e 2 |
| [Render](render.md) | Engine | placa de vídeo pelo SDL GPU, texturas, lista de desenho e alvos | fatia 0 |
| [Net](net.md) | Engine | conexões TCP numa thread própria, com as filas e a transformação de bytes do jogo | fatia 1 |
| [MU: Protocol](mu-protocol.md) | Games | enquadramento, cifra, dialeto 106.3, item e aparência estendidos | fatia 1 |
| [MU: Session](mu-session.md) | Games | a conversa com o OpenMU até "No mundo", os prazos, a época e o ping | fatia 1 |
| [MU: Translate](mu-translate.md) | Games | os pacotes do mundo viram eventos neutros | fatia 1 |
| [MU: World](mu-world.md) | Games | os eventos neutros e a visão do mundo montada com eles | fatia 1 |
| [MU: Online](mu-online.md) | Games | o cliente online no quadro, o adaptador de servidor (o OpenMU ao vivo e o replay), o transporte de verdade e as opções do `MuClient` | fatia 1; o adaptador e o replay da fatia 2 |
| [MU: Recording](mu-recording.md) | Games | o formato `.mucap`, a impressão do estado do mundo e o gravador | fatia 2 |
| [PacketGen](packetgen.md) | Tools | gera do XML do OpenMU o código de todos os pacotes | fatia 1 |
| [VisualProof](visualproof.md) | Tools | compara a captura com a imagem aprovada | fatia 0 |

No módulo MU (`Games/MuOnline`), cada área com uma peça de verdade do jogo tem a sua página, com o nome "MU:". A tela da fundação continua provisória ([TD-0014](../TECH_DEBT.md)) e não tem página. O kit (`Kit/GridRpg`, alvo `nexum_gridrpg`) está no mesmo caso: hoje ele só tem a identificação do kit (`KitIdentity`), que o "outro jogo" dos testes de arquitetura usa, e a página dele nasce com o primeiro sistema do gênero.

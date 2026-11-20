# Bibliotecas de terceiros

Toda biblioteca do `vcpkg.json` fica registrada aqui com a licença dela ([ADR 0021](adr/0021-repositorio-publico-mit.md)). A checagem `structure` recusa uma biblioteca sem registro e um registro sem biblioteca. Uma biblioteca nova precisa da aprovação do dono.

## Em uso

| Biblioteca | Licença | Para quê | Entra no jogo distribuído |
|---|---|---|---|
| `sdl3` | zlib; um cabeçalho do OpenGL ES usa Apache-2.0 | janela, entrada, placa de vídeo e pastas do usuário | sim |
| `tl-expected` | CC0-1.0 | devolver erros por valor (`Result<T>`) | sim |
| `doctest` | MIT | testes automáticos | não |

## Planejadas

Previstas pela spec (Apêndice D), cada uma na fatia que precisar dela. A licença é conferida na versão adotada antes de entrar.

- SDL_ttf e SDL_shadercross (zlib): texto e compilação de shaders. Entram no M0 ou no M1.
- Tracy (BSD-3): profiler, só no build de desenvolvimento. Entra no M0.
- KTX-Software (Apache 2.0), stb_image ou libjpeg-turbo, cgltf (MIT) e meshoptimizer (MIT): o importador. Entram no M1.
- Dear ImGui (MIT, ramo docking): o modo de autoria. Entra no M1.
- EnTT (MIT): o armazenamento das entidades, atrás da fachada `World`. Entra no M1 ou no M2.
- Asio standalone (BSL-1.0): os sockets. Entra no M2.
- Uma biblioteca JSON (nlohmann/json ou simdjson, MIT ou Apache 2.0): dados e configuração.
- GLM (MIT): matemática, atrás do Core.
- RmlUi (MIT) e FreeType (FTL, que pede crédito): a interface. Entram no M6.
- HarfBuzz ("Old MIT"), se o texto precisar de montagem: M6.
- miniaudio (domínio público ou MIT-0): o áudio. Entra no M7.
- OpenMU (MIT): os XML de pacotes e, se for o caso, código, sempre com o aviso da licença. Entra no M2, em `third_party/openmu`.

Bibliotecas com licença GPL não entram no jogo distribuído.

As ferramentas usadas só no desenvolvimento, como compiladores e analisadores, não entram no jogo e não ficam nesta lista.

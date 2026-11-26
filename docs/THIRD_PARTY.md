# Bibliotecas de terceiros

Toda biblioteca do `vcpkg.json` fica registrada aqui com a licença dela ([ADR 0021](adr/0021-repositorio-publico-mit.md)). A checagem `structure` recusa uma biblioteca sem registro e um registro sem biblioteca. Uma biblioteca nova precisa da aprovação do dono.

## Em uso

| Biblioteca | Licença | Para quê | Entra no jogo distribuído |
|---|---|---|---|
| `sdl3` | zlib; um cabeçalho do OpenGL ES usa Apache-2.0 | janela, entrada, placa de vídeo (o SDL GPU, com Vulkan, Direct3D 12 e Metal), leitura e gravação de PNG e pastas do usuário | sim |
| `asio` | BSL-1.0 | as conexões TCP, atrás do `Engine/Net` (o Asio standalone 1.38.2, sem a Boost, usado só com códigos de erro) | sim |
| `tl-expected` | CC0-1.0 | devolver erros por valor (`Result<T>`) | sim |
| `doctest` | MIT | testes automáticos | não |

## Opcionais

Ficam numa opção (*feature*) do `vcpkg.json` e só entram no build quando ela é ligada. A checagem `structure` ainda não confere esta lista: ela lê só as bibliotecas principais do `vcpkg.json`.

- **Tracy 0.15.0** (BSD-3-Clause, BSD-2-Clause e MIT): o profiler, atrás do `Engine/Debug`. Entra só no build de desenvolvimento e só com `NEXUM_PROFILER=ON` (a opção `profiler` do `vcpkg.json`), no modo "sob demanda", sem as ferramentas gráficas e sem o tratador de falhas. Não entra no jogo distribuído. Aprovado pelo dono na fatia 0.

## Conteúdo de terceiros fora do vcpkg

Arquivos de outros projetos guardados no próprio repositório, cada um com a licença ao lado. A checagem `structure` não confere esta lista, porque ela não vem do `vcpkg.json`.

- **OpenMU, as definições de pacotes** (MIT, Copyright 2017 MUnique): os XML de pacotes, o `CommonEnums.xml`, o `PacketHeaders.xml` e o esquema `PacketDefinitions.xsd`, da base `b53c704c6`, em [`third_party/openmu`](../third_party/openmu/README.md), com o aviso da licença. O `packetgen` lê esses arquivos no build e gera o código dos pacotes. O código gerado entra no jogo distribuído. Aprovado pelo dono com a spec da fatia 1.

## Planejadas

Previstas pela spec (Apêndice D), cada uma na fatia que precisar dela. A licença é conferida na versão adotada antes de entrar. O módulo dono de cada uma está no [ADR 0033](adr/0033-dono-de-cada-biblioteca-externa.md).

- SDL_ttf e SDL_shadercross (zlib): texto e compilação de shaders. Entram no M1.
- KTX-Software (Apache 2.0), stb_image ou libjpeg-turbo, cgltf (MIT) e meshoptimizer (MIT): o importador. Entram no M1.
- Dear ImGui (MIT, ramo docking): o modo de autoria. Entra no M1.
- EnTT (MIT): o armazenamento das entidades, atrás da fachada `World`. Entra no M1 ou no M2.
- Uma biblioteca JSON (nlohmann/json ou simdjson, MIT ou Apache 2.0): dados e configuração.
- GLM (MIT): matemática, com os tipos apelidados num único cabeçalho do Core. Entra no M1.
- RmlUi (MIT) e FreeType (FTL, que pede crédito): a interface. Entram no M6.
- HarfBuzz ("Old MIT"), se o texto precisar de montagem: M6.
- miniaudio (domínio público ou MIT-0): o áudio. Entra no M7.

Bibliotecas com licença GPL não entram no jogo distribuído.

As ferramentas usadas só no desenvolvimento, como compiladores e analisadores, não entram no jogo e não ficam nesta lista.

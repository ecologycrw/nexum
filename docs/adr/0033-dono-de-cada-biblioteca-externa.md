---
id: 0033
titulo: O módulo dono de cada biblioteca externa e os tipos de vocabulário
status: aceita
data: 2026-11-25
substitui:
substituida-por:
---

# ADR 0033: O módulo dono de cada biblioteca externa e os tipos de vocabulário

## Contexto

O INV-10 dizia que cada biblioteca externa fica atrás de um único módulo da engine e que os cabeçalhos públicos nunca a expõem. Uma revisão da documentação, em 2026-11-25, mostrou quatro problemas nesse texto:

- **O `tl::expected` já aparece nos cabeçalhos públicos.** O `Result<T>` do `Engine/Core` é um apelido dele, e o `tl::unexpected` é usado para devolver erros em 15 arquivos, do Core ao módulo MU e às ferramentas. A regra não registrava essa exceção.
- **O GLM não tem como ficar escondido sem custo.** Vetores, matrizes e rotações atravessam as APIs do Render, da Animation, do kit e do módulo MU. Para esconder o GLM, o Core teria de escrever os próprios tipos de matemática e todas as operações deles.
- **O SDL já tem dois donos,** o `Engine/Platform` e o `Engine/Render`, e o texto falava em "um único módulo".
- **As bibliotecas planejadas para o M1 e o M6** ([THIRD_PARTY](../THIRD_PARTY.md)) não tinham dono definido: SDL_ttf, SDL_shadercross, KTX-Software, stb_image ou libjpeg-turbo, cgltf, meshoptimizer, a biblioteca JSON, FreeType e HarfBuzz.

## Decisão

1. **Cada biblioteca tem um dono declarado.** O dono é o único lugar do código do Nexum que a inclui ou a liga. A checagem `architecture` confere isso nos donos da engine.
2. **Tipos de vocabulário.** Uma biblioteca cujos tipos são a própria linguagem das APIs pode aparecer num único cabeçalho público do `Engine/Core`, que dá a esses tipos nomes da engine. O resto do código usa os nomes do Core, nunca a biblioteca diretamente. São duas:
   - **`tl::expected`:** o `Result<T>` e o `MakeError`, em `Nexum/Core/Result.hpp`. O `tl::unexpected`, usado hoje em vários módulos para repassar um erro, faz parte dessa exceção até a troca pelo `std::expected` ([TD-0004](../TECH_DEBT.md)).
   - **GLM:** um cabeçalho de matemática do Core, que chega no M1 com a primeira matemática, dá os nomes da engine (por exemplo, `Vec3`, `Mat4` e `Quat`) aos tipos do GLM. Só esse cabeçalho e o `Engine/Core/Private/` incluem o GLM.
3. **Os donos:**

   | Biblioteca | Dono | Quando entra |
   |---|---|---|
   | SDL3 | `Engine/Platform` (janela, eventos, entrada, PNG, pastas do usuário) e `Engine/Render` (SDL GPU) | já em uso |
   | `tl-expected` | `Engine/Core`, como tipo de vocabulário | já em uso |
   | doctest | só os testes | já em uso |
   | Tracy e Dear ImGui | `Engine/Debug` | Tracy em uso; ImGui no M1 |
   | GLM | `Engine/Core`, como tipo de vocabulário | M1 |
   | SDL_ttf | `Engine/Render`, com o motor de texto da GPU | M1 |
   | SDL_shadercross | o build: compila os shaders offline e nunca entra no jogo | M1 |
   | KTX-Software | `Engine/Assets` no jogo; `Tools/MuImport` na conversão | M1 |
   | stb_image ou libjpeg-turbo, cgltf e meshoptimizer | `Tools/MuImport`; nunca entram no build de jogador (INV-02) | M1 |
   | EnTT | `Engine/World` | M1 ou M2 |
   | Asio | `Engine/Net` | M2, parte 1 |
   | Biblioteca JSON | `Engine/Core/Private/`, por trás de um leitor neutro que os dados, a configuração e o socket de automação usam | na primeira fatia que ler JSON |
   | RmlUi | `Engine/Ui` | M6 |
   | FreeType e HarfBuzz | ninguém diretamente: entram só por dentro do SDL_ttf e do RmlUi | M1 e M6 |
   | miniaudio | `Engine/Audio` | M7 |

4. **Uma biblioteca nova continua precisando da aprovação do dono do produto.** Esta tabela não aprova nenhuma biblioteca. Ela só diz onde cada uma vai ficar. A fatia que trouxer a biblioteca confirma o dono, e um dono diferente exige um ADR novo.

## Consequências

- O INV-10 da [ARCHITECTURE](../ARCHITECTURE.md#invariantes) passa a citar este ADR, com as duas exceções de vocabulário.
- O Core não precisa escrever uma biblioteca de matemática própria.
- O preço: trocar o GLM ou o `tl::expected` deixa de ser uma mudança só dentro do Core. Os nomes da engine continuam iguais, mas o código que usa as operações deles teria de ser revisto.
- Quando o GLM entrar, a checagem `architecture` passa a aceitar o include dele no cabeçalho de matemática do Core, além do `Engine/Core/Private/`.
- A troca do `tl::expected` pelo `std::expected` não é mais "uma linha": inclui trocar os `tl::unexpected` pelo `std::unexpected` em todos os módulos. O TD-0004 diz isso.

## Alternativas consideradas

- **Tipos de matemática próprios no Core, com o GLM só por dentro:** é a leitura literal da regra antiga. Exige escrever e testar todas as operações à mão, sem ganho para o jogador.
- **Liberar o GLM em qualquer cabeçalho:** espalha os nomes da biblioteca pelo código e torna uma troca futura muito mais cara.

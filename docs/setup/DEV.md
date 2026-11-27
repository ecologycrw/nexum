# Ambiente de desenvolvimento

## Requisitos

No Windows:

- Windows 10/11 64 bits.
- Visual Studio 2022 ou 2026, com a carga de trabalho **Desenvolvimento para desktop com C++** e o componente **Ferramentas do C++ Clang** (traz o `clang-format` e o `clang-tidy`).
- CMake 3.28 ou mais novo e Ninja. O mínimo do projeto é o 3.28, mas o build com os detectores de erro de memória no Linux exige o 3.29 (veja abaixo). Num computador novo, instale logo o 3.29 ou mais novo.
- vcpkg, com a variável de ambiente `VCPKG_ROOT` apontando para a pasta dele.
- Python 3.12 ou mais novo.
- Git e GitHub CLI (`gh`).

No Windows, as bibliotecas do vcpkg entram dentro dos próprios programas (o triplet `x64-windows-static-md`), e não existe nenhuma DLL para copiar. Por isso, os presets desligam o passo do vcpkg que copia as DLLs para perto de cada programa (`VCPKG_APPLOCAL_DEPS=OFF`): ele não fazia nada e, na CI, às vezes parava o build com um arquivo em uso. Se um dia uma biblioteca vier como DLL, o programa nem abre e os testes falham; aí é preciso copiar essa DLL pelo CMake ou religar a opção.

No Linux (usado na CI): Clang, GCC, CMake, Ninja, vcpkg e Python 3. O SDL precisa também das bibliotecas de janela (X11 e Wayland) e de um driver Vulkan. Sem placa de vídeo, o driver por software (lavapipe) serve. Sem tela, os testes rodam dentro do Xvfb, como na CI:

```bash
sudo apt-get install ninja-build clang g++ libvulkan1 mesa-vulkan-drivers \
  libx11-dev libxext-dev libwayland-dev libxkbcommon-dev libegl1-mesa-dev xvfb
xvfb-run --auto-servernum --server-args="-screen 0 1280x1024x24" python3 scripts/dev.py verify
```

O build com os detectores de erro de memória no Linux exige o CMake 3.29 ou mais novo e o lavapipe (`libvulkan_lvp.so`, do pacote `mesa-vulkan-drivers`). Sem o lavapipe, a configuração para com um erro, para a checagem de vazamentos nunca passar em silêncio. No Linux, os testes abrem janelas só no X11 ([TD-0010](../TECH_DEBT.md)), e o teste `muclient_draws_300_frames_and_closes` confere isso no log (CA-10 da fatia 0). Numa sessão Wayland, o SDL pode escolher o Wayland mesmo dentro do Xvfb: rode os testes sem a variável `WAYLAND_DISPLAY` (`env -u WAYLAND_DISPLAY xvfb-run ...`).

No macOS (usado na CI): as ferramentas de linha de comando do Xcode (Clang), CMake, Ninja, vcpkg e Python 3.

## Primeira vez num clone

Ative os hooks do Git, uma vez por clone:

```bash
git config core.hooksPath .githooks
```

A identidade dos commits da IA e a assinatura estão no [guia do GitHub](GITHUB.md).

A primeira compilação baixa e compila as bibliotecas externas e leva alguns minutos. As seguintes usam o cache do vcpkg.

## Comandos do dia a dia

Tudo passa por um comando só. No Linux e no macOS, use `python3` no lugar de `py`.

| O quê | Comando |
|---|---|
| **Verificar tudo** | `py scripts/dev.py verify` |
| Verificação rápida (a mesma do commit) | `py scripts/dev.py quick` |
| Compilar | `py scripts/dev.py build` |
| Rodar os testes | `py scripts/dev.py test` |
| Rodar os testes com os detectores de erro de memória | `py scripts/dev.py sanitize` |
| Compilar e testar o build de jogador | `py scripts/dev.py release` |
| Conferir a formatação | `py scripts/dev.py format` |
| Corrigir a formatação | `py scripts/dev.py format --fix` |
| Análise estática | `py scripts/dev.py tidy` |
| Checagens do repositório | `py scripts/dev.py checks` |
| Testes das checagens e dos hooks | `py scripts/dev.py pytests` |

No Linux, `--compiler gcc` troca o Clang pelo GCC: `python3 scripts/dev.py --compiler gcc test`.

Fora da CI, os comandos mostram só um resumo de cada passo: os testes que passaram, os passos de compilação e as linhas com aviso. A saída completa fica em `build/dev-last.log`, e um comando que falha aparece inteiro na tela. Para ver tudo, como a CI sempre mostra, use `--full`: `py scripts/dev.py --full verify`.

O `scripts/dev.py` encontra o Visual Studio sozinho, então os comandos funcionam em qualquer terminal.

### Profiler

O profiler (Tracy) vem desligado e só existe no build de desenvolvimento: o build de jogador recusa a opção. Para ligar, configure uma pasta de build própria com `NEXUM_PROFILER=ON`. O vcpkg instala o Tracy na primeira vez. No Windows, rode num *Developer PowerShell* do Visual Studio:

```bash
cmake --preset windows-dev -B build/windows-profiler -DNEXUM_PROFILER=ON
cmake --build build/windows-profiler
```

No Linux e no macOS, troque o preset por `linux-dev`, `linux-gcc-dev` ou `macos-dev`. Ligado, cada programa abre uma porta de rede para o visualizador do Tracy, e o Windows pede a liberação do firewall. O código marca o trabalho com as macros de `Nexum/Debug/Profiler.hpp`, nunca com o Tracy direto. Ainda não há comando no `scripts/dev.py` ([TD-0012](../TECH_DEBT.md)).

### O MuClient e a prova visual

O programa fica na pasta de build do preset, como `build/windows-dev/Games/MuOnline/App/MuClient.exe`:

| O quê | Opção |
|---|---|
| Ver a versão e o tipo de build | `--version` |
| Desenhar N quadros e fechar sozinho | `--frames N` |
| Escolher o backend da placa de vídeo (no Windows, o padrão é o Vulkan) | `--gpu-driver vulkan`, `direct3d12` ou `metal` |
| Gravar a cena de prova sem janela (só no build de desenvolvimento) | `--capture ARQUIVO.png` |
| Entrar num OpenMU: o Connect Server e a conta. A senha é pedida no terminal, sem aparecer, e nunca vem pela linha de comando | `--connect HOST:PORTA --account NOME` |
| Escolher o servidor e o personagem (sem as opções, o primeiro da lista) | `--server ID`, `--character NOME` |
| Encerrar com sucesso depois de N segundos no mundo | `--world-seconds N` |
| Gravar a sessão no mundo num arquivo `.mucap`, com a versão do servidor no cabeçalho (só no build de desenvolvimento; vem com `--connect`) | `--record ARQUIVO.mucap [--server-version TEXTO]` |
| Reproduzir uma gravação sem servidor (só no build de desenvolvimento; sem as opções do caminho até o mundo, `--record` e `--capture`) | `--replay ARQUIVO.mucap` |

O teste `visual_proof_0000_test_card` faz a captura e a compara com `tests/visual/approved/0000-test-card.png`: `ctest --preset windows-dev -R visual_proof`. A captura fica em `build/windows-dev/tests/visual/0000-test-card.png`. A comparação sozinha é `visualproof CAPTURA.png APROVADA.png --tolerance NÍVEIS`, que termina com 0 quando aprova, 1 quando recusa e 2 quando a linha de comando está errada. Só o dono põe imagens em `tests/visual/approved/` ([VISUAL](../VISUAL.md#prova-visual)).

### O servidor de teste do OpenMU

O teste de integração usa um OpenMU de verdade, sem modificações, no Docker: a imagem `munique/openmu:0.10.1`, fixada pela impressão digital, no modo `-demo`. Nesse modo, os dados e as contas de teste são criados de novo a cada início. A senha de cada conta é o próprio nome, de `test0` a `test9`. Na primeira vez, o Docker baixa cerca de 425 MB. As portas só abrem em 127.0.0.1, e o contêiner usa os processadores 0 a 5 (`tests/integration/openmu/compose.yaml`). As contas são públicas: nunca exponha esse servidor. O servidor do laboratório da central usa as mesmas portas, então só um dos dois fica ligado por vez.

```powershell
docker compose -f tests/integration/openmu/compose.yaml up -d
.\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0
docker compose -f tests/integration/openmu/compose.yaml logs
docker compose -f tests/integration/openmu/compose.yaml down
```

O servidor fica pronto em poucos segundos, quando o log mostra "Client Listener started, Port 44406". Os testes com a etiqueta `openmu` só rodam quando a variável `NEXUM_OPENMU_ADDRESS` diz o endereço do Connect Server. Sem ela, como no `verify` e na CI, o CTest os marca como pulados ([TD-0034](../TECH_DEBT.md)). Com o servidor no ar, os três levam cerca de 5 minutos: 3 minutos no mundo, a senha errada, e 20 segundos gravados e reproduzidos (`openmu_muclient_records_and_replays`, só no build de desenvolvimento):

```powershell
$env:NEXUM_OPENMU_ADDRESS = "127.0.0.1:44406"
ctest --preset windows-dev -L openmu
```

### Gravar e reproduzir uma sessão

Só o build de desenvolvimento grava e reproduz ([MU: Recording](../modules/mu-recording.md)). A gravação começa na entrada no mundo e guarda só o que chega do servidor, então a conta e a senha nunca vão para o arquivo. Ela fica na memória, com até 16 MiB, e o arquivo é escrito quando o cliente termina ([TD-0039](../TECH_DEBT.md)). Com o servidor de teste no ar, grave 60 segundos e reproduza com o servidor desligado:

```powershell
.\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0 --world-seconds 60 --record build\sessao.mucap --server-version "OpenMU 0.10.1"
docker compose -f tests/integration/openmu/compose.yaml down
.\build\windows-dev\Games\MuOnline\App\MuClient.exe --replay build\sessao.mucap
```

A gravação termina com a linha "client: wrote the recording to ...: N messages, N ticks and N checkpoints". O replay roda no ritmo gravado e confere cada tick e cada ponto de controle. Sem diferença, ele termina com 0 e a linha "the replay checked N messages, N ticks and N checkpoints". Uma diferença termina com 1, e a mensagem diz em qual tick ou ponto de controle ela aconteceu, com o valor gravado e o do replay. As linhas `world:` dos dois terminais são as mesmas: os mesmos objetos entram na visão e saem dela, na mesma ordem.

### Refazer a gravação de referência

A gravação de referência (`Games/MuOnline/Tests/Recordings/openmu-two-players.mucap`) muda quando uma mudança legítima na tradução ou na visão do mundo muda o que o replay confere: o replay acusa a diferença, e a gravação é refeita contra o servidor de teste, em poucos minutos.

1. Suba o servidor de teste e grave 20 segundos com a `test0`, digitando `test0` como senha:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test0 --world-seconds 20 --record Games\MuOnline\Tests\Recordings\openmu-two-players.mucap --server-version "OpenMU 0.10.1"
   ```

2. Uns 3 segundos depois, num segundo terminal, entre com a `test1` por 8 segundos, digitando `test1` como senha:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --connect 127.0.0.1:44406 --account test1 --world-seconds 8
   ```

3. Confira no log da `test0` as linhas "the player test1Dk (...) appeared at" e "left the view". A posição de entrada da `test0Dk` muda de uma entrada para outra: se ela nascer longe da `test1Dk`, a `test1` não entra na visão, e é preciso gravar de novo.
4. Confira que a conta e a senha não estão no arquivo: todo texto com `test0` dentro dele precisa ser o nome do personagem, `test0Dk`.
5. Copie o arquivo para `Games/MuOnline/Tests/FuzzSeeds/Recording/openmu-two-players.bin`, a semente do fuzzing, que é uma cópia igual.
6. Mude os números que os testes conferem: o teste da biblioteca em `Games/MuOnline/Tests/ReplayServerTest.cpp` e o `muclient_replays_the_reference_recording` em `Games/MuOnline/App/CMakeLists.txt` (as mensagens, os ticks, os pontos de controle, o id e a posição da `test1Dk` e os objetos na visão no fim).

### Fuzzing

Cada leitor de dados de fora tem um alvo de fuzzing (`cmake/NexumFuzzing.cmake`): um arquivo `*FuzzTest.cpp` e uma pasta de sementes em `Tests/FuzzSeeds/`. Em todo build, as sementes rodam como um teste comum (`*_seeds`). O fuzzing de verdade, com o libFuzzer, só existe no Linux com Clang, pelo preset `linux-fuzz`. A CI roda cada alvo por 60 segundos ([TD-0026](../TECH_DEBT.md)). Para rodar no Linux, como a CI:

```bash
cmake --preset linux-fuzz
cmake --build --preset linux-fuzz --target nexum_fuzz_targets
```

Os programas ficam em `build/linux-fuzz/fuzz/`, e o `targets.txt` dessa pasta diz a pasta de sementes de cada um.

### Testes sem as janelas de falha do Windows

No Windows, um assert da biblioteca do C++, um `abort` ou uma queda abrem uma janela que deixa o programa parado até alguém clicar. Nos testes, isso nunca acontece:

- **Os programas de teste** escrevem essas falhas como texto e terminam na hora, sempre, pelo `main` comum (`tests/support/DoctestMain.cpp`) e pelo doctest.
- **O `MuClient` e o `visualproof`** fazem o mesmo quando a variável de ambiente `NEXUM_FAILURES_AS_TEXT` está ligada (`Nexum/Platform/FailureReports.hpp`). Os presets de teste a ligam. Rodando o `MuClient` à mão, a janela continua aparecendo, porque ela ajuda a investigar.
- **Tempo limite:** os presets de teste dão a cada teste no máximo 300 segundos. Um teste que travar por outro motivo é encerrado e aparece como falha, em vez de parar a verificação e a CI.

Os dois valores ficam no `CMakePresets.json`, que é uma trava.

## O que roda automaticamente

| Quando | O quê | Onde |
|---|---|---|
| Antes de cada commit | busca de segredos; com código: `quick`; só com docs: checagens de docs e de termos | `.githooks/pre-commit` |
| Ao escrever a mensagem de commit | formato `tipo(escopo): descrição` | `.githooks/commit-msg` |
| Antes de cada push | bloqueia envio para `main` e `master` | `.githooks/pre-push` |
| A cada PR e a cada push na `main` | tudo de novo, em Windows, Linux (Clang e GCC) e macOS, mais o build de jogador, a análise estática, os diagramas e o resumo de segurança do PR | `.github/workflows/ci.yml` |

## As checagens do repositório

O `scripts/checks` confere o que o compilador não confere:

| Checagem | O que recusa |
|---|---|
| `docs` | links quebrados, specs incompletas, critério de aceitação sem prova, seções "Prova visual", "Fontes da central usadas", "Suficiência das descrições" e "Sala limpa" ausentes, plano aprovado com perguntas abertas para a central, ADRs fora de sequência, item provisório sem destino |
| `structure` | arquivos obrigatórios faltando, módulos fora do padrão, nomes de arquivo errados, biblioteca sem licença registrada, arquivos originais do jogo no repositório |
| `architecture` | dependência proibida entre camadas, e bibliotecas externas ou o sistema operacional fora do módulo dono |
| `cpp-rules` | `new` e `delete` soltos (o `#include <new>` é aceito), funções perigosas de C, `rand`, relógios do sistema fora de `Engine/Time/Private` e de `Tools/`, `reinterpret_cast` fora de `Engine/Platform/Private`, `throw` no runtime. A troca do `operator new` global só é aceita no contador de `tests/architecture/frame_allocations/`. Em todo o código, inclusive nos testes e em `Tools/`, um `NOLINT`, `NOLINTNEXTLINE` ou `NOLINTBEGIN` que não diga entre parênteses cada regra que desliga, sem `*`, ou que não tenha um comentário com o motivo na linha de cima |
| `terms` | a linha vermelha: nomes do MU em `Engine/` e `Kit/`, e nomes dos formatos originais fora de `Tools/` |

## Central de conhecimento

A central existe só no computador do dono ([ADR 0027](../adr/0027-central-de-conhecimento-e-sala-limpa.md)). Para ligá-la, crie o arquivo `.claude/central.local.json`, que fica fora do Git:

```json
{ "path": "C:/caminho/para/a/central" }
```

Sem esse arquivo, a skill `/consultar-central` só avisa que a central não está disponível, e o hook de acesso não restringe nada.

Abra o Nexum e a central em **sessões separadas**.

## Se um hook bloquear

Leia a mensagem: ela diz o que corrigir. **Nunca** use `--no-verify` para pular uma checagem. Se a regra parecer errada, registre uma proposta (ver [WORKFLOW.md](../WORKFLOW.md#propostas-como-as-necessidades-aparecem)).

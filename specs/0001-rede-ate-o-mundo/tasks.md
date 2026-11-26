# Tarefas da fatia 1

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui. Por isso ele é curto. O detalhe de cada tarefa vai para a mensagem do commit, e as provas, para o report.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

Cada tarefa tem a primeira linha no formato abaixo, que o hook do início da sessão lê, e no máximo 2 linhas depois dela: o resultado em uma frase e o commit.

- [x] T01: O Asio no `vcpkg.json` (trava), o `find_package` na raiz e o registro no THIRD_PARTY (CA-06, CA-15). Verificação: `verify` com o Asio instalado pelo vcpkg.
  - O Asio 1.38.2 entrou pelo vcpkg e é achado na raiz do CMake; `verify` verde nos 3 builds. Commit: este.
- [x] T02: Os XML do OpenMU em `third_party/openmu`, copiados pelo dono, com o aviso MIT e a base `b53c704c6`, registrados no THIRD_PARTY (CA-03, CA-15). Verificação: checagem `structure` e os arquivos iguais aos da central.
  - Os 7 arquivos e a licença, copiados pelo dono, têm as mesmas linhas da central; checagens `docs`, `structure` e `terms` aceitando. Commit: este.
- [x] T03: O `packetgen` (`Tools/PacketGen`, Python só com a biblioteca padrão), a função do CMake que gera o código no build (trava `cmake/`), os testes gerados e o ADR 0034 (CA-03). Verificação: testes do gerador e os testes gerados no `verify`.
  - O `packetgen` gera os 613 pacotes dos 4 XML (cabeçalhos na pasta de build) e um teste por pacote; o XML de teste próprio confere cada tipo de campo; `verify` verde (29, 30 e 24 testes). Commit: este.
- [x] T04: O dialeto 106.3 (`Mu/Protocol`): a tabela de variantes, um teste por linha da §5.2 e a configuração única com a versão `20404` e o número de série `NEXUM-CLIENT-001` (CA-03, CA-12). Verificação: `verify`.
  - `Mu/Protocol/Dialect.hpp`: um apelido para cada variante da §5.2, a identidade do cliente e a porta 44406; 22 testes (um por linha e a identidade). Commit: este.
- [ ] T05: O enquadramento `C1` a `C4`, incremental, com os exemplos do plano (CA-01). Verificação: `verify`.
- [ ] T06: A cifra: SimpleModulus com o contador de cada direção, Xor32 e Xor3, com os vetores da central (CA-02). Verificação: `verify`.
- [ ] T07: O item estendido e a aparência estendida, cada um num componente neutro (CA-04). Verificação: `verify`.
- [ ] T08: Os pacotes de exemplo da central lidos e escritos de novo, com a contagem por grupo (CA-05). Verificação: `verify`.
- [ ] T09: A estrutura de fuzzing (`NEXUM_FUZZING` em `cmake/`, o preset no `CMakePresets.json` e o passo da CI, travas) e um alvo por leitor; o `LoadPng` confere o cabeçalho (TD-0020, TD-0023) (CA-10). Verificação: `verify` e as sementes nos testes.
- [ ] T10: O transporte `Engine/Net`: a conexão TCP com o Asio, a thread de rede, as filas e a interface da transformação de bytes (CA-06). Verificação: `nexum_net_tests` e as checagens `architecture` e `terms`.
- [ ] T11: A sessão (`Mu/Session`) de "Desconectado" a "No mundo", com os prazos, a época, os eventos guardados no carregamento e o log que mascara as credenciais (CA-07, CA-12). Verificação: `verify`.
- [ ] T12: O ping a cada 20 s de relógio de parede, desde a entrada no Game Server (CA-08). Verificação: `verify`.
- [ ] T13: A tradução para eventos neutros e a visão do mundo (`Mu/Translate` e `Mu/World`) (CA-09). Verificação: `verify`.
- [ ] T14: O `MuClient` com as opções novas, a senha sem eco (`Engine/Platform`) e a sessão dentro do laço (CA-12). Verificação: `verify`.
- [ ] T15: A janela minimizada: o laço espera o próximo tick (TD-0015) (CA-13). Verificação: teste com relógio de teste no `verify`.
- [ ] T16: O quadro sem alocação com mensagens de rede chegando por um transporte de teste (CA-11). Verificação: `muonline_frame_allocation_tests`.
- [ ] T17: O OpenMU 0.10.1 em `tests/integration/openmu` (Docker, fixado pela impressão digital) e o teste de integração de 3 minutos (CA-14). Verificação: o teste contra o servidor local.
- [ ] T18: A documentação: ARCHITECTURE, páginas dos módulos, THIRD_PARTY, ADR 0034 e TECH_DEBT (saem o TD-0015, o TD-0020 e o TD-0023) (CA-15). Verificação: checagens `docs` e `structure` no `verify`.

## Etapas

Depois das tarefas, cada etapa do ciclo marca a sua linha quando termina, com a data e, na revisão, o commit. O hook do início da sessão lê esta lista para dizer a etapa e o próximo passo (`docs/WORKFLOW.md`).

- [ ] Revisão (`/revisar`)
- [ ] Fechamento (`/fechar`)
- [ ] Teste do dono
- [ ] Retrospectiva (`/retrospectiva`)

## Decisões do dono

Cada escolha que o dono fez durante a fatia, com as palavras dele e a tarefa: o que mudou e por quê. Nenhuma decisão do dono fica só no meio do texto de uma tarefa.

- **2026-11-25, antes da T01:** "Sim, todas". O dono aprovou de uma vez as travas da fatia: o Asio no `vcpkg.json` (T01, com o download pelo vcpkg), a função do `packetgen` em `cmake/` (T03) e o fuzzing em `cmake/`, no `CMakePresets.json` e no `.github/` (T09). A checagem `structure` não muda. O dono já copiou os XML do OpenMU para `third_party/openmu` (T02).

## Bloqueios

*(nenhum)*

## Prova visual

Não se aplica: a janela continua com o cartão de teste da fatia 0.

## Revisão

Achados do revisor e o que foi feito com cada um, uma linha por achado. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

- A central fica no caminho de `.claude/central.local.json`. Só a leitura é liberada, pelas ferramentas de arquivo (o terminal não chega lá), e `Client/` é proibido. A spec de protocolo é `docs/specs/protocolo/protocolo-106-3-m2.md`, e os achados ficam em `knowledge/protocolo/`.
- A checagem `architecture` já reserva o Asio para o `Engine/Net` (`Private/` e `CMakeLists.txt`). A checagem `structure` só confere a tabela "Em uso" do THIRD_PARTY contra o `vcpkg.json`: os XML do OpenMU ficam registrados numa lista fora dessa tabela, como o Tracy, sem mudar a checagem.
- O servidor do laboratório da central (`nexum-lab-openmu`) usa as mesmas portas do servidor de teste: só um fica ligado por vez. Todo contêiner roda com `--cpuset-cpus=0-5`.

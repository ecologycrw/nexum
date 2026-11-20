# Ambiente de desenvolvimento

## Requisitos

No Windows:

- Windows 10/11 64 bits.
- Visual Studio 2022 ou 2026, com a carga de trabalho **Desenvolvimento para desktop com C++** e o componente **Ferramentas do C++ Clang** (traz o `clang-format` e o `clang-tidy`).
- CMake 3.28 ou mais novo e Ninja.
- vcpkg, com a variável de ambiente `VCPKG_ROOT` apontando para a pasta dele.
- Python 3.12 ou mais novo.
- Git e GitHub CLI (`gh`).

No Linux (usado na CI): Clang, GCC, CMake, Ninja, vcpkg e Python 3.

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

O `scripts/dev.py` encontra o Visual Studio sozinho, então os comandos funcionam em qualquer terminal.

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
| `cpp-rules` | `new` e `delete` soltos, funções perigosas de C, `rand`, relógios fora de Platform e Time, `reinterpret_cast`, `throw` no runtime |
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

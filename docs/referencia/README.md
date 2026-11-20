# Referência: a especificação do Caminho B

Esta pasta guarda uma **cópia fiel** da especificação que deu origem ao Nexum:
[especificacao-engine-propria-caminho-b.md](especificacao-engine-propria-caminho-b.md).

A cópia não é editada. Ela é a fonte das decisões que o projeto adotou, e mudar o arquivo exige a aprovação do dono (é uma trava).

## De onde veio

| Item | Valor |
|---|---|
| Origem | central de conhecimento do dono, `docs/specs/arquitetura/especificacao-engine-propria-caminho-b.md` |
| Data da cópia | 2026-11-20 |
| Versão na central | commit `73ebd22` mais edições locais ainda não commitadas (front matter `atualizado: 2026-11-20`) |
| Base analisada pela spec | Server (OpenMU) `b53c704c6` e Client (MuMain) `c405ae907` |
| SHA-256 do texto (finais de linha LF) | `b1c3ada05dc690cf79aea32697488f4a9dbcc0b75fe13b0346c40aeedb5e16a1` |
| Status na central | proposta |

O hook do início da sessão compara este hash com a versão atual da spec na central. Se forem diferentes, ele avisa, e a atualização da cópia vira uma proposta para o dono decidir.

## Como ler a cópia

- Os links internos da spec são relativos à pasta dela **na central** (por exemplo, `../../../knowledge/...`). Aqui eles não abrem. Por isso a checagem de links ignora esta pasta.
- As citações `arquivo:linha` apontam para o código do MuMain e do OpenMU **na central**. O Nexum nunca abre o código do MuMain (sala limpa, [ADR 0027](../adr/0027-central-de-conhecimento-e-sala-limpa.md)).
- Os rótulos **Fato**, **Interpretação**, **Decisão**, **Recomendação** e **Meta** continuam valendo como estão na spec.

## Como a spec virou o Nexum

As decisões D-01 a D-20 da Parte II foram adotadas uma por uma, como os ADRs [0001](../adr/0001-caminho-b-sala-limpa.md) a [0020](../adr/0020-conteudo-original-e-distribuicao.md). Os ADRs 0021 a 0028 registram as decisões do ambiente de desenvolvimento, espelhado do outro projeto do dono, o Lemuria.

| Parte da spec | Onde está no Nexum |
|---|---|
| Parte 0, Visão geral | [VISION.md](../VISION.md) |
| Parte II, decisões D-01 a D-20 | [ADRs 0001 a 0020](../adr/README.md) |
| Parte III, Arquitetura | [ARCHITECTURE.md](../ARCHITECTURE.md) |
| Parte V, Qualidade e paridade | [ADR 0014](../adr/0014-qualidade-e-paridade-medida.md), [WORKFLOW.md](../WORKFLOW.md#definição-de-pronto) e [VISUAL.md](../VISUAL.md) |
| Parte VI, Roteiro | [ROADMAP.md](../ROADMAP.md) |
| Parte VII e VIII, Riscos e Lacunas | consultadas pelo `/planejar` antes de cada marco |
| Apêndice D, Dependências | [THIRD_PARTY.md](../THIRD_PARTY.md) |
| Apêndice E, Sala limpa | [WORKFLOW.md](../WORKFLOW.md#sala-limpa) e o modelo de relatório |
| D-03 e D-19, `CODING_RULES.md` | [CONVENTIONS.md](../CONVENTIONS.md) |

Se uma decisão mudar, um ADR novo substitui o antigo e diz isso no front matter. A cópia da spec continua como estava.

## Ajustes feitos na adoção

Pontos em que o Nexum escolheu um caminho que a spec deixava em aberto, ou em que foi preciso ajustar algo:

| Tema | O que a spec diz | O que o Nexum faz | Por quê |
|---|---|---|---|
| Ferramentas internas | não fala delas | scripts em Python ficam em `scripts/` | No Windows, `tools/` e `Tools/` (muimport, packetgen) são a mesma pasta ([ADR 0022](../adr/0022-organizacao-do-repositorio.md)) |
| Erros por valor | `std::expected` do C++23 | `Result<T>` sobre `tl::expected`, em C++20 | O `std::expected` exige o modo C++23 nos três compiladores; a interface é a mesma e a troca é uma linha ([TD-0004](../TECH_DEBT.md)) |
| Alvos do build | `Nexum::<Módulo>` | alvo real `nexum_<módulo>`, com o alias `Nexum::<Módulo>` | O CMake reserva nomes com `::` para alias |
| Módulos do kit e do jogo | subpastas `Grid/`, `Protocol/`... | um alvo por camada; as áreas ficam em subpastas de `Public/` e `Private/` | Mantém a convenção `Public/` e `Private/` |
| Teste de mutação | não fala | fica de fora | Ferramenta extra do Lemuria; pode voltar por proposta |

---
id: 0027
titulo: Central de conhecimento em sessão separada e sala limpa vigiada
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0027: Central de conhecimento em sessão separada e sala limpa vigiada

## Contexto

O dono mantém, só no próprio computador, uma central de conhecimento que estuda o OpenMU e o MuMain e guarda o que descobre em specs, achados e lições. O Nexum é escrito a partir dessas descrições ([ADR 0001](0001-caminho-b-sala-limpa.md)), mas nunca a partir do código do MuMain, que não tem licença. O código do OpenMU usa a licença MIT e pode ser lido.

Abrir os dois projetos na mesma sessão misturaria comandos com o mesmo nome e regras que se contradizem.

## Decisão

- **Sessões separadas.** Cada projeto é aberto na própria sessão, com as próprias skills e regras.
- **A ligação é só por leitura de arquivos.** A skill `/consultar-central` procura a resposta nas specs, nos achados, nas lições e nos documentos do OpenMU.
- Se a resposta não existir, a skill escreve a pergunta pronta para o dono levar à central. A central investiga, registra o achado e atualiza a spec.
- **Cada fatia registra no `plan.md` as fontes da central que usou**, com a base de commits. A checagem `docs` exige essa seção.

**Regras de acesso, aplicadas por um hook (`guard-central`):**

| Parte da central | Acesso |
|---|---|
| `docs/` (specs e glossário), `knowledge/` (achados), `licoes/` | leitura liberada |
| `Server/` (OpenMU, MIT) | leitura liberada |
| `Client/` (MuMain), `graphify-out/` (o grafo do código), `.claude/` e o `CLAUDE.md` da central | **bloqueado** |
| Escrita em qualquer parte | sempre bloqueada |
| Qualquer acesso pelo terminal | sempre bloqueado |

- O caminho da central fica em `.claude/central.local.json`, fora do Git.
- O repositório não depende da central. Sem ela, como na CI, a skill só avisa que ela não está disponível.
- O hook do início da sessão compara a cópia da spec de referência com a versão da central e avisa se ela mudou.

## Consequências

- O conhecimento chega ao Nexum sem o código do MuMain.
- Levar uma pergunta à central é um passo manual do dono.
- A central precisa produzir as specs antes das fatias que dependem delas. Cada marco do ROADMAP diz o que investigar antes.

## Alternativas consideradas

- **As regras do Lemuria (só lições liberadas; specs com confirmação):** o Lemuria não pode ter nada do MU; o Nexum é um cliente de MU e precisa das specs.
- **Abrir a central como pasta adicional na sessão do Nexum:** causaria os conflitos descritos no contexto.
- **Confiar só em regras escritas:** a IA poderia abrir o código do MuMain por engano; o hook impede isso.

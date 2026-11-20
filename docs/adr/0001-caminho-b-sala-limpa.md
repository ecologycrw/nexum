---
id: 0001
titulo: Caminho B, engine do zero em sala limpa
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0001: Caminho B, engine do zero em sala limpa

Fonte: [D-01 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-01--caminho-b-engine-do-zero-em-sala-limpa).

## Contexto

Havia três caminhos para um cliente moderno de MU: evoluir o MuMain por dentro (A), escrever uma engine própria (B) ou usar uma engine pronta com um módulo MU (C). O MuMain não tem arquivo de licença e é um fork de fontes do cliente original publicadas por terceiros. Tudo o que for construído dentro dele herda essa origem. O dono considera o Caminho A inviável e aceita o custo do B.

## Decisão

Caminho B, com o **processo de sala limpa**:

1. o conhecimento sobre o original é escrito nas specs e nos achados da central de conhecimento, com texto, tabelas e pseudo-código próprios;
2. o código do Nexum é escrito **a partir dessas descrições**, sem o código do MuMain aberto ao lado;
3. quando a descrição não basta, volta-se ao passo 1: investiga-se, registra-se o achado e atualiza-se a spec. Só então se implementa;
4. o código do **OpenMU** (MIT) pode ser lido e reaproveitado, com o aviso da licença.

## Consequências

- É o caminho mais caro. O custo é contido pelas fatias verticais ([ROADMAP](../ROADMAP.md)).
- A central vira **pré-requisito** da implementação: cada subsistema precisa de uma spec antes do código, e o roteiro reserva tempo para investigar.
- O OpenMU vira a principal fonte reaproveitável: XML de pacotes, documentos de pacote e um servidor local para testes.
- A regra é garantida por um hook que bloqueia a leitura do código do MuMain ([ADR 0027](0027-central-de-conhecimento-e-sala-limpa.md)).

## Alternativas consideradas

- **Caminho A, evoluir o MuMain:** herda a origem sem licença e obriga a seguir o fator por quadro, as globais e a ordem de desenho enquanto se tenta trocá-los.
- **Caminho C, Godot com extensão em C++:** daria editor, UI e render prontos, mas tiraria o controle e o aprendizado que são o objetivo; o visual clássico exigiria lutar contra o pipeline da engine.
- **Híbrido, aproveitando partes do MuMain:** o mesmo problema jurídico do Caminho A.

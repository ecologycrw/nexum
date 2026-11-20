# Plano da fatia N: Título

## Resumo para o dono

Em 2 ou 3 frases: como a fatia vai ser feita, sem jargão.

## Abordagem

Como a fatia vai ser construída, em passos lógicos.

## Alvos e módulos afetados

| Alvo do build | O que muda |
|---|---|
| `nexum_core` | ... |

## Visão de destino

Como cada peça desta fatia vai ser na engine final, e por que o que é feito agora não fecha nenhuma porta.

## Simplificações provisórias

Tudo o que é feito de forma simples de propósito. Cada item vai para `docs/TECH_DEBT.md` com a fatia ou o marco de destino.

| Item provisório | Por que é provisório | Destino |
|---|---|---|

## Decisões e dependências

- Decisões que merecem um ADR novo.
- Bibliotecas externas novas (precisam da aprovação do dono).
- Mudanças nos formatos da engine ou no contrato de rede (precisam da aprovação do dono).

## Fontes da central usadas

Seção obrigatória (sala limpa). Para cada comportamento do original que a fatia reproduz: a spec ou o achado da central, a seção, e a base de commits que ela analisou. Inclua também os documentos e os XML do OpenMU usados. Se a fatia não reproduz nenhum comportamento do original, escreva "nenhuma: a fatia não implementa comportamento do MU".

| Comportamento | Fonte na central | Base |
|---|---|---|

## Suficiência das descrições

Seção obrigatória (sala limpa). Para cada comportamento da tabela acima, confira se a descrição diz tudo o que o código e os testes vão precisar. Um item que a descrição não diz vira uma pergunta para a central, e a spec só vai para a aprovação do dono sem perguntas abertas. Se a fatia não reproduz nenhum comportamento do original, escreva "não se aplica".

### Comportamento: nome

| O que a descrição precisa dizer | O que ela diz | Fonte |
|---|---|---|
| Entradas e saídas | ... | ... |
| Ordem dos passos | ... | ... |
| Tempos (por tick, por quadro ou relógio de parede) | ... | ... |
| Limites e casos de erro | ... | ... |
| Números (tamanhos, offsets, tempos, códigos) | ... | ... |

Exemplos esperados, escritos só a partir da descrição (viram testes no `/implementar`):

| Entrada | Resultado esperado | De onde veio |
|---|---|---|
| ... | ... | calculado pela descrição, ou fornecido pela central |

Perguntas abertas para a central: nenhuma

## Riscos

- O que pode dar errado e como o plano reduz esse risco.

## Testes

| Critério | Como é provado |
|---|---|
| CA-01 | teste automático em `...` |

## Diagramas

Diagramas Mermaid, quando ajudarem a entender a fatia (ver `docs/CONVENTIONS.md`).

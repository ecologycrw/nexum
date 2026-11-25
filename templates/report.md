# Relatório da fatia N: Título

## Resumo para o dono

De 2 a 4 frases: o que foi entregue e o que dá para ver funcionando.

## Critérios de aceitação

| CA | Prova | Resultado |
|---|---|---|
| CA-01 | teste `NomeDoTeste` (etiqueta `[NNNN-CA-01]`) | passou |
| CA-02 | verificação manual: o que foi feito e o que se viu | confirmado |

## Paridade

O nível de paridade (P0 a P3) que a fatia tocou, o teste que o mede e o resultado. Se não tocou: "não se aplica".

## Prova visual

As imagens da cena de prova, os números medidos, o tempo de quadro e o que a IA e o revisor viram. Se a fatia não muda o que aparece na tela: "não se aplica".

## Sala limpa

Seção obrigatória. A checklist de sala limpa, conferida para as tarefas que implementaram comportamento do MU; o texto oficial de cada item fica no [WORKFLOW](../docs/WORKFLOW.md#sala-limpa). Se nenhuma implementou: "não se aplica: a fatia não implementa comportamento do MU".

- [ ] Escrito a partir de specs e achados da central, listados no `plan.md`, e não do código do MuMain.
- [ ] Nenhum código do MuMain aberto durante a escrita.
- [ ] Nenhum nome de variável, comentário ou estrutura copiado do MuMain.
- [ ] Constantes numéricas com a fonte citada numa spec.
- [ ] Código ou XML do OpenMU com o aviso da licença MIT.
- [ ] Nenhum asset da Webzen no commit.
- [ ] Lacunas levadas à central antes de implementar.

## O que você deve testar

Passo a passo em português simples, com os comandos prontos para copiar e o que você deve ver em cada passo.

## Desvios do plano

O que saiu diferente do `plan.md`, e por quê.

## Revisão

Achados do revisor e o que foi feito com cada um.

## Travas alteradas e testes removidos

Seção obrigatória, com as duas listas, mesmo quando vazias.

- **Travas alteradas:** os arquivos de trava que mudaram nesta fatia e a aprovação do dono para cada mudança. Se nenhum: "nenhuma".
- **Testes removidos:** os testes apagados ou enfraquecidos e o motivo de cada um. Se nenhum: "nenhum".

## Itens provisórios

- **Criados:** os itens registrados em `docs/TECH_DEBT.md` nesta fatia. Se nenhum: "nenhum".
- **Resolvidos:** os itens que saíram de `docs/TECH_DEBT.md` nesta fatia, cada um com o que o resolveu. Os que só mudaram de destino também entram aqui, com o motivo. Se nenhum: "nenhum".

## Erros e lições

Erros que a IA cometeu nesta fatia (primeira ocorrência da escada em `docs/WORKFLOW.md`).

## Propostas geradas

IDs das propostas registradas em `docs/PROPOSALS.md` durante a fatia.

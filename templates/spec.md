---
id: NNNN
titulo: Título curto da fatia
status: rascunho   # rascunho | aprovada | em-andamento | concluida
aprovada-em:
marco: M0
---

# Fatia N: Título

## Resumo para o dono

De 2 a 4 frases, sem jargão: o que muda e o que dá para ver funcionando no fim da fatia.

## Escopo

- O que esta fatia entrega.

## Fora do escopo

- O que fica de fora de propósito, e em que fatia ou marco entra (se já se sabe).

## Critérios de aceitação

Cada critério é verificável: por um teste automático com a etiqueta `[NNNN-CA-xx]` no nome ou por uma verificação manual descrita.

- CA-01: Descrição verificável.
- CA-02: Descrição verificável.

## Paridade

O nível de paridade que esta fatia toca (P0 protocolo, P1 comportamento, P2 visual clássico, P3 visual aprimorado) e como ele é medido. Se não toca: "não se aplica".

## Prova visual

Preencha se a fatia muda o que aparece na tela (ver `docs/VISUAL.md`). Se não muda, escreva "não se aplica".

- **Cena de prova:** o que aparece e de onde a câmera olha.
- **O que a imagem precisa mostrar:** o resultado esperado, de acordo com o guia visual.
- **Comparação:** com a última imagem aprovada (conteúdo próprio) ou com a referência de paridade, e a tolerância.
- **Fluidez:** o tempo de quadro esperado na cena de prova.

## Como o dono testa

Passo a passo em português simples, com os comandos prontos para copiar.

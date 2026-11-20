---
id: 0020
titulo: Conteúdo original e distribuição
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0020: Conteúdo original e distribuição

Fonte: [D-20 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-20--conteúdo-original-e-distribuição).

## Contexto

O jogo só funciona com os arquivos do original, mas esses arquivos pertencem à Webzen e não podem ser distribuídos.

## Decisão

- **Nada da Webzen no repositório:** nem modelos, nem texturas, nem sons, nem tabelas de dados, nem imagens de referência com a arte do jogo.
- O usuário aponta o `muimport` para a **própria instalação**. O cache convertido fica na máquina dele.
- O repositório tem **conteúdo de teste próprio** (malhas simples, texturas geradas, um mapa pequeno) em `Content/`, para que a CI rode sem os arquivos originais.
- As imagens de referência dos testes de paridade ficam **fora** do repositório público. Nele ficam só os parâmetros das cenas e as métricas.
- Avisos de licença: OpenMU (MIT) quando o código ou os XML dele forem usados; as dependências, em [THIRD_PARTY.md](../THIRD_PARTY.md).

## Consequências

- A checagem `structure` recusa arquivos com as extensões e os nomes dos arquivos originais (INV-17).
- O `.gitignore` exclui a pasta `cache/` do cache convertido.

## Alternativas consideradas

- **Distribuir os arquivos originais com o cliente:** proibido pela licença do jogo.

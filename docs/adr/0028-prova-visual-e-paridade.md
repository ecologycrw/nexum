---
id: 0028
titulo: Prova visual e paridade visual
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0028: Prova visual e paridade visual

Fonte: [V.2.3 da spec](../referencia/especificacao-engine-propria-caminho-b.md#v23-imagens-de-referência).

## Contexto

Build e testes verdes não dizem se a imagem está certa. No Lemuria, cada fatia que muda a tela tem uma prova visual. No Nexum, o modo Classic ainda precisa ficar próximo do original (P2), e as imagens do original têm a arte da Webzen.

## Decisão

- **O [guia visual](../VISUAL.md)** define os modos Classic e Enhanced, as cenas de paridade, o que reprova uma imagem e a fluidez.
- **Toda fatia que muda a tela tem uma prova visual:** a IA e o revisor abrem a imagem, a comparação confere, e o dono aprova. A seção "Prova visual" é obrigatória no `spec.md` e no `report.md` (pode dizer "não se aplica").
- **Paridade P2:** as cenas são capturadas rodando o MuMain como caixa preta. As imagens ficam **fora** do repositório; no repositório ficam os parâmetros das cenas e as tolerâncias.
- **Cenas de conteúdo próprio:** as imagens aprovadas pelo dono podem ficar no repositório, numa pasta que é trava.
- **A ferramenta de comparação** entra no M0, com a primeira janela, reaproveitando o leitor de PNG e a comparação por pixel do Lemuria. A comparação perceptual (SSIM) entra no M3 ([TD-0006](../TECH_DEBT.md)).

## Consequências

- O revisor tem um item obrigatório para conferir a prova visual.

## Alternativas consideradas

- **Guardar as imagens do original no repositório:** proibido pelo [ADR 0020](0020-conteudo-original-e-distribuicao.md).
- **Só revisão a olho:** não detecta mudanças pequenas.

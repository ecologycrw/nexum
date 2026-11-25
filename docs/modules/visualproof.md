# Módulo: VisualProof

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A ferramenta da prova visual ([VISUAL](../VISUAL.md)): compara a captura de uma cena com a imagem que o dono aprovou e diz se passou e, se não passou, por quê. Ela não captura nada; a captura é do próprio jogo (`MuClient --capture`). É uma ferramenta de `Tools/`, que nunca entra no build de jogador.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `visualproof_comparison` | Tools | biblioteca com a comparação e as opções; liga o `Nexum::Core` e o `Nexum::Platform` |
| `visualproof` | Tools | o programa de linha de comando |

Pasta `Tools/VisualProof/` ([ADR 0023](../adr/0023-arquivos-e-includes-em-pascalcase.md): pastas de módulo com a primeira letra maiúscula).

## Conceitos

- **Uso:** `visualproof CAPTURA.png APROVADA.png --tolerance NÍVEIS`. Termina com 0 quando aprova, 1 quando recusa e 2 quando a linha de comando está errada.
- **Tolerância** (`Tolerance`): a maior diferença aceita por canal, de 0 a 255. É obrigatória, porque cada cena declara a sua. A cena da fatia 0 usa 1 nível.
- **`CompareImages` e `CompareFiles`** (`ImageComparison.hpp`): recusam, cada caso com a sua mensagem:
  - tamanhos diferentes;
  - pixels acima da tolerância, com quantos são e o primeiro deles (a posição e as duas cores);
  - um arquivo ilegível, com o caminho;
  - uma imagem vazia ou de uma cor só, seja a captura ou a aprovada.
- **`ParseProofOptions`** (`ProofOptions.hpp`): lê a linha de comando, com exatamente duas imagens e uma tolerância, e recusa opções desconhecidas.

## Como usar

O teste do CTest `visual_proof_0000_test_card` (`tests/visual/CMakeLists.txt`) roda a captura e depois o `visualproof`. Ele apaga a captura anterior antes de começar. As imagens aprovadas ficam em `tests/visual/approved/`, uma trava em que só o dono põe imagens. Como rodar e onde fica a captura está no [DEV.md](../setup/DEV.md).

## Fontes da central

Não se aplica: a ferramenta não reproduz nada do original.

## Invariantes relevantes

- **INV-02 e INV-19:** fica em `Tools/` e nunca entra no build de jogador.
- **INV-20:** paridade se mede, não se declara. A ferramenta é a régua da prova visual.
- **INV-14:** em `Tools/`, exceções seriam permitidas, mas a ferramenta usa erros por valor, como a engine.

## Testes

Em `Tools/VisualProof/Tests/` (`visualproof_tests`): `ImageComparisonTest.cpp` (cada caso de aprovação e de recusa) e `ProofOptionsTest.cpp` (a linha de comando).

## Itens provisórios

- [TD-0016](../TECH_DEBT.md): só compara pixel a pixel; a comparação perceptual chega no M3.
- A decisão de onde a imagem aprovada é gerada a partir do M3 está na [P-0008](../PROPOSALS.md).

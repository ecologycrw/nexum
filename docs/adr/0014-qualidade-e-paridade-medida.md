---
id: 0014
titulo: Qualidade com testes, paridade medida e CI
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0014: Qualidade com testes, paridade medida e CI

Fonte: [D-14 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-14--qualidade-testes-paridade-medida-e-ci) e a [Parte V](../referencia/especificacao-engine-propria-caminho-b.md#parte-v--qualidade-e-paridade).

## Contexto

"100% de fidelidade" não é verificável. A paridade precisa de testes que digam se foi atingida, e o projeto precisa funcionar em três sistemas que a máquina de desenvolvimento não tem.

## Decisão

| Nível | O que testa | Ferramenta | Quando |
|---|---|---|---|
| Unidade | funções puras: matemática, A*, inventário, decodificadores, interpolação | doctest | todo commit, nos 3 sistemas |
| Formato | o `muimport` contra amostras reais da instalação do usuário | doctest e amostras locais | na máquina do desenvolvedor; na CI, só com o conteúdo próprio |
| Fuzzing | leitores de arquivo e decodificadores de pacote e de item | libFuzzer (Clang) | toda noite e antes de fechar cada marco |
| Protocolo | o codec contra os **vetores do OpenMU** | doctest | todo commit |
| Lógica sem rede | sistemas do jogo alimentados com eventos neutros | doctest | todo commit |
| Replay | sessões gravadas (`.mucap`) reproduzidas sem servidor | ferramenta própria | todo commit |
| Integração | cliente automatizado contra um OpenMU local | socket de automação | toda noite |
| Paridade visual | cenas fixas comparadas com imagens de referência | captura sem janela e comparação | antes de fechar marcos de render |
| Desempenho | cenas fixas medidas | Tracy e modo benchmark | toda noite; regressão acima de 10% vira alerta |

- **Sanitizers** (AddressSanitizer e UndefinedBehaviorSanitizer) no Linux, e AddressSanitizer no MSVC.
- **CI** em Windows (MSVC), Linux (Clang e GCC) e macOS (Clang, arm64), com Debug e Release.
- **Vetores de protocolo:** um programa pequeno em C#, nosso, usa a biblioteca de pacotes do OpenMU para gravar pacotes de exemplo, o resultado da cifra e itens em todos os formatos. O codec precisa chegar aos mesmos bytes.
- **Fuzzing:** zero falhas em 1 hora por alvo antes de fechar o marco que o introduz.

## Consequências

- Cada fatia diz no `spec.md` qual nível de paridade toca e como ele é medido.
- As ferramentas concretas de verificação ficam no [ADR 0025](0025-ferramentas-de-verificacao.md).

## Alternativas consideradas

- **Declarar fidelidade sem medir:** a promessa da versão original da spec, que não podia ser conferida.

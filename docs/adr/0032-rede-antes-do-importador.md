---
id: 0032
titulo: A rede antes do importador, com o M2 dividido em duas partes
status: aceita
data: 2026-11-25
substitui:
substituida-por:
---

# ADR 0032: A rede antes do importador, com o M2 dividido em duas partes

## Contexto

- A Parte VI da [spec de referência](../referencia/especificacao-engine-propria-caminho-b.md#parte-vi--roteiro-em-fatias-verticais) põe o M1 (importador e visualizador) antes do M2 (rede sem gráficos).
- O maior risco do produto é a compatibilidade com o OpenMU sem modificações. O risco 2 da Parte VII, "erros sutis de protocolo", tem probabilidade e impacto altos, e a própria spec diz que a rede vem cedo porque é onde estão os erros mais caros.
- Na retrospectiva da fatia 0, em 2026-11-25, o dono escolheu fazer o M2 antes do M1.
- A conferência da spec mostrou que o M2 tem duas metades com dependências diferentes:
  - **a rede** (o codec gerado, a cifra, a sessão até "No mundo", o ping, os eventos neutros, a gravação e o replay) não depende de nada do M1;
  - **a visão 2D da grade, com os atributos do mapa, e o clique para andar** dependem do M1. A spec pede essa visão no ImGui do modo de autoria ([ADR 0013](0013-ferramentas-de-autoria-em-imgui.md)), que chega no M1. E os atributos vêm do arquivo `EncTerrain{N}.att` da instalação, que só o importador do M1 pode ler, porque o jogo nunca abre um arquivo original (INV-08).
- A central de conhecimento já escreveu o que as duas metades mandam investigar antes: a spec de protocolo do cliente 106.3 e a spec dos atributos de terreno, ambas de 2026-11-22.

## Decisão

1. **O M2 é dividido em duas partes.** A ordem da Fase 1 passa a ser: M2 parte 1, M1, M2 parte 2, M3, M4, M5, M6 e M7.
2. **M2, parte 1: a rede sem mapa.** Traz o gerador do codec (`packetgen`), a transformação de bytes, a sessão até "No mundo", o ping, os eventos neutros, a gravação `.mucap` e o replay, e os vetores como teste. O critério exato fica na spec da fatia, mas inclui: os vetores passam, o cliente entra no mundo de um OpenMU local e continua lá sem queda, e um replay reproduz o mesmo estado final.
3. **M2, parte 2: a grade e o andar.** Vem depois do M1. Traz a visão 2D da grade no ImGui, com os atributos importados, o clique para andar com o A\* do kit e os 30 minutos andando sem queda e sem aviso de violação.
4. **O critério de pronto do M2 da spec não muda.** Ele é atingido quando a parte 2 termina.
5. **A cópia da spec de referência não muda.** O desvio fica registrado neste ADR, no [ROADMAP](../ROADMAP.md) e nos ajustes do [README da referência](../referencia/README.md#ajustes-feitos-na-adoção).

## Consequências

- O risco maior é atacado primeiro, sem esperar o importador.
- A parte 1 não tem tela própria: a prova vem dos testes, dos vetores, do log e do replay.
- O M1 não muda. A parte 2 do M2 usa o ImGui e o importador que ele traz.
- O que a spec ligava ao começo do M2 passa a começar na parte 1: os testes de lógica com eventos neutros (INV-01), os vetores do OpenMU (INV-20) e o fuzzing dos decodificadores (INV-12).
- A estimativa de velocidade que a spec faz "depois do M2" passa a valer depois das duas partes.
- O ROADMAP e a ARCHITECTURE mostram a ordem nova.

## Alternativas consideradas

- **Seguir a ordem da spec, com o M1 antes do M2:** adia o maior risco do produto para depois de um marco grande.
- **Fazer o M2 inteiro antes do M1:** o M2 teria de trazer o ImGui, o começo do importador e o formato do cache da engine, decisões do M1 tomadas sem os modelos, e ficaria do tamanho de um marco GG.

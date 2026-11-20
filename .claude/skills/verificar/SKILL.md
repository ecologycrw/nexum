---
name: verificar
description: Roda todas as verificações do Nexum (formatação, build, testes, sanitizers, build de jogador, análise estática e checagens) e mostra a saída real, com cada critério de aceitação ligado à sua prova. Use depois de cada tarefa, antes de revisar e antes de fechar uma fatia.
---

# /verificar

## Passos

Rode e mostre a saída real (resumida, mas com os números):

```bash
py scripts/dev.py verify
```

No Linux e no macOS, use `python3` no lugar de `py`.

Se houver uma fatia em andamento, monte também a tabela de critérios:

| CA | Prova | Situação |
|---|---|---|
| CA-01 | teste com a etiqueta `[NNNN-CA-01]` encontrado em `Engine/Core/Tests/...` | passou |
| CA-02 | verificação manual prevista no plano | pendente |

Para achar as provas, procure `[NNNN-CA-` nos arquivos `.cpp` das pastas `Tests/` de cada módulo e de `tests/`.

Se a fatia toca um nível de paridade, mostre o resultado do teste que o mede. Se a fatia muda o que aparece na tela, faça também a prova visual de `docs/VISUAL.md`: gere a imagem da cena de prova, abra o arquivo, diga o que vê e mostre a comparação e o tempo de quadro.

## Resultado

- **Verde** só se o comando passou nesta execução. Numa fatia que muda a tela, a imagem também precisa ter sido gerada e conferida nesta execução.
- Linux, GCC e macOS só são conferidos pela CI. Sem remoto (modo local), diga isso ao dono.
- Se algo falhar, mostre o erro, explique em linguagem simples e corrija (dentro do escopo da tarefa).
- Nunca diga que passou sem ter rodado.

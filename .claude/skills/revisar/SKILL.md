---
name: revisar
description: Faz a revisão independente da fatia do Nexum com o subagente revisor, que trabalha com contexto limpo e confere também a sala limpa e a linha vermelha, e corrige os achados graves. Use quando todas as tarefas da fatia estiverem concluídas.
---

# /revisar

## Pré-condições

- Todas as tarefas do `tasks.md` estão `[x]`.
- `/verificar` está verde.

## Passos

1. **Chame o subagente `revisor`** com a ferramenta de agentes. Passe:
   - o número e o nome da fatia;
   - os caminhos da spec e do plano;
   - os caminhos das imagens da cena de prova, se a fatia muda o que aparece na tela;
   - a instrução para revisar o diff `git diff main...HEAD` seguindo a lista obrigatória dele.

   Não resuma o código para ele: ele precisa ler sozinho, com contexto limpo.
2. **Avalie cada achado.** Confira no código se o achado procede; o revisor também pode errar.
3. **Corrija os críticos e importantes que procedem,** dentro do escopo e com testes. Um achado de sala limpa (comportamento sem fonte, constante sem origem) é sempre importante: busque a fonte com `/consultar-central` ou leve a pergunta à central. Rode `/verificar` de novo. Commits `fix(NNNN): ...` e, se houver remoto, `git push`.
4. **Achados menores ou fora do escopo** vão para `/propor` ou para o report.
5. **Registre na seção "Revisão" do `tasks.md`:** cada achado, se procedia e o que foi feito. O `/fechar` usa esse registro.
6. Diga ao dono que a próxima etapa é `/fechar`.

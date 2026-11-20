---
id: 0024
titulo: Fluxo Git com merge só pelo dono e começo em modo local
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0024: Fluxo Git com merge só pelo dono e começo em modo local

## Contexto

A IA precisa salvar o trabalho sozinha, mas a versão oficial só pode mudar com a aprovação do dono. O GitHub não deixa ninguém aprovar o próprio PR, e uma IA logada na conta do dono teria os poderes dele. O dono escolheu começar o Nexum com o Git só neste computador e publicar no GitHub depois.

## Decisão

**Fluxo com o GitHub (o destino):**

- **Conta de máquina** só para a IA, com permissão *Write*. Ela faz os commits, envia as branches e abre os PRs. O dono aprova e faz o merge com a própria conta, pelo navegador.
- A identidade dos commits é configurada **só neste repositório**, com a assinatura por chave SSH da conta da IA. Os commits trazem a linha `Co-Authored-By` do Claude.
- **Uma branch por fatia** (`fatia/NNNN-nome`). Um commit por tarefa, se o build e os testes passarem. O `/fechar` abre o PR, e o dono faz o **squash merge**.
- A `main` é protegida por um ruleset do GitHub ([ruleset-main.json](../setup/ruleset-main.json)): PR com 1 aprovação do code owner, CI verde nos 3 sistemas, sem force push, sem exclusão, commits assinados, sem exceções.
- Proibido para a IA: force push, push para a `main`, `--no-verify`, `git config`, `gh pr merge`, `gh auth` e `gh api`. Isso vale nas permissões, nos hooks e no GitHub.

**Modo local (o começo):**

- Enquanto não houver um repositório remoto, as skills pulam `git pull`, `git push` e o `gh`, e a prova é a saída do `verify` mostrada no chat.
- O merge é decisão do dono. Com um "sim" explícito dele, a IA roda `git merge --ff-only`. As permissões do Claude Code pedem confirmação antes de qualquer `git merge`, e o hook `guard-bash` recusa um merge sem `--ff-only`.
- Os commits da preparação usam a identidade global deste computador, sem assinatura, e o histórico local fica commit por commit, sem squash. Isso fica registrado em [TD-0008](../TECH_DEBT.md).
- **A fatia 0 só fecha com o GitHub publicado**, porque o M0 exige a CI verde nos 3 sistemas.

O passo a passo está em [docs/setup/GITHUB.md](../setup/GITHUB.md).

## Consequências

- Mesmo que todas as travas locais falhem, a `main` publicada continua protegida no GitHub.
- No modo local, a proteção depende só das travas deste computador.

## Alternativas consideradas

- **Commits na conta do dono:** o dono não conseguiria aprovar os PRs, e a IA teria poderes de administrador.
- **Reaproveitar a conta de máquina do Lemuria:** misturaria o histórico e as permissões dos dois projetos. O Lemuria rejeitou a mesma ideia em relação a outro projeto.
- **Publicar no GitHub antes da preparação:** foi o caminho do Lemuria; o dono preferiu começar local.

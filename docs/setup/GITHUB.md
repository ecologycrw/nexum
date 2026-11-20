# Configuração do GitHub

Passo a passo para publicar o Nexum no GitHub com as proteções do projeto ([ADR 0024](../adr/0024-fluxo-git.md)). Os passos marcados **(dono)** são feitos pelo dono do produto; os marcados **(IA)**, pela IA.

- Repositório: `ecologycrw/nexum` (público).
- Conta do dono: `ecologycrw`, usada **só pelo navegador**.
- Conta da IA: `nexum-ci-dev` (nome sugerido), usada pelo `gh` e pelo Git neste PC.

O Nexum começou com o Git só local: a preparação já tem commits, feitos com a identidade global deste PC e sem assinatura. Isso não atrapalha, porque os PRs entram na `main` por squash merge, e o GitHub assina o commit do squash.

Se você escolher outro nome para a conta da IA, troque `nexum-ci-dev` nos comandos.

> **Atenção:** o `gh` deste PC hoje está logado na conta de máquina do Lemuria. Os passos 2 e 3 trocam o login. Depois, para voltar a trabalhar no Lemuria, você precisa trocar de novo (ou usar o `gh auth switch`).

---

## 1. Criar a conta da IA (dono, navegador)

1. Abra uma **janela anônima** e entre em github.com/signup.
2. Crie a conta `nexum-ci-dev` com um e-mail que você controle. Ele precisa ser diferente do e-mail da sua conta pessoal e do e-mail da conta do Lemuria.
3. Confirme o e-mail.
4. Ative a verificação em duas etapas em **Settings → Password and authentication**.
5. Deixe essa janela anônima aberta: você vai usá-la no passo 2.

A IA usa uma conta separada para não ter os seus poderes. Ela envia código e abre PRs, mas não consegue aprovar nem fazer merge.

## 2. Entrar na conta da IA neste PC (dono, terminal)

```bash
gh auth login --hostname github.com --git-protocol https --web --scopes admin:ssh_signing_key
```

O comando mostra um código. Abra o link **na janela anônima logada como `nexum-ci-dev`**, cole o código e autorize. Depois:

```bash
gh auth setup-git
```

```bash
gh auth status
```

O `gh auth status` deve mostrar `nexum-ci-dev` como a conta ativa.

## 3. Criar a chave de assinatura dos commits (dono, terminal)

Crie a chave. Quando ela pedir uma senha (*passphrase*), aperte **Enter** duas vezes, sem digitar nada:

```bash
ssh-keygen -t ed25519 -C "nexum-ci-dev signing" -f "$HOME/.nexum_signing_key"
```

Cadastre a chave pública na conta da IA:

```bash
gh ssh-key add "$HOME/.nexum_signing_key.pub" --type signing --title "Nexum signing"
```

A chave privada (`.nexum_signing_key`, sem `.pub`) nunca sai do seu PC e nunca entra no repositório.

## 4. Configurar o Git deste repositório (dono, terminal)

Rode **dentro da pasta do Nexum**, um comando por vez. Estas configurações valem só para o Nexum.

```bash
git config user.name "nexum-ci-dev"
```

```bash
git config user.email "$(gh api user --jq '.id')+$(gh api user --jq '.login')@users.noreply.github.com"
```

```bash
git config gpg.format ssh
```

```bash
git config user.signingkey "$HOME/.nexum_signing_key.pub"
```

```bash
git config commit.gpgsign true
```

```bash
git config core.hooksPath .githooks
```

## 5. Criar o repositório (dono, navegador)

1. Em github.com, logado na sua conta pessoal: **+ → New repository**.
2. Nome `nexum`, **Public**. **Não** marque README, `.gitignore` nem licença: o repositório precisa nascer vazio.
3. **Create repository**.

## 6. Convidar a conta da IA (dono e conta da IA, navegador)

1. No repositório: **Settings → Collaborators → Add people** → `nexum-ci-dev` → papel **Write**.
2. Na janela anônima, logada como `nexum-ci-dev`, aceite o convite (chega por e-mail ou em github.com/notifications).

## 7. Configurações do repositório (dono, navegador)

Em **Settings**:

| Onde | O que fazer |
|---|---|
| General → Pull Requests | Deixar marcado **só** "Allow squash merging". Marcar "Automatically delete head branches" |
| Actions → General → Workflow permissions | **Read repository contents and packages permissions** |
| Actions → General → Fork pull request workflows | **Require approval for all external contributors** |
| Advanced Security (ou Code security) | Ativar **Secret Protection** e **Push protection** |
| Advanced Security → Private vulnerability reporting | **Enable** |
| Moderation options → Interaction limits | **Limit to repository collaborators** (o máximo de tempo disponível) |

## 8. Enviar a main pela primeira vez (dono, terminal)

> **A ordem importa: envie a `main` antes de importar a regra do passo 9.** Com a regra ativa, o GitHub recusa este envio (erro `GH013`): ela exige a CI, que ainda não rodou, e exige commits assinados, e o commit inicial foi feito antes da chave. Se a regra já tiver sido importada, desligue-a por um instante (**Settings → Rules → Rulesets → Proteger a main → Enforcement status: Disabled**), envie a `main` e ligue a regra de novo (**Active**). Foi o que aconteceu na publicação do Nexum.

A IA adiciona o endereço do repositório (`git remote add origin ...`). Depois, você envia a `main`, uma única vez:

```bash
git switch main
```

```bash
git push -u origin main
```

```bash
git switch preparacao/ambiente
```

O último comando volta a pasta para a branch da preparação. Depois disso, ninguém envia nada direto para a `main`: tudo entra por PR.

## 9. Proteger a main (dono, navegador)

**Settings → Rules → Rulesets → New ruleset → Import a ruleset** e escolha o arquivo [`docs/setup/ruleset-main.json`](ruleset-main.json) deste projeto. Confira e clique em **Create**.

A regra exige na `main`:

- PR com 1 aprovação, do code owner, de alguém que não fez o último envio;
- aprovações antigas descartadas a cada novo envio;
- CI verde em Windows, Linux (Clang e GCC) e macOS, build de jogador nos três sistemas, análise estática aprovada e diagramas válidos;
- só squash merge, histórico linear e commits assinados;
- nada de force push nem de apagar a branch;
- nenhuma exceção, nem para o dono.

## 10. PR #1 (IA)

A IA envia a branch `preparacao/ambiente`, abre o PR #1 e confere:

- a CI rodando nos três sistemas, e o que ela acusar de Linux e macOS, que nunca foram testados aqui ([TD-0002](../TECH_DEBT.md));
- o PR bloqueado até a sua aprovação.

## 11. Revisar e fazer o merge (dono, navegador)

No PR: aba **Files changed → Review changes → Approve**. Depois, **Squash and merge**.

## Renovação

O login do `gh` da conta da IA não expira sozinho. Se algum dia ele sair, repita o passo 2.

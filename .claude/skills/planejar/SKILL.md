---
name: planejar
description: Escolhe a próxima fatia do Nexum, confere na central o que o marco manda investigar antes, e escreve a spec e o plano para o dono aprovar. Use quando não houver fatia em andamento e o dono quiser começar a próxima etapa.
---

# /planejar

Transforma o próximo item do roadmap numa spec e num plano aprovados. **Não implementa nada.**

## Pré-condições

- Nenhuma fatia em andamento: o `spec-atual` do `docs/STATUS.md` está vazio. Se não estiver, pare e explique ao dono.
- A árvore de trabalho está limpa (`git status --short` vazio). A única exceção são propostas novas em `docs/PROPOSALS.md` registradas entre fatias: elas entram no primeiro commit desta fatia.

## Passos

1. **Contexto.** Leia `docs/ROADMAP.md`, `docs/STATUS.md`, as propostas abertas em `docs/PROPOSALS.md`, `docs/TECH_DEBT.md`, o report da última fatia e as seções de `docs/ARCHITECTURE.md` que a fatia toca. Leia também, na cópia da spec em `docs/referencia/`, a seção do marco na Parte VI, os subsistemas da Parte IV que ele toca e as lacunas da Parte VIII.
2. **Escolha da fatia.** É a próxima de "Próximas" no ROADMAP, a menos que a última retrospectiva tenha decidido outra. Um marco grande pode virar várias fatias: proponha um pedaço que termine em algo que roda e se mede. Se ela depender de algo que ainda não existe, proponha antes uma fatia de fundação e pergunte ao dono.
3. **Investigar antes (sala limpa).** Para cada comportamento do original que a fatia vai reproduzir, e para cada item da coluna "Investigar antes" do marco, use `/consultar-central`.
   - Se tudo estiver descrito nas specs e nos achados da central, siga.
   - Se faltar algo, **pare**: entregue ao dono as perguntas prontas para a central, registre-as em "Bloqueios" no `docs/STATUS.md` e não escreva a spec ainda. O código nunca nasce sem a descrição.
4. **Branch.** Rode `git switch main`. Se houver remoto (`git remote`), rode `git pull --ff-only`. Depois, `git switch -c fatia/NNNN-nome`. A fatia N usa o número N com quatro dígitos; o nome é curto, em kebab-case.
5. **Spec.** Crie `specs/NNNN-nome/spec.md` a partir de `templates/spec.md`, com `status: rascunho` e o `marco`. Os critérios de aceitação são verificáveis: cada um é provado por um teste automático ou por uma verificação manual descrita. Preencha "Paridade" (o nível P0 a P3 que a fatia toca, ou "não se aplica"). Escreva "Como o dono testa" com passos que ele consiga seguir sozinho. Se a fatia mudar o que aparece na tela, leia `docs/VISUAL.md` e preencha a seção "Prova visual"; se não mudar, escreva "não se aplica".
6. **Plano.** Crie `plan.md` a partir de `templates/plan.md`. Preencha com cuidado a "Visão de destino" e as "Simplificações provisórias" (ver "interface pensa no destino, implementação no agora" em `docs/CONVENTIONS.md`). Liste bibliotecas novas, ADRs necessários e mudanças nos formatos da engine ou no contrato de rede. **Preencha "Fontes da central usadas"** com cada spec, achado e documento do OpenMU, e a base de commits; a checagem `docs` exige essa seção.
7. **Checagem.** Rode `py scripts/checks/run.py docs terms`.
8. **Apresente ao dono**, em português simples:
   - o que a fatia entrega e como testar;
   - os critérios de aceitação em linguagem simples;
   - o que fica de fora;
   - as fontes da central que sustentam a fatia;
   - o que precisa da decisão do dono (bibliotecas novas, ADRs, formatos da engine, contrato de rede);
   - que, ao aprovar, a fatia passa para "Agora" no ROADMAP.

   Pergunte se o dono aprova.
9. **Só depois de um "sim" explícito:**
   - mude o status da spec para `aprovada` e preencha `aprovada-em`;
   - no `docs/ROADMAP.md`, ponha a fatia em "Agora", com o link da spec, e a anterior em "Concluídas", se ainda não estiver lá. A aprovação da spec vale para essa mudança; qualquer outra mudança no ROADMAP continua precisando de aprovação própria;
   - atualize o STATUS (`spec-atual: NNNN`, `branch`, "Agora", "Próxima ação" e uma linha no "Histórico": spec aprovada);
   - commit `docs(NNNN): plan slice N`. Se houver remoto, `git push -u origin fatia/NNNN-nome`.

Se o dono pedir mudanças, ajuste a spec e o plano e volte ao passo 8.

## Nunca

- Começar a implementar.
- Escrever a spec de um comportamento do original sem a descrição dele na central.
- Mudar o ROADMAP ou a VISION sem a aprovação do dono (pôr a fatia aprovada em "Agora" já está coberto pela aprovação da spec).

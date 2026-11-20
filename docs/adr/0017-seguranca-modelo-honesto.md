---
id: 0017
titulo: Segurança com um modelo honesto
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0017: Segurança com um modelo honesto

Fonte: [D-17 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-17--segurança-um-modelo-honesto).

## Contexto

Um cliente modificado pula qualquer verificação feita no próprio cliente. A cifra do protocolo do MU usa chaves fixas e públicas, então é ofuscação, não segurança.

## Decisão

- **O servidor é a única defesa.** Ele já valida dano, posição, drop e alvos. Mexer nos arquivos locais não dá vantagem que o servidor aceite.
- **Hash de assets no cliente não é segurança.** Pode existir para detectar arquivo corrompido, não para impedir trapaça.
- **O protocolo não tem TLS**, e a documentação para o usuário diz isso ([SECURITY.md](../../SECURITY.md)).
- **Senha:** nunca gravada em disco. Para a reconexão automática, fica em memória só se o usuário aceitar, e é apagada ao sair.
- **Dados do servidor são entradas não confiáveis.** Todo decodificador confere limites e passa por fuzzing. Um servidor com defeito ou malicioso não pode derrubar o cliente (INV-12).
- **Build de jogador:** sem modo de autoria, sem socket de automação, sem carregar DLL de terceiros (INV-19).

## Consequências

- O cliente não promete o que não pode garantir.

## Alternativas consideradas

- **Anti-trapaça no cliente e verificação de assets como proteção:** dariam uma falsa garantia.

# Segurança

## Como relatar uma vulnerabilidade

**Não abra uma issue pública.** Use o relato privado do GitHub: aba **Security → Report a vulnerability** deste repositório.

Descreva o que você encontrou, como reproduzir e o impacto que você imagina. A resposta vem assim que possível.

## Escopo

O projeto está em preparação e ainda não tem versão publicada. Mesmo assim, relatos sobre o código do repositório são bem-vindos.

## O que o cliente não promete

O modelo de segurança é honesto ([ADR 0017](docs/adr/0017-seguranca-modelo-honesto.md)):

- **O servidor é a única defesa.** O cliente não tem anti-trapaça, e mexer nos arquivos locais não dá vantagem que o servidor aceite.
- **O protocolo do OpenMU não usa TLS.** A cifra dele é uma ofuscação com chaves públicas, e a senha trafega sem criptografia de verdade.
- **A senha nunca é gravada em disco.** Para a reconexão automática, ela fica só na memória, se o usuário aceitar, e é apagada ao sair.
- **Dados do servidor são tratados como não confiáveis.** Um servidor com defeito ou malicioso não pode derrubar o cliente.

---

*To report a vulnerability, use GitHub's private vulnerability reporting (Security → Report a vulnerability). Please do not open a public issue.*

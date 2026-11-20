---
id: 0005
titulo: Tick fixo de 25 Hz com desenho interpolado
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0005: Tick fixo de 25 Hz com desenho interpolado

Fonte: [D-05 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-05--modelo-de-tempo-tick-fixo-de-25-hz-com-interpolação).

## Contexto

O cliente original rodava travado a 25 quadros por segundo, e toda a lógica de referência (velocidades de animação, rotações, contadores) foi calibrada para esse ritmo. O cliente moderno destravou o FPS com um fator por quadro, que exige cerca de 15 regras de disciplina e preserva a média dos sorteios, mas não a distribuição.

## Decisão

1. **A simulação anda em ticks fixos de 40 ms (25 Hz).** Tudo o que o original fazia "por quadro" é feito "por tick", sem fator.
2. **O desenho roda no ritmo do monitor** e mostra o estado interpolado entre o tick anterior e o atual, com um α de 0 a 1: transformações, o tempo das animações (o esqueleto é avaliado no desenho) e as posições de partículas e rastros.
3. **Entrada e câmera são processadas a cada quadro.** Comandos de jogo viram pedidos numa fila que o próximo tick executa, com retorno visual no mesmo quadro.
4. **Rede:** os pacotes recebidos são retirados a cada quadro e aplicados no início do próximo tick, em ordem ([ADR 0010](0010-rede-adaptador-em-camadas.md)).
5. **Recuperação de atraso:** no máximo 5 ticks por quadro. O excesso é descartado, e comandos nunca são repetidos para "recuperar" ticks.
6. **Relógios de parede** (ping, prazos, reconexão) usam o relógio monotônico, independente de quadros e ticks.
7. **Pausa:** a simulação pode parar (no editor e nos testes), e o desenho continua com α fixo.

## Consequências

- Cada componente que se move guarda o estado anterior e o atual.
- Cada spec de efeito ou de animação diz explicitamente o que é "por tick".
- Relógios do sistema só aparecem em `Engine/Platform` e `Engine/Time`, e a checagem `cpp-rules` confere (INV-05).

## Alternativas consideradas

- **Fator por quadro:** mais regras e erros sutis de distribuição.
- **Passo variável:** não reproduz contadores inteiros nem sorteios por tick.
- **50 Hz:** exigiria reescalar toda a lógica de referência.

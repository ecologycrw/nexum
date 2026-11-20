---
id: 0006
titulo: Modelo de threads
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0006: Modelo de threads

Fonte: [D-06 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-06--modelo-de-threads).

## Contexto

Uma thread de simulação separada complica a depuração e a reprodução, e só se paga com uma medição. O quadro, por outro lado, nunca pode esperar por disco ou rede.

## Decisão

| Thread | Faz | Não faz |
|---|---|---|
| Principal | eventos do SDL, ticks, desenho e envio à GPU, UI, comandos de áudio | bloquear em disco ou rede |
| Rede (uma por conexão) | ler e escrever no socket, enquadrar, cifrar e decifrar, contar bytes | tocar no estado do jogo |
| Workers (N = núcleos − 1, de 1 a 8) | ler arquivos, descompactar, decodificar texturas, montar malhas; avaliar poses em paralelo com muitos personagens | enviar dados para a GPU |
| Áudio (interna do miniaudio) | mixagem | — |

- Rede → principal: uma fila de um produtor e um consumidor, com mensagens já decifradas.
- Principal → rede: uma fila **única** de saída. A cifra e o contador são aplicados só na thread de rede.
- Workers: uma fila de tarefas e uma de conclusões, conferida a cada quadro.
- Uma fila com mutex é aceitável; uma fila sem trava só com uma medição de disputa.
- **Sem thread de simulação**, até que uma medição mostre o tick passando de 8 ms de forma recorrente.

## Consequências

- Simples de depurar e de reproduzir. Um carregamento nunca trava a janela (INV-06).

## Alternativas consideradas

- **Simulação na própria thread desde o início:** contradiz o laço único e exige um retrato imutável por tick sem motivo medido.

---
id: 0010
titulo: Rede com adaptador em camadas, codec gerado e dialeto 106.3
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0010: Rede com adaptador em camadas, codec gerado e dialeto 106.3

Fonte: [D-10 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-10--rede-adaptador-em-camadas-codec-gerado-dialeto-1063) e o contrato completo em [I.13](../referencia/especificacao-engine-propria-caminho-b.md#i13-rede-o-contrato-com-o-openmu).

## Contexto

A rede concentra os erros mais caros: um contador errado fecha a conexão sem ressincronizar, e o OpenMU escolhe o formato dos pacotes pela versão que o cliente manda. O alvo é o OpenMU **sem modificações**.

## Decisão

1. **Camadas do adaptador de protocolo:**

| Camada | Onde fica | Faz |
|---|---|---|
| Transporte | Engine, na thread de rede | socket TCP (Asio standalone), filas, métricas |
| Transformação de bytes | interface na Engine; implementação no módulo MU | enquadramento C1–C4, Xor32 na saída, SimpleModulus em C3 e C4 com o contador, tudo na thread de rede |
| Sessão | módulo MU | máquina de estados (lista de servidores, login, personagem, no mundo, trocando de mapa, reconectando) |
| Tradução | módulo MU | pacote ↔ **evento neutro** do jogo |

2. **Codec gerado** por uma ferramenta própria (`packetgen`), a partir dos XML do OpenMU (MIT), nas duas direções. Ficam à mão o item estendido, os campos binários de aparência e os códigos que mudam com o idioma.
3. **Dialeto explícito:** o Nexum se apresenta como **cliente 106.3** e implementa os formatos Extended. A versão fica numa configuração.
4. **Ordem:** os eventos recebidos são aplicados um a um, na ordem de chegada, nunca agrupados.
5. **Época:** um contador local aumenta a cada conexão e a cada troca de mapa; eventos de época velha são descartados.
6. **Saída única:** todos os pacotes de saída, inclusive o ping, passam pela mesma fila.
7. **Ping** a cada 20 s pelo relógio de parede.
8. **Aviso de pronto** enviado depois de toda troca de mapa, quando o mapa novo estiver pronto.
9. **Ritmo de ataque:** o cliente nunca envia ataques mais rápido do que a velocidade de ataque permite.

## Consequências

- O gerador de código é uma peça do M2, e os vetores gerados pelo próprio OpenMU validam o codec (P0).
- A Engine só vê bytes; o formato das mensagens é todo do módulo MU (INV-03, INV-09).

## Alternativas consideradas

- **Reusar a biblioteca .NET de pacotes do OpenMU por NativeAOT:** juridicamente possível (MIT), mas traz o runtime .NET para dentro de um projeto C++. Fica como plano de reserva se o gerador atrasar.

---
id: 0002
titulo: Três camadas, engine, kit do gênero e módulo MU
status: aceita
data: 2026-11-20
substitui:
substituida-por:
---

# ADR 0002: Três camadas, engine, kit do gênero e módulo MU

Fonte: [D-02 da spec](../referencia/especificacao-engine-propria-caminho-b.md#d-02--três-camadas-engine-kit-do-gênero-módulo-mu).

## Contexto

Uma engine "que nunca sabe o que é MU" não pode ter, no núcleo, o inventário com o tamanho dos baús, o cursor com o balão de NPC, as flags de zona segura ou as constantes do mapa. Ao mesmo tempo, uma engine "folha em branco" obrigaria o módulo MU a reescrever tudo o que todo RPG de ação em grade precisa.

## Decisão

Três camadas, e a dependência só aponta para baixo:

| Camada | Pasta | Sabe | Não sabe |
|---|---|---|---|
| Engine (Nexum) | `Engine/` | janela, GPU, recursos, entidades, animação, VFX, áudio, bytes de rede, host de UI, diagnóstico | grade, inventário, MU |
| Kit do gênero (Nexum GridRpg) | `Kit/GridRpg/` | grade com camadas de atributos, coordenada dupla, A*, passos com predição e correção, câmera aérea, grade de luz, inventário em grade, texto flutuante, cursor contextual, minimapa | o que cada flag ou cursor significa no MU |
| Módulo MU | `Games/MuOnline/` | tudo do MU e do OpenMU | — |

- **Significados injetados:** o módulo MU registra as regras ("o bit 0x04 bloqueia o andar", "este inventário tem 8×8 células"), e o kit as executa sem saber de onde vêm.
- **Regra vigiada:** `Engine` não inclui nada de `Kit` nem de `Games`; `Kit` não inclui nada de `Games`; `Tools` pode usar as três. O CMake só expõe as pastas `Public/`, a checagem `architecture` recusa `#include` e ligações proibidos, e a checagem `terms` recusa nomes do MU em `Engine/` e `Kit/` (a "linha vermelha").
- Um "outro jogo" de teste, em `tests/architecture/OtherGame`, usa só a Engine e o Kit e prova que eles funcionam sem o módulo MU.

## Consequências

- O kit pode servir a outro jogo do mesmo gênero.
- Cada sistema do kit precisa de uma interface de configuração, o que custa trabalho.

## Alternativas consideradas

- **Duas camadas:** obriga a escolher entre uma engine vazia e uma engine com MU dentro.
- **Monólito:** é o que o original herdou e o que se quer evitar.

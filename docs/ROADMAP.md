# Roteiro

> Muda só com a aprovação do dono. A retrospectiva de cada fatia recomenda a próxima, que passa de "Próximas" para "Agora" quando o dono aprova a spec dela no `/planejar`. A fonte é a Parte VI da [especificação de referência](referencia/especificacao-engine-propria-caminho-b.md#parte-vi--roteiro-em-fatias-verticais).

**Sem datas.** O tamanho de cada marco é relativo: P (pequeno), M (médio), G (grande), GG (muito grande). Depois do M2, a velocidade real permite estimar o resto.

**Fatias e marcos.** Um marco M ou P costuma ser uma fatia só. Um marco G ou GG vira duas ou mais fatias, decididas na retrospectiva da fatia anterior. Cada fatia termina em algo que roda e se mede.

**Investigar antes.** A sala limpa exige a spec antes do código. A coluna "Investigar antes" diz o que precisa existir na central antes do marco; o `/planejar` confere e, se faltar, escreve a pergunta para o dono levar à central.

## Concluídas

- **Preparação.** Documentos e ADRs a partir da spec do Caminho B, a cópia da spec, o esqueleto do código com as três camadas, as checagens automáticas (camadas, linha vermelha, regras de C++, docs e estrutura), os hooks do Git, a CI, verde nos 3 sistemas desde o PR #2, e o ambiente de IA. Parte do M0 já veio daqui: CMake com presets e vcpkg, módulos com `Public/` e `Private/`, os testes de fronteira e de palavras proibidas, `Result`, doctest, as convenções e os primeiros ADRs.

## Agora

- **Fatia 0, Fundação** (marco M0, tamanho M): [spec](../specs/0000-fundacao/spec.md). O `MuClient` abre uma janela e desenha um cartão de teste pela placa de vídeo, com o laço de ticks, as peças de base, o profiler e a ferramenta de prova visual, e a CI fica verde nos 3 sistemas.

## Próximas

### Fase 0 e Fase 1: do zero a jogável

| Marco | Tamanho | O que entrega, do ponto de vista de quem testa | Investigar antes (central) |
|---|---|---|---|
| **M1 Importador e visualizador** | G | Qualquer modelo do jogo animando a partir do cache convertido, no modo de autoria. O `muimport` converte a instalação inteira e exporta glTF. 1 hora de fuzzing sem falha | resolução de nome de textura, "script" de textura no nome, ordem dos eixos de Euler e lateralidade, formatos das tabelas de dados e dos sons (agente `formatos`) |
| **M2 Rede sem gráficos** | G | Entrar no OpenMU e "ver" o mundo numa visão 2D de depuração: 100% dos vetores do OpenMU passam, 30 minutos logado andando sem queda, replay com o mesmo estado final | spec da cifra (Xor32, SimpleModulus, contador), da sessão e do item estendido, com vetores; qual porta cabe a cada tipo de cliente (agente `protocolo`) |
| **M3 Mundo 3D estático** | G | Lorencia em 3D, igual ao original no modo Classic: a cena passa na comparação de imagem e o picking acerta a célula em 100 pontos de teste | perfil de luz por mapa, névoa e céu (agente `render`) |
| **M4 Personagens e movimento online** | GG | Andar por Lorencia conectado, vendo outros jogadores e monstros; trocar de mapa 50 vezes sem vazar nada | velocidades de animação por action e classe, sockets dos monstros, ritmo do andar com itens de velocidade (agentes `render` e `gameplay`) |
| **M5 Combate e VFX base** | G | Matar monstros e pegar o que cai: 10 minutos de caça sem nenhum ataque descartado pelo servidor | efeitos do golpe básico e das primeiras skills, números de dano, morte e drop (agentes `render` e `gameplay`) |
| **M6 UI núcleo** | G | Jogar sem a interface de depuração: criar personagem, entrar, caçar, organizar o inventário e conversar só pela UI | layout de cada janela, textos e ícones (agente `interface`) |
| **M7 Fechamento da Fase 1** | M | Áudio de paridade, configurações, reconexão, relatório de falhas, medição completa dos orçamentos e build de jogador limpo | — |

### ★ Ponto de decisão

Antes da Fase 2, um ADR novo responde por escrito:

1. A arquitetura aguentou? Qual decisão precisa mudar?
2. Qual é a velocidade real de entrega, e o que ela diz sobre a Fase 2?
3. A paridade P0–P2 foi atingida? Onde não foi, por quê?
4. Seguir com o conteúdo do MU, ou usar a engine e o kit para outro jogo do gênero?

## Depois

Fase 2, conteúdo. A ordem é revista no ponto de decisão.

- **M8 Itens (G):** aparência completa de itens, pilhas de reflexo por nível, asas, brilho de excelentes, conjuntos.
- **M9 Skills (GG):** skills e efeitos por classe, o catálogo por prioridade de uso.
- **M10 Buffs, tecido, pets, montarias, fauna e clima (GG).**
- **M11 Eventos (GG):** Blood Castle, Devil Square, Chaos Castle, Kalima, Kanturu, Crywolf, Castle Siege e os demais.
- **M12 Social e economia (GG):** party, guild, amigos, chat, troca, loja pessoal, baú, NPCs de loja, mix.
- **M13 Helper, Master Tree e quests (G).**
- **M14 Enhanced Look (G):** os 7 recursos do Enhanced do MuMain (filtragem de textura, iluminação aprimorada, luzes dinâmicas, luz do sol com nuvens, light mood, pós-processamento com bloom, LUT e AO, antialiasing), os ambientes por mapa e os ajustes do jogador. À parte, a opção "Áudio 3D". Investigar antes: as fórmulas e os valores do Enhanced (agentes `render` e `interface`) e, de novo, o defeito do D3D12.
- **M15 Autoria completa e mods (G):** editor de mapa e de dados, mods com glTF.

Quando uma fatia depende de algo que ainda não existe, uma fatia de fundação técnica entra antes dela.

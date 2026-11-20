# Propostas

Fila única de tudo o que surgiu fora da tarefa atual: ideias, problemas, regras que estão faltando e lições aprendidas. A fila é processada na `/retrospectiva`, e cada entrada vira fatia, decisão, regra ou trava, ou é descartada. Ver [WORKFLOW.md](WORKFLOW.md#propostas-como-as-necessidades-aparecem).

Formato de cada entrada (o modelo completo fica em `templates/proposal.md`):

```markdown
### P-0001: título curto
- Origem: fatia e tarefa, feedback do dono, medição ou checagem
- Tipo: fatia | regra | trava | skill | agente | doc | decisão | bug
- Evidência: o que mostra que isso é necessário
- Prioridade sugerida: alta | média | baixa
- Status: aberta
```

## Abertas

### P-0001: separar os jogos em repositórios próprios no ponto de decisão do M7
- Origem: feedback do dono, depois do merge da preparação
- Tipo: decisão
- Evidência: o dono perguntou "Games deveria ficar onde está por enquanto e no futuro fazer um Documents/Nexum Projects ?" e, depois da explicação, pediu "Sim, registre". Hoje a spec põe as três camadas num repositório só ([ADR 0002](adr/0002-tres-camadas.md)), o que permite mudar a engine e o módulo MU na mesma fatia enquanto a API da engine ainda muda muito. A ideia é revisar isso no ponto de decisão depois do M7 (pergunta 4 do [ROADMAP](ROADMAP.md#-ponto-de-decisão)): o Nexum (Engine, Kit e Tools) viraria uma biblioteca com versão, e cada jogo teria o próprio repositório, por exemplo numa pasta `Nexum Projects`. A separação já é barata, porque a Engine e o Kit nunca incluem nada de `Games/` e o CMake só expõe as pastas `Public/`. Exigiria um ADR com as regras de versão, a forma de o jogo instalar a engine e a CI de cada lado
- Prioridade sugerida: baixa (decidir no ponto de decisão do M7)
- Status: aberta

## Processadas

### P-0002: conferir se a descrição da central basta antes de aprovar a spec
- Origem: feedback do dono, depois do merge da preparação
- Tipo: skill
- Evidência: o dono perguntou "como que o planejar confia que a descrição basta para o comportamento e não que a descrição está faltando ou resumida demais ?" e pediu "Sim, registre". Hoje a primeira avaliação é julgamento da IA no passo 3 do `/planejar`. As outras redes (parar no `/implementar`, o revisor, os testes de paridade) pegam a falta tarde, depois de parte do código escrito. A proposta acrescenta ao `/planejar` e ao `templates/plan.md`:
  1. uma checklist por comportamento reproduzido do original (entradas e saídas, ordem dos passos, tempos por tick ou por quadro, limites e casos de erro, números com fonte), em que cada item vazio vira uma pergunta para a central antes da aprovação;
  2. dois ou três exemplos de entrada e resultado esperado, escritos no plano só a partir da descrição; nos casos delicados, pedidos à central.

  A checagem `docs` pode passar a exigir essa seção nos planos aprovados. Muda uma trava (a skill) e o modelo do plano
- Prioridade sugerida: média (aplicar antes do M1 e do M2, os marcos com mais comportamento do original)
- Status: aceita
- Destino: trava e skill, aplicadas na branch `consolidacao/proposta-p-0002` a pedido do dono ("não conseguiria adiantar já a P-0002 aqui nesta sessão mesmo ?"). Mudaram:
  - a skill `/planejar` (passo 3);
  - o `/implementar` (os exemplos viram testes);
  - o revisor;
  - o `templates/plan.md` (seção "Suficiência das descrições");
  - a checagem `docs`, que exige a seção e recusa um plano aprovado com "Perguntas abertas para a central";
  - o `docs/WORKFLOW.md` e o `docs/setup/DEV.md`.

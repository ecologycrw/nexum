# Modelo de proposta

Cada proposta entra em `docs/PROPOSALS.md`, na seção **Abertas**, com o próximo número livre (contando também as processadas):

```markdown
### P-0001: título curto
- Origem: fatia e tarefa, feedback do dono, medição ou checagem
- Tipo: fatia | regra | trava | skill | agente | doc | decisão | bug
- Evidência: o que mostra que isso é necessário (com as palavras do dono, quando a proposta vier do dono)
- Prioridade sugerida: alta | média | baixa
- Status: aberta
```

Quando a retrospectiva processa a proposta, ela vai para a seção **Processadas** com o status `aceita` (e o destino: fatia, ADR, regra, trava, skill ou agente) ou `descartada` (e o motivo).

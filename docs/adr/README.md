# Decisões de arquitetura (ADRs)

Cada ADR registra uma decisão importante, o motivo e as alternativas. Um ADR aceito não é editado: se a decisão mudar, um ADR novo o substitui e diz isso no front matter (`substitui` / `substituida-por`). O modelo fica em `templates/adr.md`.

Os ADRs 0001 a 0020 são as decisões D-01 a D-20 da [especificação de referência](../referencia/README.md), adotadas uma por uma. Os ADRs 0021 a 0028 são decisões do Nexum, a maioria sobre o ambiente de desenvolvimento. Os ADRs 0029 a 0031 trazem as versões novas de D-04, D-09 e D-12, depois que a spec reescreveu o Enhanced. O ADR 0032 muda a ordem do roteiro da Parte VI: a rede do M2 vem antes do M1.

| ADR | Decisão | Status |
|---|---|---|
| [0001](0001-caminho-b-sala-limpa.md) | Caminho B, engine do zero em sala limpa | aceita |
| [0002](0002-tres-camadas.md) | Três camadas, engine, kit do gênero e módulo MU | aceita |
| [0003](0003-linguagem-build-e-dependencias.md) | Linguagem, build e dependências | aceita |
| [0004](0004-sdl3-e-sdl-gpu.md) | Plataforma e GPU com SDL3 e SDL GPU | substituída pelo 0029 |
| [0005](0005-tick-fixo-de-25-hz.md) | Tick fixo de 25 Hz com desenho interpolado | aceita |
| [0006](0006-modelo-de-threads.md) | Modelo de threads | aceita |
| [0007](0007-mundo-entidades-e-handles.md) | Mundo com entidades, componentes e handles | aceita |
| [0008](0008-conversao-offline-de-assets.md) | Assets convertidos offline para formatos próprios | aceita |
| [0009](0009-render-forward-e-classic.md) | Render forward com passes fixos e Classic em espaço gama | substituída pelo 0030 |
| [0010](0010-rede-adaptador-em-camadas.md) | Rede com adaptador em camadas, codec gerado e dialeto 106.3 | aceita |
| [0011](0011-interface-rmlui-640x480.md) | Interface com RmlUi e base 640x480 | aceita |
| [0012](0012-audio-paridade-primeiro.md) | Áudio com paridade primeiro | substituída pelo 0031 |
| [0013](0013-ferramentas-de-autoria-em-imgui.md) | Ferramentas de autoria em ImGui dentro da engine | aceita |
| [0014](0014-qualidade-e-paridade-medida.md) | Qualidade com testes, paridade medida e CI | aceita |
| [0015](0015-aleatoriedade-deterministica.md) | Aleatoriedade determinística | aceita |
| [0016](0016-dados-do-jogo.md) | Dados do jogo | aceita |
| [0017](0017-seguranca-modelo-honesto.md) | Segurança com um modelo honesto | aceita |
| [0018](0018-configuracao-logs-e-diagnostico.md) | Configuração, logs e diagnóstico | aceita |
| [0019](0019-convencoes.md) | Convenções do mundo e do código | aceita |
| [0020](0020-conteudo-original-e-distribuicao.md) | Conteúdo original e distribuição | aceita |
| [0021](0021-repositorio-publico-mit.md) | Repositório público com licença MIT | aceita |
| [0022](0022-organizacao-do-repositorio.md) | Organização do repositório em Engine, Kit, Games e Tools | aceita |
| [0023](0023-arquivos-e-includes-em-pascalcase.md) | Arquivos C++ e pastas de include em PascalCase | aceita |
| [0024](0024-fluxo-git.md) | Fluxo Git com merge só pelo dono e começo em modo local | aceita |
| [0025](0025-ferramentas-de-verificacao.md) | Ferramentas de verificação | aceita |
| [0026](0026-documentacao-como-memoria.md) | Documentação como memória do projeto | aceita |
| [0027](0027-central-de-conhecimento-e-sala-limpa.md) | Central de conhecimento em sessão separada e sala limpa vigiada | aceita |
| [0028](0028-prova-visual-e-paridade.md) | Prova visual e paridade visual | aceita |
| [0029](0029-sdl3-e-sdl-gpu.md) | Plataforma e GPU com SDL3 e SDL GPU, com o Vulkan como padrão no Windows | aceita |
| [0030](0030-render-forward-classic-e-enhanced.md) | Render forward com passes fixos, Classic em espaço gama e o Enhanced do MuMain como referência | aceita |
| [0031](0031-audio-paridade-primeiro-e-audio-3d.md) | Áudio com paridade primeiro e o Áudio 3D como opção à parte | aceita |
| [0032](0032-rede-antes-do-importador.md) | A rede antes do importador, com o M2 dividido em duas partes | aceita |

# Glossário

Termos usados no projeto, em português simples.

| Termo | Significado |
|---|---|
| **A\*** | O algoritmo clássico para calcular um caminho entre dois pontos desviando dos obstáculos. No MU, quem calcula o caminho é o cliente; o servidor só confere passo a passo |
| **Adaptador de servidor** | O encaixe único entre o jogo e o servidor. Do outro lado pode estar o OpenMU ao vivo ou o replay de uma gravação, e o jogo não sabe qual dos dois é |
| **ADR** | *Architecture Decision Record*: um registro curto de uma decisão importante, com o motivo e as alternativas. Fica em `docs/adr/` |
| **Alocação de memória** | Quando o programa pede ao computador um pedaço novo de memória. Pedir no meio do jogo pode causar pequenos travamentos, por isso o caminho quente usa arenas |
| **Antialiasing (MSAA)** | Suavizar os "serrilhados" nas bordas dos objetos. O MSAA faz isso calculando cada ponto da tela mais de uma vez (2, 4 ou 8 amostras). No Classic vem desligado, como no MuMain |
| **AO** | Oclusão de ambiente: escurece cantos, bases de parede e o chão embaixo dos objetos, onde a luz chega menos |
| **Arena** | Um bloco de memória reservado uma vez e zerado a cada quadro ou a cada tick, para os dados temporários |
| **Asset** | Tudo o que não é código: modelos 3D, animações, texturas, ícones, sons |
| **Áudio 3D** | Uma opção do Nexum, separada do Enhanced: o som vem da posição de quem o fez, e há volumes extras (ambiente, interface). O original toca os efeitos sempre em 2D |
| **Aviso de pronto** | A mensagem em que o cliente diz ao servidor "terminei de carregar o mapa" (`C1 F3 12`). O servidor só a espera na troca de mapa |
| **Bloom** | O brilho que "vaza" em volta das partes mais claras da tela. No Enhanced, só os efeitos de luz (magias, fogo) brilham assim |
| **Branch** | Uma linha de trabalho paralela no Git. Cada fatia é feita na sua branch, sem mexer na versão oficial |
| **Build de desenvolvimento (Dev)** | A versão compilada para desenvolver: com conferências extras e com o modo de autoria |
| **Build de jogador** | A versão otimizada que vai para quem joga, sem modo de autoria, socket de automação nem inspetor de pacotes |
| **C1, C2, C3, C4** | Os quatro tipos de pacote do protocolo do MU. C1 e C3 têm tamanho de 1 byte; C2 e C4, de 2 bytes. C3 e C4 são cifrados |
| **CA** | Critério de aceitação: uma frase verificável que diz quando uma fatia está pronta (por exemplo, `CA-01`) |
| **Cache convertido** | A pasta, na máquina do usuário, com os arquivos originais já convertidos pelo `muimport` para os formatos da engine |
| **Central de conhecimento** | Um projeto separado, só no computador do dono, que estuda o MuMain e o OpenMU e escreve specs, achados e lições |
| **CI** | Integração contínua: os testes que rodam nas máquinas do GitHub a cada envio de código, nos 3 sistemas |
| **Classic** | O modo de aparência da paridade: espaço gama, luz por vértice e materiais clássicos, para ficar próximo do original |
| **clang-tidy** | Uma ferramenta que lê o código C++ e aponta erros e práticas perigosas |
| **CMake** | A ferramenta que descreve como o código C++ é compilado |
| **Codec** | O código que transforma pacotes em dados e dados em pacotes. No Nexum ele é gerado a partir dos XML do OpenMU |
| **Commit** | Um ponto de salvamento no histórico do Git |
| **Conta de máquina** | A conta do GitHub usada só pela IA. Ela envia código e abre PRs, mas não aprova nem faz merge |
| **Coordenada dupla** | Cada personagem tem uma posição em células (a que o servidor conhece) e uma posição contínua no mundo (a que aparece na tela) |
| **Dialeto de pacotes** | A família de formatos que o OpenMU usa com um cliente, escolhida pela versão que o cliente manda no login. O Nexum usa o dialeto 106.3, o "Extended" |
| **Enhanced** | O modo de aparência aprimorado, criado pelo MuMain: filtragem de textura, iluminação aprimorada, luzes dinâmicas, luz do sol, light mood, pós-processamento e antialiasing, sem compromisso de paridade. Não inclui o som |
| **Enquadramento** | Cortar o fluxo contínuo de bytes do TCP em mensagens separadas, lendo o tamanho no cabeçalho |
| **Época** | Um número local que muda a cada conexão e a cada troca de mapa. Mensagens e eventos de uma época velha são descartados |
| **Evento neutro** | A tradução de um pacote para algo que o jogo entende sem saber de bytes, como "o objeto X entrou na visão em P" |
| **Fatia** | Uma etapa pequena e completa do desenvolvimento, que entrega algo que dá para ver funcionando ou medir |
| **FPS** | Quadros por segundo: quantas vezes por segundo a janela é redesenhada |
| **Fuzzing** | Alimentar um leitor ou decodificador com milhões de dados aleatórios ou estragados para achar o que o derruba |
| **glTF** | Um formato aberto de modelos 3D e animações. O `muimport` exporta para ele, para conferir um modelo no Blender |
| **GPU** | A placa de vídeo. A **integrada** vem junto do processador e é mais fraca que uma placa separada |
| **GridRpg** | O kit do gênero do Nexum: os sistemas de um RPG de ação em grade, sem nada específico do MU |
| **Handle geracional** | Uma referência a algo que pode deixar de existir, feita de um índice e de uma "geração". Um handle velho nunca aponta para um objeto novo que ocupou o mesmo lugar |
| **Hook** | Um script que roda automaticamente antes ou depois de uma ação (um commit, um comando da IA) e pode bloqueá-la |
| **Interpolação de render** | Com o tick fixo, a tela desenha "entre" dois ticks, misturando o estado anterior e o atual. Assim o movimento fica liso a 144 Hz, com a lógica a 25 Hz |
| **Invariante** | Uma regra que nenhuma mudança pode violar (`INV-01`…, em [ARCHITECTURE.md](ARCHITECTURE.md#invariantes)) |
| **Light mood** | A luz própria de cada mapa no Enhanced: deixa a luz fixa do chão mais escura, para tochas, fogueiras e magias se destacarem. No Classic fica desligado |
| **Linha vermelha** | A regra de que a Engine e o Kit não sabem o que é MU: nenhum nome, número ou código de pacote do jogo dentro deles. Uma checagem automática confere |
| **LUT de cor** | Uma tabela que troca cada cor da tela por outra, para dar um tom próprio a cada mapa. No Enhanced é opcional e nunca clareia a imagem |
| **`main`** | A branch principal: a versão oficial, aprovada pelo dono |
| **Marco** | Um objetivo grande do roteiro (M0 a M15), como "rede sem gráficos". Pode virar várias fatias |
| **Merge (squash)** | Juntar uma branch na `main`. O *squash* transforma todos os commits da branch num commit só |
| **Modo de autoria (Authoring)** | O modo do build de desenvolvimento com as ferramentas em ImGui: inspetores, visualizadores, editores e console |
| **Módulo** | Uma peça da engine com função própria, guardada numa pasta com o código e os testes dela. `Core` e `Platform` são módulos |
| **Módulo MU** | A camada de cima, em `Games/MuOnline`: tudo o que é específico do MU e do OpenMU |
| **muimport** | A ferramenta que lê, uma única vez, os arquivos originais do usuário e grava o cache convertido |
| **MuMain** | O cliente open source em C++ estudado pela central. Não tem licença, e o Nexum nunca lê o código dele |
| **OpenMU** | O servidor open source em C#, com licença MIT. É o servidor do Nexum, sem modificações |
| **Paridade (P0 a P3)** | Os níveis de fidelidade ao original: P0 protocolo, P1 comportamento, P2 visual clássico, P3 visual aprimorado. Cada nível tem um teste |
| **Picking** | Descobrir o que está sob o cursor do mouse: uma célula do chão ou um objeto |
| **Ponto de controle** | O retrato resumido do mundo (a impressão do estado, os objetos na visão e os itens) que a gravação guarda a cada 5 segundos e no fim. O replay confere se chegou ao mesmo retrato |
| **Pilha de passes** | O jeito de descrever um material clássico: o modelo é desenhado mais de uma vez, com modos diferentes, e o nível do item escolhe quantos passes extras ele recebe |
| **PR** | *Pull Request*: o pedido para juntar uma branch na `main`. É onde o dono revisa e aprova |
| **Predição** | O cliente mostra uma ação antes da resposta do servidor (começar a andar, a animação de ataque), para parecer rápido |
| **`Private`** | A pasta de um módulo com os detalhes internos, que os outros módulos não podem usar |
| **Prova visual** | A conferência da imagem gerada por uma fatia. As regras ficam em [VISUAL.md](VISUAL.md#prova-visual) |
| **`Public`** | A pasta de um módulo com o que os outros módulos podem usar |
| **Push** | Enviar os commits para o GitHub |
| **Quadro** | Uma imagem da janela. O quadro roda no ritmo do monitor; a lógica roda em ticks |
| **RAII** | Uma regra de C++: todo recurso tem um dono, e ele é liberado automaticamente quando o dono deixa de existir |
| **Render graph** | Uma descrição automática dos passes de desenho. Adiado: o Nexum usa uma lista fixa de passes |
| **Replay** | Reproduzir uma sessão gravada (`.mucap`) sem servidor, conferindo cada tick e cada ponto de controle, para saber que o jogo chega ao mesmo estado com as mesmas mensagens |
| **Retrato** | A cópia interpolada do mundo que o desenho recebe. O desenho só lê o retrato |
| **RHI** | A camada que esconde as diferenças entre Direct3D 12, Vulkan e Metal. No Nexum, é o SDL GPU |
| **RmlUi** | A biblioteca de interface, que usa RML e RCSS, parecidos com HTML e CSS |
| **Rubberband** | O "elástico": quando o servidor corrige a posição, o personagem é puxado para onde realmente está |
| **Sala limpa** | Escrever um programa novo a partir de **descrições** de comportamento, sem copiar o código do original. Regra central do Nexum |
| **Sanitizer** | Uma ferramenta que roda junto com os testes e detecta erros de memória |
| **SDL3 e SDL GPU** | A biblioteca que cuida da janela, da entrada e da placa de vídeo, em Windows, Linux e macOS |
| **Servidor autoritativo** | O servidor é o único que decide o resultado (dano, posição, drop). O cliente só pede e mostra |
| **Shader** | Um programa pequeno que roda na placa de vídeo e decide onde e com que cor cada ponto aparece |
| **Skinning** | Deformar a malha de um personagem de acordo com os ossos do esqueleto |
| **Spec** | Uma especificação: na central, a descrição completa de um formato ou sistema; no Nexum, o que uma fatia entrega e como saber que está pronta |
| **Tick fixo** | A lógica do jogo avança em passos de 40 ms (25 por segundo), não importa o FPS |
| **Trava** | Qualquer proteção automática: hook, permissão, teste de arquitetura, CI, regra do GitHub |
| **vcpkg** | O programa que baixa e compila as bibliotecas externas de C++ nas versões combinadas |
| **VFS** | Sistema de arquivos virtual: o jeito único de o jogo abrir arquivos, juntando o cache convertido, o conteúdo próprio e os mods, sem diferenciar maiúsculas |
| **Webzen** | A empresa dona do MU Online. Nenhum asset dela entra no repositório |

# Relatório da fatia 1: A rede até o mundo

> Rascunho em construção durante o `/implementar`. As seções de provas, sala limpa e teste do dono são preenchidas no `/fechar`. Por enquanto, aqui ficam os desvios do plano e as travas alteradas, tarefa a tarefa.

## Desvios do plano

- **A checagem `structure` não mudou** (T02). O plano previa que ela passasse a aceitar a pasta `third_party/openmu` e o registro, no THIRD_PARTY, de conteúdo que não vem do vcpkg. Ela já aceitava a pasta, e o registro ficou numa lista própria do THIRD_PARTY ("Conteúdo de terceiros fora do vcpkg"), fora da tabela que a checagem confere, como já acontecia com o Tracy. Uma trava a menos mudou.
- **A checagem `architecture` também não mudou** (T01). Ela já dava o Asio ao `Engine/Net` desde a preparação.
- **O código gerado só tem cabeçalhos** (T03). O plano falava em "código gerado"; ele saiu como cabeçalhos com funções pequenas, sem arquivos `.cpp`, mais um arquivo de testes por XML. As peças comuns de leitura e escrita ficam escritas à mão em `Mu/Protocol/PacketFields.hpp`.
- **Seis campos de um arquivo usam um enum do outro** (T03): por exemplo, o `ClientToServerPackets.xml` usa o `ServerToClient.GuildMemberRole`. Os dois arquivos se citam um ao outro, então esses campos ficam com o byte cru (`std::uint8_t`), sem o tipo do enum. Nenhum deles é usado nesta fatia.

- **O fuzzing achou, antes de rodar, dois leitores sem guarda para uma mensagem vazia** (T09). Ao escrever os alvos, ficou claro que a cifra (`Encrypt`, `Decrypt` e o Xor32) lia o primeiro byte de uma mensagem vazia, e que o `Encrypt` aceitava uma mensagem sem código depois do cabeçalho. Os dois passaram a recusar esses casos com um erro. Na sessão, as mensagens vêm do enquadramento, que nunca entrega uma mensagem assim, mas o leitor não depende disso (INV-12).
- **O `LoadPng` ganhou dois irmãos públicos** (T09): o `ReadPngSize`, que lê o tamanho no cabeçalho, e o `DecodePng`, que lê um PNG da memória. O alvo de fuzzing do PNG usa os dois, e o `LoadPng` segue o mesmo caminho.

## Travas alteradas

- **`vcpkg.json`** (T01): entrou o Asio 1.38.2, para o transporte TCP. Aprovado pelo dono em 2026-11-25 ("Sim, todas"), com o download pelo vcpkg.
- **`cmake/NexumPacketGen.cmake`** (T03, arquivo novo): a função `nexum_generate_packets`, que roda o `packetgen` no build, e a busca do Python 3.10, com uma mensagem clara se ele faltar. Aprovado pelo dono na mesma resposta.
- **`cmake/NexumFuzzing.cmake`** (T09, arquivo novo): a opção `NEXUM_FUZZING` e a função `nexum_add_fuzz_target`. Sem a opção, cada alvo vira um teste que roda as sementes dele nos 3 sistemas; com ela, só no Clang, vira também um programa do libFuzzer. Aprovado pelo dono em 2026-11-25 ("Sim, todas").
- **`CMakePresets.json`** (T09): o preset `linux-fuzz`, de configuração, de build e de teste. Mesma aprovação.
- **`.github/workflows/ci.yml`** (T09): o passo "Fuzzing" no job do Linux com Clang, que compila o preset `linux-fuzz` e roda cada alvo por 60 s a partir das sementes. Mesma aprovação.
- **`scripts/dev.py`** (entre a T10 e a T11, pela primeira CI do PR #11): o `tidy` compila o build de desenvolvimento antes de analisar, porque o `clang-tidy` lê os cabeçalhos gerados dos pacotes, e o job da análise estática na CI só configurava. Aprovado pelo dono em 2026-11-26 ("Sim, pode mudar").

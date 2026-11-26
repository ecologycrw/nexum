# Relatório da fatia 1: A rede até o mundo

> Rascunho em construção durante o `/implementar`. As seções de provas, sala limpa e teste do dono são preenchidas no `/fechar`. Por enquanto, aqui ficam os desvios do plano e as travas alteradas, tarefa a tarefa.

## Desvios do plano

- **A checagem `structure` não mudou** (T02). O plano previa que ela passasse a aceitar a pasta `third_party/openmu` e o registro, no THIRD_PARTY, de conteúdo que não vem do vcpkg. Ela já aceitava a pasta, e o registro ficou numa lista própria do THIRD_PARTY ("Conteúdo de terceiros fora do vcpkg"), fora da tabela que a checagem confere, como já acontecia com o Tracy. Uma trava a menos mudou.
- **A checagem `architecture` também não mudou** (T01). Ela já dava o Asio ao `Engine/Net` desde a preparação.
- **O código gerado só tem cabeçalhos** (T03). O plano falava em "código gerado"; ele saiu como cabeçalhos com funções pequenas, sem arquivos `.cpp`, mais um arquivo de testes por XML. As peças comuns de leitura e escrita ficam escritas à mão em `Mu/Protocol/PacketFields.hpp`.
- **Seis campos de um arquivo usam um enum do outro** (T03): por exemplo, o `ClientToServerPackets.xml` usa o `ServerToClient.GuildMemberRole`. Os dois arquivos se citam um ao outro, então esses campos ficam com o byte cru (`std::uint8_t`), sem o tipo do enum. Nenhum deles é usado nesta fatia.

## Travas alteradas

- **`vcpkg.json`** (T01): entrou o Asio 1.38.2, para o transporte TCP. Aprovado pelo dono em 2026-11-25 ("Sim, todas"), com o download pelo vcpkg.
- **`cmake/NexumPacketGen.cmake`** (T03, arquivo novo): a função `nexum_generate_packets`, que roda o `packetgen` no build, e a busca do Python 3.10, com uma mensagem clara se ele faltar. Aprovado pelo dono na mesma resposta.

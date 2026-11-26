---
id: 0034
titulo: O gerador do codec (packetgen) em Python, rodando no build
status: aceita
data: 2026-11-25
substitui:
substituida-por:
---

# ADR 0034: O gerador do codec (packetgen) em Python, rodando no build

## Contexto

O [D-10](../referencia/especificacao-engine-propria-caminho-b.md#d-10--rede-adaptador-em-camadas-codec-gerado-dialeto-1063) e o [ADR 0010](0010-rede-adaptador-em-camadas.md) mandam gerar o codec dos pacotes a partir dos XML do OpenMU (MIT), com uma tabela do dialeto 106.3 escrita à mão. A spec de referência diz que o gerador é nosso, mas não diz em que ele é escrito nem onde fica o código gerado. São 613 pacotes nos quatro arquivos (391 do servidor, 204 do cliente, 11 do Connect Server e 7 do chat). Gerados, eles passam de 30 mil linhas de C++.

O dono aprovou esta decisão junto com a spec da [fatia 1](../../specs/0001-rede-ate-o-mundo/spec.md), em 2026-11-25.

## Decisão

1. **O `packetgen` é um script em Python 3, só com a biblioteca padrão,** em `Tools/PacketGen/Private/packetgen.py`. O Python já é exigido pelo `scripts/dev.py` e pelas checagens, e a biblioteca padrão lê XML.
2. **O código gerado fica na pasta de build, nunca no repositório.** A função `nexum_generate_packets`, de `cmake/NexumPacketGen.cmake`, roda o script a cada build em que um XML ou o script mudou. Cada pacote é um cabeçalho próprio, `Mu/Protocol/Packets/<Arquivo>/<Pacote>.hpp`, que inclui só os tipos comuns do arquivo (`Types.hpp`). O módulo MU inclui os pacotes pelas partes do dialeto (`Mu/Protocol/Dialect/`), e cada arquivo do código inclui só as partes que usa. Nenhum cabeçalho inclui todos os pacotes juntos. Como o build precisa conhecer os arquivos antes de gerar, o script os lista quando o CMake configura (`--list`), e uma mudança num XML ou no script configura de novo.
3. **O script também roda no build de jogador,** porque o jogo lê os pacotes com o código gerado. O script não entra no programa: ele é uma ferramenta do build, como o compilador. A pasta `Tools/PacketGen` do build guarda só os testes do gerador, e esses só existem no build de desenvolvimento (INV-02).
4. **O que o código gerado garante:**
   - cada campo é lido e escrito pela posição, com a ordem dos bytes e o deslocamento de bits que o XML declara, nunca por um *cast* de struct (INV-12);
   - a leitura de um pacote (`View::Read`) confere o cabeçalho, o tamanho declarado, o código, o subcódigo e o tamanho mínimo antes de dar acesso aos campos;
   - cada pacote gerado tem um teste gerado dos bytes fixos, do tamanho e da recusa de um pacote menor. Os testes vêm em arquivos de até 40 pacotes, cada um incluindo só os pacotes que testa.
5. **As peças comuns ficam escritas à mão,** em `Games/MuOnline/Public/Mu/Protocol/PacketFields.hpp`: ler e escrever números, bits, textos e blocos, e conferir o cabeçalho.

## Consequências

- **Melhor:** o diff de uma mudança no XML mostra só o XML, e não milhares de linhas geradas. O código gerado nunca fica desatualizado em relação ao XML.
- **Melhor:** para seguir um OpenMU novo, basta copiar os XML de uma base nova, gerar de novo e refazer a tabela do dialeto (o risco 5 da spec de referência).
- **Pior:** o build passa a exigir o Python 3.10 ou mais novo, também no build de jogador. O CMake procura o Python e para com uma mensagem clara se ele faltar.
- **Pior:** quem quer ler o código gerado precisa compilar antes. Ele fica em `build/<preset>/Games/MuOnline/Generated/`.
- **Mudança de 2026-11-26 ([P-0016](../PROPOSALS.md)):** no começo, cada XML gerava um cabeçalho só, e o do servidor tinha 1,2 MB. Toda compilação que falava com a rede lia o arquivo inteiro e passava de 1 GB de memória, e o build enchia os 16 GB do computador do dono. Desde então, cada pacote é um cabeçalho próprio, como diz o item 2.
- **Exigido:** o script é testado em duas camadas. Os testes em Python conferem a leitura do XML e as recusas. O C++ gerado de um XML de teste próprio (`Tools/PacketGen/Tests/Fixture.xml`) confere cada tipo de campo, byte a byte.
- **Exigido:** os XML ficam em `third_party/openmu`, com o aviso da licença MIT e a base registrada.

## Alternativas consideradas

- **O gerador em C++:** precisaria de uma biblioteca de XML nova só para isso, e o gerador teria de ser compilado antes de compilar o resto.
- **O código gerado guardado no repositório:** poria milhares de linhas no diff a cada mudança do XML, e o código guardado poderia ficar diferente do XML sem ninguém perceber.
- **Gerar só os pacotes que o 106.3 usa:** a tabela do dialeto escolheria as variantes dentro do gerador. Isso juntaria duas responsabilidades. Com o gerador produzindo todas as variantes, a tabela do dialeto continua sendo o único lugar que sabe o que é o 106.3.

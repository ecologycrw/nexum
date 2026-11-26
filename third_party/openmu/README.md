# As definições de pacotes do OpenMU

Estes arquivos vêm do [OpenMU](https://github.com/MUnique/OpenMU), sem nenhuma mudança no conteúdo (só os fins de linha viram LF, como em todo o repositório), e seguem a licença MIT do projeto, copiada em [LICENSE](LICENSE) (Copyright 2017 MUnique).

- **Base:** o commit `b53c704c6` do OpenMU, o mesmo que a central de conhecimento estudou.
- **Origem:** a pasta `src/Network/Packets/` do OpenMU. Os XML de pacotes vêm das subpastas `ConnectServer`, `ClientToServer`, `ServerToClient` e `ChatServer`; o `CommonEnums.xml`, o `PacketHeaders.xml` e o esquema `PacketDefinitions.xsd`, da própria pasta.
- **Quem usa:** o `packetgen` (`Tools/PacketGen`), que gera, a cada build, o código que lê e escreve os pacotes. O código gerado fica na pasta de build, nunca no repositório ([ADR 0034](../../docs/adr/0034-packetgen-em-python-no-build.md)).
- **Para atualizar:** copie de novo os mesmos arquivos de uma base nova, troque a base aqui e no [THIRD_PARTY](../../docs/THIRD_PARTY.md), gere o código de novo e refaça a tabela do dialeto 106.3. Os arquivos nunca são editados à mão.

# Nexum

> *Nexum is a clean-room C++20 engine for a game client compatible with the unmodified OpenMU server. Documentation is currently in Portuguese.*

O Nexum (do latim *nexum*, "laço, vínculo") é uma engine escrita do zero, em C++20, para um cliente de MU compatível com o servidor [OpenMU](https://github.com/MUnique/OpenMU) **sem nenhuma modificação** no servidor.

São três camadas num mesmo repositório, e a dependência só aponta para baixo:

- **Engine (Nexum):** janela, GPU, recursos, entidades, animação, efeitos, áudio, rede em bytes, interface e diagnóstico. Não sabe o que é MU.
- **Kit do gênero (Nexum GridRpg):** o que todo RPG de ação em grade, com câmera aérea e servidor autoritativo, precisa: grade do mundo, movimento por passos com predição e correção, câmera, inventário em grade, texto flutuante, cursor e minimapa. Também não sabe o que é MU.
- **Módulo MU:** tudo o que é do MU e do OpenMU: protocolo, regras do lado do cliente, efeitos, interface e eventos. Gera o executável `MuClient`.

Ao lado delas fica a ferramenta `muimport`, que converte, uma única vez, os arquivos da instalação que o usuário já tem para os formatos próprios da engine. O jogo nunca abre um arquivo original.

## Estado atual

**Em preparação.** Já existem os documentos, um esqueleto que compila, as checagens automáticas e o ambiente de desenvolvimento assistido por IA. A primeira fatia, o M0 (Fundação), é a próxima. Veja [docs/STATUS.md](docs/STATUS.md) e [docs/ROADMAP.md](docs/ROADMAP.md).

## Documentação

- [Visão](docs/VISION.md): o que o produto é e o que ele não é
- [Arquitetura](docs/ARCHITECTURE.md): como as peças se encaixam e as regras que nunca mudam
- [Fluxo de trabalho](docs/WORKFLOW.md): como o projeto é desenvolvido, inclusive a sala limpa
- [Guia visual](docs/VISUAL.md): os modos Classic e Enhanced e como a paridade visual é medida
- [Decisões (ADRs)](docs/adr/README.md): por que cada decisão foi tomada
- [Especificação de referência](docs/referencia/README.md): a spec do Caminho B, de onde vêm as decisões
- [Glossário](docs/GLOSSARY.md): os termos usados no projeto
- [Bibliotecas de terceiros](docs/THIRD_PARTY.md): as dependências e as licenças delas

## Sala limpa

O Nexum é escrito a partir de **descrições de comportamento**, com texto, tabelas e pseudo-código próprios. Nenhum código do cliente MuMain é lido ou copiado. O código e os documentos do OpenMU, que usa a licença MIT, podem ser reaproveitados, sempre com o aviso da licença. Veja [docs/WORKFLOW.md](docs/WORKFLOW.md#sala-limpa).

## Desenvolvimento assistido por IA

Quase todo o código deste projeto é escrito por IA, com revisão e aprovação humana. Os commits da IA são identificados como coautoria do Claude. Veja [docs/WORKFLOW.md](docs/WORKFLOW.md).

## Contribuições

Ainda não estão abertas. Isso vai mudar quando a base estiver estável.

## Segurança

Para relatar uma vulnerabilidade, veja [SECURITY.md](SECURITY.md).

## Licença

[MIT](LICENSE).

## Aviso

O Nexum é um projeto independente, sem afiliação com a Webzen, com o OpenMU ou com qualquer outra empresa ou marca. MU Online é uma marca da Webzen. **Nenhum asset da Webzen faz parte deste repositório:** nem modelos, nem texturas, nem sons, nem tabelas de dados. Para jogar, o usuário aponta o `muimport` para a própria instalação.

# A gravação de referência do OpenMU

O `openmu-two-players.mucap` é uma sessão de cerca de 20 segundos no mundo, gravada pelo `MuClient` no [OpenMU](https://github.com/MUnique/OpenMU) 0.10.1 local, a imagem de teste do `tests/integration/openmu/compose.yaml`. A gravação guarda só o que o servidor enviou no mundo, e esse conteúdo vem do OpenMU: os números de itens e de NPCs, as posições e os nomes dos personagens de teste `test0Dk` e `test1Dk`. Ele segue a licença MIT do OpenMU, copiada em [third_party/openmu/LICENSE](../../../../third_party/openmu/LICENSE) (Copyright 2017 MUnique). Não há nada da Webzen, nem conta, nem senha ([spec da fatia 2](../../../../specs/0002-gravacao-e-replay/spec.md), decisão 5).

- **Cópia:** a semente `../FuzzSeeds/Recording/openmu-two-players.bin` do alvo de fuzzing `muonline_recording` tem os mesmos bytes.
- **Quem usa:** o teste da biblioteca em `../ReplayServerTest.cpp`, o `muclient_replays_the_reference_recording` e o `muclient_fails_a_replay_stopped_before_its_end`, em `Games/MuOnline/App/CMakeLists.txt`.
- **Para refazer:** siga o [DEV.md](../../../../docs/setup/DEV.md#refazer-a-gravação-de-referência), e troque as duas cópias juntas.

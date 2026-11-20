# Itens provisórios

Tudo o que foi simplificado de propósito, ou que ainda precisa de uma decisão ou de uma prova. Todo item tem um destino (uma fatia, como `fatia 3` ou `fatia de escala`, ou um marco, como `M0`), e a checagem automática recusa um item sem destino. Quando o item é resolvido, ele sai desta tabela e a resolução fica registrada no report da fatia.

| ID | Item provisório | Por que é provisório | Destino | Origem |
|---|---|---|---|---|
| TD-0001 | A CI nunca rodou | o repositório ainda é só local; a CI roda quando ele for publicado no GitHub ([GITHUB.md](setup/GITHUB.md)) | M0 | preparação |
| TD-0002 | O build e os testes nunca rodaram em Linux (Clang e GCC) nem no macOS | a máquina de desenvolvimento é Windows; os outros sistemas só são testados pela CI | M0 | preparação |
| TD-0003 | No Linux, o SDL é compilado sem suporte a janelas de verdade (X11 e Wayland) | a preparação só precisa da versão do SDL; a janela nos 3 sistemas é do M0 | M0 | preparação |
| TD-0004 | `Result<T>` usa o `tl::expected` em vez do `std::expected` | o `std::expected` exige o modo C++23 nos três compiladores; a interface é a mesma, e a troca é uma linha ([ADR 0003](adr/0003-linguagem-build-e-dependencias.md)) | M7 | preparação |
| TD-0005 | Os requisitos exatos do SDL GPU e o piso de hardware não foram conferidos | a spec pede a conferência na documentação do SDL na época do M0 ([VISION](VISION.md#piso-de-hardware)) | M0 | preparação |
| TD-0006 | A prova visual ainda não tem a ferramenta de comparação de imagens | ainda não existe nenhuma tela; a ferramenta entra com a primeira janela ([ADR 0028](adr/0028-prova-visual-e-paridade.md)) | M0 | preparação |
| TD-0007 | O build de jogador é conferido só pelo `MuClient --version`, sem a checagem de símbolos | o modo de autoria ainda não existe; a checagem de símbolos entra com ele | M1 | preparação |
| TD-0009 | A conta de máquina do Nexum é, provisoriamente, a do Lemuria (`lemuria-ci-dev`) | o GitHub restringiu a `nexum-ci-dev` logo depois da criação, e o [ADR 0024](adr/0024-fluxo-git.md) rejeita misturar as contas dos dois projetos. Quando o suporte liberar a conta, o dono volta a identidade do Git e o `gh` para a `nexum-ci-dev` e tira a `lemuria-ci-dev` dos colaboradores | M0 | preparação |

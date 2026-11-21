# Another game of the same genre, built with the engine and the kit alone, without the game module of Games/
# (INV-02, docs/adr/0002-tres-camadas.md). scripts/checks refuses any dependency on that module in this folder,
# and the red line refuses any name of that game.
nexum_add_library(other_game
    SOURCES Private/OtherGame.cpp
    PUBLIC_DEPS Nexum::GridRpg Nexum::Time)

nexum_add_tests(other_game_tests
    SOURCES OtherGameTest.cpp
    DEPS other_game)

#include <iostream>
#include <memory>

#include "types.h"

int main() {
    using namespace wallgo;
    // Create a placeholder game with a dummy preset
    std::string preset = "example_preset";
    auto game = std::make_shared<Game>(preset);

    // Create and apply some arbitrary moves
    Move m1(PlayerColor::Red, 1, Direction::Up, std::nullopt, Direction::Left);
    Move m2(PlayerColor::Blue, 2, Direction::Down, Direction::Right, Direction::Up);
    Move m3(PlayerColor::Red, 0, std::nullopt, std::nullopt, Direction::Down);
    game->apply_move(m1);
    game->apply_move(m2);
    game->apply_move(m3);

    // Print encoded history and preset
    std::cout << "Encoded: " << game->encode() << std::endl;

    return 0;
}

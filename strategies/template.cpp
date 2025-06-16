#include <bits/stdc++.h>

#include "../lib/types.h"  // remove later

using namespace wallgo;

#ifdef RED

namespace red {

class PlayerImpl : public Player {
   public:
    void init(PlayerColor player, std::shared_ptr<Game> game, int seed) override {}
    Move move(const std::vector<Move>& valid_moves) override {}
};

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::make_unique<PlayerImpl>(new PlayerImpl()); }

}  // namespace red

#endif  // RED

#ifdef BLUE

namespace blue {

class PlayerImpl : public Player {
   public:
    void init(PlayerColor player, std::shared_ptr<Game> game, int seed) override {}
    Move move(const std::vector<Move>& valid_moves) override {}
};

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::make_unique<PlayerImpl>(new PlayerImpl()); }

}  // namespace blue

#endif  // BLUE

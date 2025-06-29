#include <random>

#include "../lib/types.h"

using namespace wallgo;

namespace {
class PlayerImpl : public Player {
   private:
    // Local variables

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override {}

    Position place(const std::vector<Position>& valid_positions) override {}

    Move move(const std::vector<Move>& valid_moves) override {}
};
}  // namespace

#ifdef RED

namespace red {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new PlayerImpl()); }

}  // namespace red

#endif  // RED

#ifdef BLUE

namespace blue {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new PlayerImpl()); }

}  // namespace blue

#endif  // BLUE

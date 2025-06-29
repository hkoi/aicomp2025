#include <random>

#include "../lib/types.h"

using namespace wallgo;

namespace {
class RandomImpl : public Player {
   private:
    std::mt19937 rng_;

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override { rng_ = std::mt19937(seed); }

    Position place(PieceId pieceId, const std::vector<Position>& valid_positions) override {
        std::uniform_int_distribution<int> dist(0, (int)valid_positions.size() - 1);
        return valid_positions[dist(rng_)];
    }

    Move move(const std::vector<Move>& valid_moves) override {
        std::uniform_int_distribution<int> dist(0, (int)valid_moves.size() - 1);
        return valid_moves[dist(rng_)];
    }
};
}  // namespace

#ifdef RED

namespace red {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new RandomImpl()); }

}  // namespace red

#endif  // RED

#ifdef BLUE

namespace blue {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new RandomImpl()); }

}  // namespace blue

#endif  // BLUE
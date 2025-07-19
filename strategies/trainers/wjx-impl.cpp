#include <bits/stdc++.h>
#include "../lib/types.h"

using namespace wallgo;

namespace {
class PlayerImpl : public Player {
   private:
    using dist_grid = std::array<std::array<int, 7>, 7>;

    static const int inf = 100;

    // Local variables
    std::mt19937 rng;
    std::shared_ptr<const Game> game;
    PlayerColor player;
    PlayerColor opponent;

    static dist_grid distance(PlayerColor player, const Board& board) {
        const auto opponent = (player == PlayerColor::Red ? PlayerColor::Blue : PlayerColor::Red);
        dist_grid dist;
        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                dist[r][c] = inf;
            }
        }

        std::queue<Position> queue;
        for (const auto& piece : board.get_pieces(player)) {
            const auto& pos = piece.pos;
            dist[pos.r][pos.c] = 0;
            queue.emplace(pos);
        }

        while (!queue.empty()) {
            const auto& pos = queue.front();
            queue.pop();
            for (const auto& neighbor : board.get_accessible_neighbors(pos)) {
                const auto& piece = board.get(neighbor.pos()).piece();
                if (piece.has_value() && piece.value().owner == opponent) {
                    continue;
                }
                if (dist[neighbor.pos().r][neighbor.pos().c] > dist[pos.r][pos.c] + 1) {
                    dist[neighbor.pos().r][neighbor.pos().c] = dist[pos.r][pos.c] + 1;
                    queue.emplace(neighbor.pos());
                }
            }
        }
        return dist;
    }

    double calculate_score(const Board& board) {
        auto my_dist = distance(player, board);
        auto opp_dist = distance(opponent, board);
        double score = 0;
        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                Position pos{r, c};
                if (my_dist[r][c] < inf && opp_dist[r][c] < inf) {
                    score += 1 - static_cast<double>(my_dist[r][c]) / (my_dist[r][c] + opp_dist[r][c]) * 2;
                }
                else if (my_dist[r][c] < inf && opp_dist[r][c] == inf) {
                    score += 1;
                }
                else if (my_dist[r][c] == inf && opp_dist[r][c] < inf) {
                    score -= 1;
                }
            }
        }
        return score;
    }

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override {
        rng.seed(seed);
        this->game = game;
        this->player = player;
        this->opponent = (player == PlayerColor::Red ? PlayerColor::Blue : PlayerColor::Red);
    }

    Position place(PieceId pieceId, const std::vector<Position>& valid_positions) override {
        double score = -inf;
        Position best_position;
        auto valid_positions_copy = valid_positions;
        std::shuffle(valid_positions_copy.begin(), valid_positions_copy.end(), rng);
        for (const auto& pos : valid_positions_copy) {
            auto board = game->board();
            board.set(Cell{pos, Piece{player, pos, pieceId}, board.get(pos).walls()});
            auto current_score = calculate_score(board);
            if (score < current_score) {
                score = current_score;
                best_position = pos;
            }
        }
        return best_position;
    }

    Move move(const std::vector<Move>& valid_moves) override {
        double score = -inf;
        Move best_move(player, 0, std::nullopt, std::nullopt, Direction::Up);
        auto valid_moves_copy = valid_moves;
        std::shuffle(valid_moves_copy.begin(), valid_moves_copy.end(), rng);
        for (const auto& mv : valid_moves_copy) {
            auto board = game->board();
            board = board.apply_move(mv);
            auto current_score = calculate_score(board);
            if (score < current_score) {
                score = current_score;
                best_move = mv;
            }
        }
        return best_move;
    }
};

class RandomImpl : public Player {
   private:
    std::mt19937 rng;

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override {
        rng.seed(seed);
    }

    Position place(PieceId pieceId, const std::vector<Position>& valid_positions) override {
        std::uniform_int_distribution<int> dist(0, (int)valid_positions.size() - 1);
        return valid_positions[dist(rng)];
    }

    Move move(const std::vector<Move>& valid_moves) override {
        std::uniform_int_distribution<int> dist(0, (int)valid_moves.size() - 1);
        return valid_moves[dist(rng)];
    }
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
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new RandomImpl()); }

}  // namespace blue

#endif  // BLUE

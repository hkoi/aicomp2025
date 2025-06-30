#include <algorithm>
#include <array>
#include <cstring>
#include <queue>
#include <random>

#ifdef ONLINE_JUDGE
#include "aicomp.h"
#else
#include "../lib/types.h"
#endif

using namespace wallgo;

namespace {

class RedPlayerImpl : public Player {
   private:
    std::mt19937 rng_;
    std::shared_ptr<const Game> game;

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game_, int seed) override {
        rng_ = std::mt19937(seed);
        game = game_;
    }

    Position place(PieceId, const std::vector<Position>& valid_positions) override {
        std::uniform_int_distribution<int> dist(0, (int)valid_positions.size() - 1);
        return valid_positions[dist(rng_)];
    }

    double evaluate_board(const Board& board) {
        double ans = 0.0;

        // Pre-compute distance matrices for all pieces once
        std::vector<std::array<std::array<int, 7>, 7>> red_distances, blue_distances;

        // Calculate distances from all red pieces
        for (const auto& piece : board.get_pieces(PlayerColor::Red)) {
            std::array<std::array<int, 7>, 7> dist;
            for (int i = 0; i < 7; ++i) {
                for (int j = 0; j < 7; ++j) {
                    dist[i][j] = -1;
                }
            }

            std::queue<Position> queue;
            queue.push(piece.pos);
            dist[piece.pos.r][piece.pos.c] = 0;

            while (!queue.empty()) {
                auto [r, c] = queue.front();
                queue.pop();

                for (const auto& neighbor : board.get_accessible_neighbors(Position{r, c})) {
                    auto [nr, nc] = neighbor.pos();
                    if (dist[nr][nc] == -1) {
                        dist[nr][nc] = dist[r][c] + 1;
                        queue.push(neighbor.pos());
                    }
                }
            }
            red_distances.push_back(dist);
        }

        // Calculate distances from all blue pieces
        for (const auto& piece : board.get_pieces(PlayerColor::Blue)) {
            std::array<std::array<int, 7>, 7> dist;
            for (int i = 0; i < 7; ++i) {
                for (int j = 0; j < 7; ++j) {
                    dist[i][j] = -1;
                }
            }

            std::queue<Position> queue;
            queue.push(piece.pos);
            dist[piece.pos.r][piece.pos.c] = 0;

            while (!queue.empty()) {
                auto [r, c] = queue.front();
                queue.pop();

                for (const auto& neighbor : board.get_accessible_neighbors(Position{r, c})) {
                    auto [nr, nc] = neighbor.pos();
                    if (dist[nr][nc] == -1) {
                        dist[nr][nc] = dist[r][c] + 1;
                        queue.push(neighbor.pos());
                    }
                }
            }
            blue_distances.push_back(dist);
        }

        // Now calculate control for each cell using pre-computed distances
        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                double red_control = 1e-5, blue_control = 1e-5;

                // Sum red control
                for (const auto& dist : red_distances) {
                    if (dist[r][c] != -1) {
                        red_control += exp(-dist[r][c]);
                    }
                }

                // Sum blue control
                for (const auto& dist : blue_distances) {
                    if (dist[r][c] != -1) {
                        blue_control += exp(-dist[r][c]);
                    }
                }

                ans += red_control / blue_control;
            }
        }

        // Penalise opponent from claming territory

        double red = std::pow(board.get_territory().red_total, 2);
        double blue = std::pow(board.get_territory().blue_total, 2);
        ans += pow(red - blue, 3) * 100;
        // std::cout << "Penalty: " << board.get_territory().blue_total << std::endl;
        return ans;
    }

    Move move(const std::vector<Move>& valid_moves) override {
        std::cout << game->board() << std::endl;

        std::vector<double> scores;
        for (auto move : valid_moves) {
            Board new_board = game->board().apply_move(move);
            double score = evaluate_board(new_board);
            scores.push_back(score);

            // std::cout << "Evaluating: " << move << std::endl;
            // std::cout << "Score: " << score << std::endl;
            // std::cout << "Board after move:" << std::endl;
            // std::cout << new_board << std::endl << std::endl;
        }
        auto it = std::max_element(scores.begin(), scores.end());
        return valid_moves[it - scores.begin()];
    }
};

class RandomImpl : public Player {
   private:
    std::mt19937 rng_;

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override { rng_ = std::mt19937(seed); }

    Position place(PieceId, const std::vector<Position>& valid_positions) override {
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
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new RedPlayerImpl()); }

}  // namespace red

#endif  // RED

#ifdef BLUE

namespace blue {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new RandomImpl()); }

}  // namespace blue

#endif  // BLUE
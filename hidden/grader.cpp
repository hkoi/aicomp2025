#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "game_utils.h"
#include "types.h"
namespace red {
std::unique_ptr<wallgo::Player> get();
}

namespace blue {
std::unique_ptr<wallgo::Player> get();
}

namespace wallgo {

const double TIME_LIMIT = 25.0;
class GameController {
   private:
    std::vector<std::unique_ptr<Player>> players_;
    std::vector<std::shared_ptr<Game>> games_;
    std::ostream& output_data_;
    int seed_;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_time_;
    std::chrono::steady_clock steady_clock_;
    std::array<int, 3> player_used_time_ = {0, 0, 0};

    double getTimeSinceLastEvent() const {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = now - last_time_;
        return diff.count();
    }
    std::ostream& addEvent(int player) {
        last_time_ = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = last_time_ - start_time_;
        return output_data_ << player << "\t" << diff.count() << "\t";
    }

    int addAndCheckTime(int player, double time_used) {
        player_used_time_[player] += static_cast<int>(time_used);
        if (player_used_time_[player] > TIME_LIMIT) {
            return player;  // return the player who exceeded the time limit
        }
        return 0;  // no player exceeded the time limit
    }

   public:
    GameController(int seed, std::ostream& output_data)
        : seed_(seed), output_data_(output_data), players_(3), games_(3) {
        games_[0] = std::shared_ptr<Game>(new Game());
        games_[1] = std::shared_ptr<Game>(new Game());
        games_[2] = std::shared_ptr<Game>(new Game());

        start_time_ = std::chrono::steady_clock::now();
    }

    GameOutcome run() {
        // initialise
        addEvent(1) << "Initializing Player 1 (Red)" << std::endl;
        auto p1 = red::get();
        p1->init(PlayerColor::Red, games_[1], seed_);
        double player1_initialize_time = getTimeSinceLastEvent();
        addEvent(1) << "Initializing Player 1 completed in " << player1_initialize_time << "s" << std::endl;
        if (addAndCheckTime(1, player1_initialize_time) == 1) {
            return GameOutcome{PlayerColor::Blue, OPPONENT_TLE, games_[0]->encode(), "TLE during initialization"};
        }
        p1.swap(players_[1]);

        addEvent(2) << "Initializing Player 2 (Blue)" << std::endl;
        auto p2 = blue::get();
        p2->init(PlayerColor::Blue, games_[2], seed_);
        double player2_initialize_time = getTimeSinceLastEvent();
        addEvent(2) << "Initializing Player 2 completed in " << player2_initialize_time << "s" << std::endl;
        if (addAndCheckTime(2, player2_initialize_time) == 2) {
            return GameOutcome{PlayerColor::Red, OPPONENT_TLE, games_[0]->encode(), "TLE during initialization"};
        }
        p2.swap(players_[2]);

        // addEvent(1) << "Initializing Player 1 (Red)" << std::endl;
        // players_[1]->init(PlayerColor::Red, games_[1], seed_);
        // double player1_initialize_time = getTimeSinceLastEvent();
        // addEvent(1) << "Initializing Player 1 completed in " << player1_initialize_time << "s" << std::endl;
        // if (addAndCheckTime(1, player1_initialize_time) == 1) {
        //     return GameOutcome{PlayerColor::Blue, OPPONENT_TLE, games_[0]->encode(), "TLE during initialization"};
        // }

        // addEvent(2) << "Initializing Player 2 (Blue)" << std::endl;
        // players_[2]->init(PlayerColor::Blue, games_[2], seed_);
        // double player2_initialize_time = getTimeSinceLastEvent();
        // addEvent(2) << "Initializing Player 2 completed in " << player2_initialize_time << "s" << std::endl;
        // if (addAndCheckTime(2, player2_initialize_time) == 2) {
        //     return GameOutcome{PlayerColor::Red, OPPONENT_TLE, games_[0]->encode(), "TLE during initialization"};
        // }

        // place the pieces, in order RBBRRBBR
        for (int i = 0; i < 8; ++i) {
            std::vector<Position> valid_positions;

            for (int r = 0; r < 7; ++r) {
                for (int c = 0; c < 7; ++c) {
                    Position pos{r, c};
                    if (!games_[0]->board()[r][c].piece()) {
                        valid_positions.push_back(pos);
                    }
                }
            }

            Position pos;
            int current_player = (i == 0 || i == 3 || i == 4 || i == 7) ? 1 : 2;
            pos = players_[current_player]->place(valid_positions);
            double time_used = getTimeSinceLastEvent();
            addEvent(current_player) << "Took " << time_used << "s to place piece." << std::endl;
            addEvent(current_player) << "Placed piece at (" << pos.r << "," << pos.c << ")" << std::endl;

            int tle_player = addAndCheckTime(current_player, time_used);
            if (tle_player) {
                return GameOutcome{static_cast<PlayerColor>(3 - current_player), OPPONENT_TLE, games_[0]->encode(),
                                   "TLE during piece placement"};
            }

            if (std::find(valid_positions.begin(), valid_positions.end(), pos) == valid_positions.end()) {
                std::stringstream message;
                message << "Player " << current_player << " placed piece at an invalid position (" << pos.r << ","
                        << pos.c << ")";
                return GameOutcome{static_cast<PlayerColor>(3 - current_player), OPPONENT_ILLEGAL_MOVE,
                                   games_[0]->encode(), message.str()};
            }

            for (int game = 0; game < 3; ++game) {
                // id is i/2
                games_[game]->place_piece(pos, static_cast<PlayerColor>(current_player), i / 2);
            }
        }

        // move
        bool game_ended = false;
        int current_player = 1;
        for (; !game_ended; current_player = 3 - current_player) {
            PlayerColor opponent_color = static_cast<PlayerColor>(3 - current_player);

            addEvent(current_player) << "Computing valid moves" << std::endl;
            std::vector<Move>&& valid_moves =
                game_util::GetValidMoves(*games_[0], static_cast<PlayerColor>(current_player));  // note: won't be empty
            addEvent(current_player) << valid_moves.size() << " valid moves" << std::endl;

            Move move = players_[current_player]->move(valid_moves);
            if (move.player() != static_cast<PlayerColor>(current_player)) {
                return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0]->encode(),
                                   "Returned move does not have player set"};
            }
            Piece piece = games_[0]->get_piece(move.player(), move.piece_id());
            double time_used = getTimeSinceLastEvent();
            addEvent(current_player) << "Took " << time_used << "s to compute move." << std::endl;
            addEvent(current_player) << "Chose piece " << piece.id << " at (" << piece.pos.r << "," << piece.pos.c
                                     << ")" << std::endl;
            addEvent(current_player) << "Number of steps: " << (move.direction1() ? 1 : 0) + (move.direction2() ? 1 : 0)
                                     << std::endl;
            if (move.direction1()) {
                addEvent(current_player) << "Direction 1: " << static_cast<int>(*move.direction1()) << std::endl;
            }
            if (move.direction2()) {
                addEvent(current_player) << "Direction 2: " << static_cast<int>(*move.direction2()) << std::endl;
            }
            addEvent(current_player) << "Wall direction: " << static_cast<int>(move.wall_placement_direction())
                                     << std::endl;

            int tle_player = addAndCheckTime(current_player, time_used);
            if (tle_player) {
                return GameOutcome{opponent_color, OPPONENT_TLE, games_[0]->encode(), "TLE during move computation"};
            }

            if (!game_util::IsMoveLegal(*games_[0], move)) {
                std::stringstream message;
                message << "Illegal move made by " << current_player;
                return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0]->encode(), message.str()};
            }
            for (int i = 0; i < 3; i++) {
                games_[i]->apply_move(move);
            }
            if (game_util::IsGameOver(*games_[0])) {
                addEvent(current_player) << "Ended the game and made the last move" << std::endl;
                break;
            }
            addEvent(current_player) << "Game state: " << games_[0]->encode() << std::endl;
        }
        auto res = games_[0]->get_territory();
        if (res.red_total != res.blue_total) {
            std::stringstream message;
            message << res.red_total << "-" << res.blue_total;
            return GameOutcome{res.red_total > res.blue_total ? PlayerColor::Red : PlayerColor::Blue, BY_TOTAL_AREA,
                               games_[0]->encode(), message.str()};
        }

        if (res.red_max != res.blue_max) {
            std::stringstream message;
            message << res.red_total << "(" << res.red_max << ")-" << res.blue_total << "(" << res.blue_max << ")";
            return GameOutcome{res.red_max > res.blue_max ? PlayerColor::Red : PlayerColor::Blue, BY_LARGEST_AREA,
                               games_[0]->encode(), message.str()};
        }

        std::stringstream message;
        message << res.red_total << "(" << res.red_max << ")-" << res.blue_total << "(" << res.blue_max << ")";
        if (current_player == 1) {
            return GameOutcome{PlayerColor::Blue, BY_LAST_PLACEMENT, games_[0]->encode(), message.str()};
        } else {
            return GameOutcome{PlayerColor::Red, BY_LAST_PLACEMENT, games_[0]->encode(), message.str()};
        }
    }

    void runWrapper() {
        auto outcome = run();
        int score = outcome.winner == PlayerColor::Red ? 100 : 0;
        std::stringstream res;
        res << score << "\t";

        std::string msg = outcome.winner == PlayerColor::Red ? "W-L" : "L-W";
        if (outcome.reason == BY_TOTAL_AREA) {
            res << "BY_TOTAL_AREA\t" << outcome.message;
        } else if (outcome.reason == BY_LARGEST_AREA) {
            res << "BY_LARGEST_AREA\t" << outcome.message;
        } else if (outcome.reason == BY_LAST_PLACEMENT) {
            res << "BY_LAST_PLACEMENT\t" << outcome.message;
        } else if (outcome.reason == OPPONENT_TLE) {
            res << "OPPONENT_TLE\t" << msg;
        } else if (outcome.reason == OPPONENT_ILLEGAL_MOVE) {
            res << "OPPONENT_ILLEGAL_MOVE\t" << msg;
        }

        res << "\t" << outcome.encoded_game << std::endl;
        addEvent(0) << res.str();
    }
};
}  // namespace wallgo

int main() {
    std::string secret = "A5v29CsgPI0ExImG";
    std::cout << secret << std::endl;

    auto t = std::chrono::steady_clock::now().time_since_epoch().count();
    std::mt19937 rng(t);

    std::stringstream log;

    wallgo::GameController controller(rng(), log);
    controller.runWrapper();
    // std::cout << "Game finished. Outputting the last line of the log." << std::endl;
    std::string prev_line, line;
    while (std::getline(log, line)) {
        prev_line = line;
    }

    std::cout << prev_line << std::endl;  // Output the last line of the log
    return 0;
}

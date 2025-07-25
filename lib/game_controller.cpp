#include "game_controller.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "types.h"

namespace wallgo {

double GameController::getTimeSinceLastEvent() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - last_time_;
    return diff.count();
}

bool GameController::subtractTimeAndCheckTimeLimit(int player, double time) {
    playersRemainingTime_[player] -= time;
    if (playersRemainingTime_[player] < 0) {
        addEvent(player) << "Ran out of time!" << std::endl;
        return true;
    }
    return false;
}

std::ostream& GameController::addEvent(int player) {
    last_time_ = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = last_time_ - start_time_;
    return output_data_ << "P" << player << "\t" << diff.count() << "s\t";
}

GameController::GameController(int seed, std::unique_ptr<Player> player1, std::unique_ptr<Player> player2,
                               std::ostream& output_data)
    : seed_(seed), output_data_(output_data), players_(3), games_(3), playersRemainingTime_(3, 1) {
    games_[0] = std::shared_ptr<Game>(new Game());
    games_[1] = std::shared_ptr<Game>(new Game());
    games_[2] = std::shared_ptr<Game>(new Game());

    addEvent(0) << "Initializing Game with seed " << seed << std::endl;

    start_time_ = std::chrono::steady_clock::now();
    addEvent(1) << "Initializing Player 1 (Red)" << std::endl;
    player1->init(PlayerColor::Red, games_[1], seed_);
    double player1_initialize_time = getTimeSinceLastEvent();
    if (subtractTimeAndCheckTimeLimit(1, player1_initialize_time)) {
        std::stringstream message;
        message << "Player 1 ran out of time while initializing";
        throw std::runtime_error(message.str());
    }
    addEvent(1) << "Initializing Player 1 completed in " << player1_initialize_time << "s" << std::endl;
    player1.swap(players_[1]);

    addEvent(2) << "Initializing Player 2 (Blue)" << std::endl;
    player2->init(PlayerColor::Blue, games_[2], seed_);
    double player2_initialize_time = getTimeSinceLastEvent();
    if (subtractTimeAndCheckTimeLimit(2, player2_initialize_time)) {
        std::stringstream message;
        message << "Player 2 ran out of time while initializing";
        throw std::runtime_error(message.str());
    }
    addEvent(2) << "Initializing Player 2 completed in " << player2_initialize_time << "s" << std::endl;
    player2.swap(players_[2]);
}

GameOutcome GameController::run() {
    // place the pieces, in order RBBRRBBR
    for (int i = 0; i < 8; ++i) {
        std::vector<Position> valid_positions;

        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                Position pos{r, c};
                if (!games_[0]->board().get(pos).piece()) {
                    valid_positions.push_back(pos);
                }
            }
        }

        Position pos;
        int current_player = (i == 0 || i == 3 || i == 4 || i == 7) ? 1 : 2;
        int current_piece_id = i / 2;
        pos = players_[current_player]->place(current_piece_id, valid_positions);
        double time_used = getTimeSinceLastEvent();
        if (subtractTimeAndCheckTimeLimit(current_player, time_used - 0.1)) {
            std::stringstream message;
            message << "Player " << current_player << " ran out of time while placing piece";
            return GameOutcome{static_cast<PlayerColor>(3 - current_player), OPPONENT_TLE, games_[0]->encode(),
                               message.str()};
        }
        addEvent(current_player) << "Took " << time_used << "s to place piece." << std::endl;
        addEvent(current_player) << "Placed piece at (" << pos.r << "," << pos.c << ")" << std::endl;

        if (std::find(valid_positions.begin(), valid_positions.end(), pos) == valid_positions.end()) {
            std::stringstream message;
            message << "Player " << current_player << " placed piece at an invalid position (" << pos.r << "," << pos.c
                    << ")";
            return GameOutcome{static_cast<PlayerColor>(3 - current_player), OPPONENT_ILLEGAL_MOVE, games_[0]->encode(),
                               message.str()};
        }

        for (int game = 0; game < 3; ++game) {
            // id is i/2
            games_[game]->place_piece(pos, static_cast<PlayerColor>(current_player), current_piece_id);
        }
    }

    // move
    bool game_ended = false;
    int current_player = 1;
    for (; !game_ended; current_player = 3 - current_player) {
        PlayerColor opponent_color = static_cast<PlayerColor>(3 - current_player);

        std::vector<Move>&& valid_moves =
            games_[0]->board().get_valid_moves(static_cast<PlayerColor>(current_player));  // note: won't be empty

        Move move = players_[current_player]->move(valid_moves);
        if (move.player() != static_cast<PlayerColor>(current_player)) {
            return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0]->encode(),
                               "Returned move does not have player set"};
        }
        Piece piece = games_[0]->board().get_piece(move.player(), move.piece_id());
        double time_used = getTimeSinceLastEvent();
        if (subtractTimeAndCheckTimeLimit(current_player, time_used - 0.1)) {
            std::stringstream message;
            message << "Player " << current_player << " ran out of time while making a move";
            return GameOutcome{opponent_color, OPPONENT_TLE, games_[0]->encode(), message.str()};
        }
        addEvent(current_player)
            << "Chose piece " << piece.id << " at (" << piece.pos.r << "," << piece.pos.c << "), "
            << "Number of steps: " << (move.direction1() ? 1 : 0) + (move.direction2() ? 1 : 0)
            << (move.direction1() ? ", Direction 1: " + std::to_string(static_cast<int>(*move.direction1())) : "")
            << (move.direction2() ? ", Direction 2: " + std::to_string(static_cast<int>(*move.direction2())) : "")
            << ", Wall direction: " << static_cast<int>(move.wall_placement_direction()) << std::endl;

        if (!games_[0]->board().is_move_legal(move)) {
            std::stringstream message;
            message << "Illegal move made by " << current_player;
            return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0]->encode(), message.str()};
        }
        for (int i = 0; i < 3; i++) {
            games_[i]->apply_move(move);
        }
        if (games_[0]->board().is_game_over()) {
            addEvent(current_player) << "Ended the game and made the last move" << std::endl;
            break;
        }
    }
    auto res = games_[0]->board().get_territory();
    if (res.red_total != res.blue_total) {
        std::stringstream message;
        message << "Total territories: " << res.red_total << "-" << res.blue_total;
        return GameOutcome{res.red_total > res.blue_total ? PlayerColor::Red : PlayerColor::Blue, BY_TOTAL_AREA,
                           games_[0]->encode(), message.str()};
    }

    if (res.red_max != res.blue_max) {
        std::stringstream message;
        message << res.red_max << "-" << res.blue_max;
        return GameOutcome{res.red_max > res.blue_max ? PlayerColor::Red : PlayerColor::Blue, BY_LARGEST_AREA,
                           games_[0]->encode(), message.str()};
    }

    if (current_player == 1) {
        return GameOutcome{PlayerColor::Blue, BY_LAST_PLACEMENT, games_[0]->encode(),
                           "Player 2 wins by last placement"};
    } else {
        return GameOutcome{PlayerColor::Red, BY_LAST_PLACEMENT, games_[0]->encode(), "Player 1 wins by last placement"};
    }
}

}  // namespace wallgo

#include "game_controller.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "types.h"

namespace RED {
std::unique_ptr<wallgo::Player> get();
}

namespace BLUE {
std::unique_ptr<wallgo::Player> get();
}

namespace wallgo {

double GameController::getTimeSinceLastEvent() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - last_time_;
    return diff.count();
}

std::ostream& GameController::addEvent(int player) {
    last_time_ = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = last_time_ - start_time_;
    return output_data_ << player << "\t" << diff.count() << "\t";
}

GameController::GameController(int seed, std::string preset, std::unique_ptr<Player> player1,
                               std::unique_ptr<Player> player2, std::ostream& output_data)
    : seed_(seed), output_data_(output_data), players_(3), games_(3) {
    games_[0] = std::shared_ptr<Game>(new Game(preset));
    games_[1] = std::shared_ptr<Game>(new Game(preset));
    games_[2] = std::shared_ptr<Game>(new Game(preset));

    start_time_ = std::chrono::steady_clock::now();
    addEvent(1) << "Initializing Player 1 (Red)" << std::endl;
    player1->init(PlayerColor::Red, games_[1], seed_);
    double player1_initialize_time = getTimeSinceLastEvent();
    addEvent(1) << "Initializing Player 1 completed in " << player1_initialize_time << "s" << std::endl;
    player1.swap(players_[1]);

    addEvent(2) << "Initializing Player 2 (Blue)" << std::endl;
    player2->init(PlayerColor::Blue, games_[2], seed_);
    double player2_initialize_time = getTimeSinceLastEvent();
    addEvent(2) << "Initializing Player 2 completed in " << player2_initialize_time << "s" << std::endl;
    player2.swap(players_[2]);
}

GameOutcome GameController::run() {
    bool game_ended = false;
    int current_player = 1;
    for (; !game_ended; current_player = 3 - current_player) {
        PlayerColor opponent_color = static_cast<PlayerColor>(3 - current_player);

        addEvent(current_player) << "Computing valid moves" << std::endl;
        std::vector<Move>&& valid_moves =
            game_util::GetValidMoves(games_[0]->board(), current_player);  // TODO: implement; won't be empty
        addEvent(current_player) << valid_moves.size() << " valid moves" << std::endl;

        Move move = players_[current_player]->move(valid_moves);
        if (move.player() != static_cast<PlayerColor>(current_player)) {
            return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0],
                               "Returned move does not have player set"};
        }
        Piece piece = move.piece();
        double time_used = getTimeSinceLastEvent();
        addEvent(current_player) << "Took " << time_used << "s." << std::endl;
        addEvent(current_player) << "Chose piece " << piece.id << " at (" << piece.pos.r << "," << piece.pos.c << ")"
                                 << std::endl;
        if (!game_util::IsMoveLegal(games_[0]->board(), move)) {  // TODO: implement
            std::stringstream message;
            message << "Illegal move made by " << current_player;
            return GameOutcome{opponent_color, OPPONENT_ILLEGAL_MOVE, games_[0], message.str()};
        }
        for (int i = 0; i < 3; i++) {
            games_[i]->apply_move(move);
        }
        if (game_util::IsGameOver(games_[0]->board())) {  // TODO: implement
            addEvent(current_player) << "Ended the game and made the last move" << std::endl;
            break;
        }
    }
    auto [territory1, territory2] = games_[0]->total_territory();
    if (territory1 != territory2) {
        std::stringstream message;
        message << territory1 << "-" << territory2;
        return GameOutcome{territory1 > territory2 ? PlayerColor::Red : PlayerColor::Blue, BY_TOTAL_AREA, games_[0],
                           message.str()};
    }

    std::tie(territory1, territory2) = games_[0]->max_territory();
    if (territory1 != territory2) {
        std::stringstream message;
        message << territory1 << "-" << territory2;
        return GameOutcome{territory1 > territory2 ? PlayerColor::Red : PlayerColor::Blue, BY_LARGEST_AREA, games_[0],
                           message.str()};
    }

    if (current_player == 1) {
        return GameOutcome{PlayerColor::Blue, BY_LAST_PLACEMENT, games_[0], "Player 2 wins by last placement"};
    } else {
        return GameOutcome{PlayerColor::Red, BY_LAST_PLACEMENT, games_[0], "Player 1 wins by last placement"};
    }
}

}  // namespace wallgo
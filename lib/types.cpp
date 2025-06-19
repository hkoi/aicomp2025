#include "types.h"

#include <sstream>

namespace wallgo {

// Cell implementation
Cell::Cell(Position pos, std::optional<Piece> piece, std::array<std::optional<WallType>, 4> walls)
    : piece_(piece), walls_(walls), pos_(pos) {}

std::optional<Piece> Cell::piece() const { return piece_; }

std::array<std::optional<WallType>, 4> Cell::walls() const { return walls_; }

std::optional<WallType> Cell::wall(Direction d) const { return walls_[static_cast<int>(d)]; }

Position Cell::pos() const { return pos_; }

// Move implementation
Move::Move(PlayerColor player, PieceId piece_id, std::optional<Direction> direction1,
           std::optional<Direction> direction2, Direction wall_placement_direction)
    : player_(player),
      piece_id_(piece_id),
      direction1_(direction1),
      direction2_(direction2),
      wall_placement_direction_(wall_placement_direction) {}

PlayerColor Move::player() const { return player_; }

PieceId Move::piece_id() const { return piece_id_; }

std::optional<Direction> Move::direction1() const { return direction1_; }

std::optional<Direction> Move::direction2() const { return direction2_; }

Direction Move::wall_placement_direction() const { return wall_placement_direction_; }

std::string Move::encode() const {
    unsigned int value = static_cast<unsigned int>(player_) - 1;

    value <<= 3;
    value |= piece_id_;

    value <<= 3;
    if (direction1_) value |= static_cast<unsigned int>(*direction1_) + 1;

    value <<= 3;
    if (direction2_) value |= static_cast<unsigned int>(*direction2_) + 1;

    value <<= 3;
    value |= static_cast<unsigned int>(wall_placement_direction_) + 1;

    std::string data;
    for (int i = 0; i < 3; ++i) {
        int part = value & 31;
        data += part >= 10 ? 'a' + part - 10 : '0' + part;
        value >>= 5;
    }

    return data;
}

// Game implementation
Game::Game() : board_(), history_() {
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            board_[r][c] = Cell({r, c}, std::nullopt, std::array<std::optional<WallType>, 4>{});
        }
    }
}

Board Game::board() const { return board_; }

std::vector<Move> Game::history() const { return history_; }

std::pair<int, int> Game::total_territory() const {
    // TODO: implement territory calculation
    return {0, 0};
}

std::pair<int, int> Game::max_territory() const {
    // TODO: implement max territory calculation
    return {0, 0};
}

void Game::apply_move(Move move) {
    // TODO: implement move application
    history_.push_back(move);
}

std::string Game::encode() const {
    std::string s;
    for (const auto& move : history_) {
        s += move.encode();
    }
    return s;
}

}  // namespace wallgo

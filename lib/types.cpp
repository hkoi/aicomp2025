#include "types.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <sstream>

#include "game_utils.h"

namespace wallgo {

// Position implementation

Position Position::move(std::optional<Direction> d) const {
    if (!d) return *this;
    switch (*d) {
        case Direction::Up:
            return {r - 1, c};
        case Direction::Down:
            return {r + 1, c};
        case Direction::Left:
            return {r, c - 1};
        case Direction::Right:
            return {r, c + 1};
    }
    return *this;  // Default case, should not happen
}

bool Position::operator==(Position other) const { return r == other.r && c == other.c; }

// Cell implementation
Cell::Cell(Position pos, std::optional<Piece> piece, std::array<WallType, 4> walls)
    : piece_(piece), walls_(walls), pos_(pos) {}

std::optional<Piece> Cell::piece() const { return piece_; }

std::array<WallType, 4> Cell::walls() const { return walls_; }

WallType Cell::wall(Direction d) const { return walls_[static_cast<int>(d)]; }

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
            board_[r][c] = Cell({r, c});
        }
    }
}

Board Game::board() const { return board_; }

std::vector<Move> Game::history() const { return history_; }

Game::GetTerritoryResult Game::get_territory() const {
    std::array<std::array<bool, 7>, 7> visited = {};
    const auto &board = board_;

    GetTerritoryResult res = {0, 0, 0, 0};

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (visited[r][c]) continue;

            Cell cell = board[r][c];
            if (!cell.piece()) continue;

            std::queue<Position> queue;
            queue.push(cell.pos());
            visited[r][c] = true;

            int territory_count = 1;
            bool single_color = true;
            PlayerColor color = cell.piece()->owner;

            while (!queue.empty()) {
                Position pos = queue.front();
                queue.pop();

                if (board[pos.r][pos.c].piece()) {
                    if (board[pos.r][pos.c].piece()->owner != color) {
                        single_color = false;
                    }
                }

                for (Cell neighbor : get_accessible_neighbors(pos)) {
                    if (!visited[neighbor.pos().r][neighbor.pos().c]) {
                        visited[neighbor.pos().r][neighbor.pos().c] = true;
                        queue.push(neighbor.pos());
                        territory_count++;
                    }
                }
            }

            if (!single_color) continue;

            if (color == PlayerColor::Red) {
                res.red_total += territory_count;
                res.red_max = std::max(res.red_max, territory_count);
            } else if (color == PlayerColor::Blue) {
                res.blue_total += territory_count;
                res.blue_max = std::max(res.blue_max, territory_count);
            }
        }
    }
    return res;
}

void Game::apply_move(Move move) {
    board_ = game_util::ApplyMove(*this, move);
    history_.push_back(move);
}

void Game::place_piece(Position pos, PlayerColor player, PieceId piece_id) {
    Piece piece{player, pos, piece_id};
    board_[pos.r][pos.c] = Cell(pos, piece);
    placements_.push_back(piece);
}

std::string Game::encode() const {
    std::string s;
    for (const auto &piece : placements_) {
        std::stringstream ss;
        ss << piece.pos.r << piece.pos.c << static_cast<int>(piece.owner) << piece.id;
        s += ss.str();
    }
    s += '_';
    for (const auto &move : history_) {
        s += move.encode();
    }
    return s;
}

std::vector<Cell> Game::get_accessible_neighbors(Position pos) const {
    std::vector<Cell> neighbors;
    for (auto dir : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        Position neighbor_pos = pos.move(dir);
        if (neighbor_pos.r >= 0 && neighbor_pos.r < 7 && neighbor_pos.c >= 0 && neighbor_pos.c < 7) {
            if (board_[pos.r][pos.c].wall(dir) == WallType::None) {
                neighbors.push_back(board_[neighbor_pos.r][neighbor_pos.c]);
            }
        }
    }
    return neighbors;
}

std::vector<Piece> Game::get_pieces(PlayerColor player) const {
    std::vector<Piece> pieces;
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = board_[r][c];
            if (cell.piece() && cell.piece()->owner == player) {
                pieces.push_back(*cell.piece());
            }
        }
    }
    std::sort(pieces.begin(), pieces.end(), [](const Piece &a, const Piece &b) { return a.id < b.id; });
    return pieces;
}

Piece Game::get_piece(PlayerColor player, PieceId pieceId) const {
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = board_[r][c];
            if (cell.piece()) {
                const Piece &piece = *cell.piece();
                if (piece.id == pieceId && piece.owner == player) {
                    return piece;
                }
            }
        }
    }
    throw std::runtime_error("No piece with id exists");
}

}  // namespace wallgo

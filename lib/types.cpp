#include "types.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <sstream>

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

// Board implementation

Cell Board::get(Position pos) const {
    if (pos.r < 0 || pos.r >= 7 || pos.c < 0 || pos.c >= 7) {
        throw std::out_of_range("Position out of bounds");
    }
    return board_[pos.r][pos.c];
}

void Board::set(Cell c) {
    Position pos = c.pos();
    if (pos.r < 0 || pos.r >= 7 || pos.c < 0 || pos.c >= 7) {
        throw std::out_of_range("Position out of bounds");
    }
    board_[pos.r][pos.c] = c;
}

std::vector<Cell> Board::get_accessible_neighbors(Position pos) const {
    std::vector<Cell> neighbors;
    for (auto dir : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        Position neighbor_pos = pos.move(dir);
        if (neighbor_pos.r >= 0 && neighbor_pos.r < 7 && neighbor_pos.c >= 0 && neighbor_pos.c < 7) {
            if (get(pos).wall(dir) == WallType::None) {
                neighbors.push_back(get(neighbor_pos));
            }
        }
    }
    return neighbors;
}

Piece Board::get_piece(PlayerColor player, PieceId pieceId) const {
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = get({r, c});
            if (cell.piece() && cell.piece()->owner == player && cell.piece()->id == pieceId) {
                return *cell.piece();
            }
        }
    }
    throw std::runtime_error("No piece with id exists");
}

std::vector<Piece> Board::get_pieces(PlayerColor player) const {
    std::vector<Piece> pieces;
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = get({r, c});
            if (cell.piece() && cell.piece()->owner == player) {
                pieces.push_back(*cell.piece());
            }
        }
    }
    std::sort(pieces.begin(), pieces.end(), [](const Piece &a, const Piece &b) { return a.id < b.id; });
    return pieces;
}

Board::GetTerritoryResult Board::get_territory() const {
    std::array<std::array<bool, 7>, 7> visited = {};

    GetTerritoryResult res = {0, 0, 0, 0};

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (visited[r][c]) continue;

            Cell cell = get({r, c});
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

                if (get(pos).piece()) {
                    if (get(pos).piece()->owner != color) {
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
bool Board::is_move_legal(const Move &move) const {
    Position pos = get_piece(move.player(), move.piece_id()).pos;

    if (pos.r < 0 || pos.r >= 7 || pos.c < 0 || pos.c >= 7) {
        return false;  // Move is out of bounds
    }

    if (move.player() != get(pos).piece()->owner) {
        return false;  // Player does not own the piece at the position
    }

    if (!get(pos).piece() || get(pos).piece()->id != move.piece_id()) {
        return false;  // Piece ID does not match the piece at the position
    }

    if (move.direction2() && !move.direction1()) {
        return false;  // Cannot have a second direction without a first
    }

    std::vector<Direction> directions;
    if (move.direction1()) {
        directions.push_back(*move.direction1());
    }
    if (move.direction2()) {
        directions.push_back(*move.direction2());
    }

    for (const auto &dir : directions) {
        if (get(pos).wall(dir) != WallType::None) {
            return false;  // Cannot move through a wall
        }

        Position new_pos = pos.move(dir);
        if (new_pos.r < 0 || new_pos.r >= 7 || new_pos.c < 0 || new_pos.c >= 7) {
            return false;  // Move is out of bounds
        }
        if (get(new_pos).piece()) {
            return false;  // Cannot move to a cell that already has a piece
        }

        pos = new_pos;
    }
    if (get(pos).wall(move.wall_placement_direction()) != WallType::None) {
        return false;  // Cannot place a wall in a direction that already has a wall
    }

    return true;
}

std::vector<Move> Board::get_valid_moves(PlayerColor player) const {
    std::vector<Move> valid_moves;
    Direction all_directions[] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = get({r, c});

            if (!cell.piece() || cell.piece()->owner != player) {
                continue;
            }
            PieceId piece_id = cell.piece()->id;

            for (auto wall_dir : all_directions) {
                Move move(player, piece_id, std::nullopt, std::nullopt, wall_dir);
                if (is_move_legal(move)) {
                    valid_moves.push_back(move);
                }

                for (auto dir1 : all_directions) {
                    Move move(player, piece_id, dir1, std::nullopt, wall_dir);
                    if (is_move_legal(move)) {
                        valid_moves.push_back(move);
                    }
                    for (auto dir2 : all_directions) {
                        Move move(player, piece_id, dir1, dir2, wall_dir);
                        if (is_move_legal(move)) {
                            valid_moves.push_back(move);
                        }
                    }
                }
            }
        }
    }

    return valid_moves;
}

Board Board::apply_move(const Move &move) const {
    if (!is_move_legal(move)) {
        throw std::runtime_error("Illegal move");
    }

    Board new_board = *this;

    Position pos = get_piece(move.player(), move.piece_id()).pos, new_pos = pos;
    Piece piece = *new_board.get(pos).piece();

    // Move the piece
    if (move.direction1()) {
        new_pos = new_pos.move(*move.direction1());
    }
    if (move.direction2()) {
        new_pos = new_pos.move(*move.direction2());
    }

    // Update the piece position and walls
    new_board.set(Cell(pos, std::nullopt, get(pos).walls()));

    auto new_walls = new_board.get(new_pos).walls();
    new_walls[static_cast<int>(move.wall_placement_direction())] =
        (piece.owner == PlayerColor::Red) ? WallType::PlayerRed : WallType::PlayerBlue;
    piece.pos = new_pos;
    new_board.set(Cell(new_pos, piece, new_walls));

    // Update the walls on the opposite cell of destination
    Position opposite_pos = new_pos.move(move.wall_placement_direction());
    auto new_opposite_walls = new_board.get(opposite_pos).walls();
    int opposite_dir = static_cast<int>(move.wall_placement_direction()) ^ 1;
    new_opposite_walls[opposite_dir] = (piece.owner == PlayerColor::Red) ? WallType::PlayerRed : WallType::PlayerBlue;
    new_board.set(Cell(opposite_pos, new_board.get(opposite_pos).piece(), new_opposite_walls));

    return new_board;
}

bool Board::is_game_over() const {
    // check if any red pieces can reach blue pieces
    std::array<std::array<bool, 7>, 7> visited = {};

    for (auto piece : get_pieces(PlayerColor::Red)) {
        Position pos = piece.pos;

        // use BFS to check if any red piece can reach a blue piece
        std::queue<Position> queue;
        queue.push(pos);
        visited[pos.r][pos.c] = true;
        while (!queue.empty()) {
            Position current = queue.front();
            queue.pop();

            for (Cell neighbor : get_accessible_neighbors(current)) {
                if (visited[neighbor.pos().r][neighbor.pos().c]) continue;
                visited[neighbor.pos().r][neighbor.pos().c] = true;

                if (neighbor.piece() && neighbor.piece()->owner == PlayerColor::Blue) {
                    return false;  // Red can reach Blue
                }

                queue.push(neighbor.pos());
            }
        }
    }

    return true;
}

// Game implementation
Game::Game() : board_(), history_() {
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            board_.set(Cell({r, c}, std::nullopt,
                            {r == 0 ? WallType::Border : WallType::None, r == 6 ? WallType::Border : WallType::None,
                             c == 0 ? WallType::Border : WallType::None, c == 6 ? WallType::Border : WallType::None}));
        }
    }
}

Board Game::board() const { return board_; }

std::vector<Move> Game::history() const { return history_; }

void Game::apply_move(Move move) {
    board_ = board_.apply_move(move);
    history_.push_back(move);
}

void Game::place_piece(Position pos, PlayerColor player, PieceId piece_id) {
    Piece piece{player, pos, piece_id};
    board_.set(Cell(pos, piece, board_.get(pos).walls()));
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

}  // namespace wallgo

namespace std {

ostream &operator<<(ostream &os, const wallgo::Move &move) {
    os << "Move(player=" << static_cast<int>(move.player()) << ", piece_id=" << move.piece_id()
       << ", direction1=" << (move.direction1() ? std::to_string(static_cast<int>(*move.direction1())) : "None")
       << ", direction2=" << (move.direction2() ? std::to_string(static_cast<int>(*move.direction2())) : "None")
       << ", wall_placement_direction=" << static_cast<int>(move.wall_placement_direction()) << ")";
    return os;
}
ostream &operator<<(ostream &os, const wallgo::Board &board) {
    using namespace wallgo;
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = board.get({r, c});
            os << '+';
            if (cell.wall(Direction::Up) != WallType::None) {
                os << "---";
            } else {
                os << "   ";
            }
        }
        os << "+\n";
        for (int c = 0; c < 7; ++c) {
            const Cell &cell = board.get({r, c});
            if (cell.wall(Direction::Left) != WallType::None) {
                os << '|';
            } else {
                os << ' ';
            }
            os << ' ';
            if (cell.piece()) {
                if (cell.piece()->owner == PlayerColor::Red) {
                    os << (char)('A' + cell.piece()->id);
                } else {
                    os << (char)('a' + cell.piece()->id);
                }
            } else {
                os << '.';
            }
            os << ' ';
        }
        cout << "|\n";
    }
    for (int c = 0; c < 7; ++c) {
        const Cell &cell = board.get({6, c});
        os << '+';
        if (cell.wall(Direction::Down) != WallType::None) {
            os << "---";
        } else {
            os << "   ";
        }
    }
    os << "+\n";
    return os;
}

}  // namespace std
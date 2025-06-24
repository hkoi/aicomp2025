#include "game_utils.h"

#include <queue>

#include "types.h"

namespace game_util {

bool IsMoveLegal(const Game& game, const Move& move) {
    const auto& board = game.board();
    Position pos = game.get_piece(move.player(), move.piece_id()).pos;

    if (pos.r < 0 || pos.r >= 7 || pos.c < 0 || pos.c >= 7) {
        return false;  // Move is out of bounds
    }

    if (move.player() != board[pos.r][pos.c].piece()->owner) {
        return false;  // Player does not own the piece at the position
    }

    if (!board[pos.r][pos.c].piece() || board[pos.r][pos.c].piece()->id != move.piece_id()) {
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

    for (const auto& dir : directions) {
        if (board[pos.r][pos.c].wall(dir) != WallType::None) {
            return false;  // Cannot move through a wall
        }

        Position new_pos = pos.move(dir);
        if (new_pos.r < 0 || new_pos.r >= 7 || new_pos.c < 0 || new_pos.c >= 7) {
            return false;  // Move is out of bounds
        }
        if (board[new_pos.r][new_pos.c].piece()) {
            return false;  // Cannot move to a cell that already has a piece
        }

        pos = new_pos;
    }
    if (board[pos.r][pos.c].wall(move.wall_placement_direction()) != WallType::None) {
        return false;  // Cannot place a wall in a direction that already has a wall
    }

    Position new_pos = pos.move(move.wall_placement_direction());
    if (new_pos.r < 0 || new_pos.r >= 7 || new_pos.c < 0 || new_pos.c >= 7) {
        return false;  // Move is out of bounds
    }

    return true;
}

std::vector<Move> GetValidMoves(const Game& game, PlayerColor player) {
    const auto& board = game.board();
    std::vector<Move> valid_moves;
    Direction all_directions[] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell& cell = board[r][c];

            if (!cell.piece() || cell.piece()->owner != player) {
                continue;
            }
            PieceId piece_id = cell.piece()->id;

            for (auto wall_dir : all_directions) {
                Move move(player, piece_id, std::nullopt, std::nullopt, wall_dir);
                if (IsMoveLegal(game, move)) {
                    valid_moves.push_back(move);
                }

                for (auto dir1 : all_directions) {
                    Move move(player, piece_id, dir1, std::nullopt, wall_dir);
                    if (IsMoveLegal(game, move)) {
                        valid_moves.push_back(move);
                    }
                    for (auto dir2 : all_directions) {
                        Move move(player, piece_id, dir1, dir2, wall_dir);
                        if (IsMoveLegal(game, move)) {
                            valid_moves.push_back(move);
                        }
                    }
                }
            }
        }
    }

    return valid_moves;
}

Board ApplyMove(const Game& game, const Move& move) {
    Board board = game.board(), new_board = board;

    Position pos = game.get_piece(move.player(), move.piece_id()).pos, new_pos = pos;
    Piece piece = *new_board[pos.r][pos.c].piece();

    // Move the piece
    if (move.direction1()) {
        new_pos = new_pos.move(*move.direction1());
    }
    if (move.direction2()) {
        new_pos = new_pos.move(*move.direction2());
    }

    // Update the piece position and walls
    new_board[pos.r][pos.c] = Cell(pos, std::nullopt, board[pos.r][pos.c].walls());

    auto new_walls = new_board[new_pos.r][new_pos.c].walls();
    new_walls[static_cast<int>(move.wall_placement_direction())] =
        (piece.owner == PlayerColor::Red) ? WallType::PlayerRed : WallType::PlayerBlue;
    piece.pos = new_pos;
    new_board[new_pos.r][new_pos.c] = Cell(new_pos, piece, new_walls);

    // Update the walls on the opposite cell of destination
    Position opposite_pos = new_pos.move(move.wall_placement_direction());
    auto new_opposite_walls = new_board[opposite_pos.r][opposite_pos.c].walls();
    int opposite_dir = static_cast<int>(move.wall_placement_direction()) ^ 1;
    new_opposite_walls[opposite_dir] = (piece.owner == PlayerColor::Red) ? WallType::PlayerRed : WallType::PlayerBlue;
    new_board[opposite_pos.r][opposite_pos.c] =
        Cell(opposite_pos, new_board[opposite_pos.r][opposite_pos.c].piece(), new_opposite_walls);

    return new_board;
}

bool IsGameOver(const Game& game) {
    // check if any red pieces can reach blue pieces
    std::array<std::array<bool, 7>, 7> visited = {};

    for (auto piece : game.get_pieces(PlayerColor::Red)) {
        Position pos = piece.pos;

        // use BFS to check if any red piece can reach a blue piece
        std::queue<Position> queue;
        queue.push(pos);
        visited[pos.r][pos.c] = true;
        while (!queue.empty()) {
            Position current = queue.front();
            queue.pop();

            for (Cell neighbor : game.get_accessible_neighbors(current)) {
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

}  // namespace game_util
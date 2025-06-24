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

namespace wallgo {

using PieceId = int;

enum class PlayerColor { Red = 1, Blue = 2 };

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

enum class WallType { None, PlayerRed, PlayerBlue, Edge };

struct Position {
    int r;
    int c;
    Position move(std::optional<Direction> d) const;
    bool operator==(Position other) const;
};

struct Piece {
    PlayerColor owner;
    Position pos;
    PieceId id;
};

class Cell {
   private:
    Position pos_;
    std::optional<Piece> piece_;
    std::array<WallType, 4> walls_;

   public:
    Cell(Position pos = {0, 0}, std::optional<Piece> piece = std::nullopt,
         std::array<WallType, 4> walls = {WallType::None, WallType::None, WallType::None, WallType::None});
    std::optional<Piece> piece() const;
    std::array<WallType, 4> walls() const;
    WallType wall(Direction d) const;
    Position pos() const;
};

class Move {
   private:
    PlayerColor player_;
    PieceId piece_id_;
    std::optional<Direction> direction1_;
    std::optional<Direction> direction2_;
    Direction wall_placement_direction_;

   public:
    Move(PlayerColor player, PieceId piece_id, std::optional<Direction> direction1, std::optional<Direction> direction2,
         Direction wall_placement_direction);
    PlayerColor player() const;
    PieceId piece_id() const;
    std::optional<Direction> direction1() const;
    std::optional<Direction> direction2() const;
    Direction wall_placement_direction() const;
    std::string encode() const;  // Encode move as string
};

using Board = std::array<std::array<Cell, 7>, 7>;

class Game {
   private:
    Board board_;
    std::vector<Piece> placements_;
    std::vector<Move> history_;

   public:
    Game();
    Board board() const;
    std::vector<Move> history() const;

    struct GetTerritoryResult {
        int red_total, red_max, blue_total, blue_max;
    };
    GetTerritoryResult get_territory() const;

    void place_piece(Position pos, PlayerColor player, PieceId piece_id);
    void apply_move(Move move);
    std::string encode() const;  // Encode move history as string
    std::vector<Cell> get_accessible_neighbors(Position pos) const;
    std::vector<Piece> get_pieces(PlayerColor player) const;
    Piece get_piece(PlayerColor player, PieceId piece) const;
};

class Player {
   public:
    virtual void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) = 0;
    virtual Position place(const std::vector<Position>& valid_positions) = 0;
    virtual Move move(const std::vector<Move>& valid_moves) = 0;
    virtual ~Player() = default;
};

enum Reason { BY_TOTAL_AREA, BY_LARGEST_AREA, BY_LAST_PLACEMENT, OPPONENT_TLE, OPPONENT_ILLEGAL_MOVE };
struct GameOutcome {
    PlayerColor winner;
    Reason reason;
    std::string encoded_game;
    std::string message;
};

}  // namespace wallgo

using namespace wallgo;

namespace game_util {

bool IsMoveLegal(const Game& game, const Move& move);
std::vector<Move> GetValidMoves(const Game& game, PlayerColor player);
Board ApplyMove(const Game& game, const Move& move);
bool IsGameOver(const Game& game);

}  // namespace game_util

namespace red {
std::unique_ptr<Player> get();
}

namespace blue {
std::unique_ptr<Player> get();
}

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
    const auto& board = board_;

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
    for (const auto& piece : placements_) {
        std::stringstream ss;
        ss << piece.pos.r << piece.pos.c << static_cast<int>(piece.owner) << piece.id;
        s += ss.str();
    }
    s += '_';
    for (const auto& move : history_) {
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
            const Cell& cell = board_[r][c];
            if (cell.piece() && cell.piece()->owner == player) {
                pieces.push_back(*cell.piece());
            }
        }
    }
    std::sort(pieces.begin(), pieces.end(), [](const Piece& a, const Piece& b) { return a.id < b.id; });
    return pieces;
}

Piece Game::get_piece(PlayerColor player, PieceId pieceId) const {
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Cell& cell = board_[r][c];
            if (cell.piece()) {
                const Piece& piece = *cell.piece();
                if (piece.id == pieceId && piece.owner == player) {
                    return piece;
                }
            }
        }
    }
    throw std::runtime_error("No piece with id exists");
}

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

/*
example output:

A5v29CsgPI0ExImG
0       0.0237958       0       BY_LARGEST_AREA 22(10)-22(11) 53105220132112110612102260235613_3l0...
*/
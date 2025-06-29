#ifndef WALLGO_TYPES_H
#define WALLGO_TYPES_H

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wallgo {

using PieceId = int;

enum class PlayerColor { Red = 1, Blue = 2 };

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

enum class WallType { None = 0, PlayerRed = 1, PlayerBlue = 2, Border = 3 };

struct Position {
    int r;
    int c;

    // Moves the current position in the specified direction, if provided.
    Position move(std::optional<Direction> d) const;
    bool operator==(Position other) const;
};

struct Piece {
    PlayerColor owner;
    Position pos;
    PieceId id;
};

// Represents a cell on the board, which can contain a piece and walls in four directions.
// The walls are represented as an array of WallType, where the index corresponds to the direction.
// The position of the cell is also stored.
class Cell {
   private:
    Position pos_;
    std::optional<Piece> piece_;
    std::array<WallType, 4> walls_;

   public:
    Cell(Position pos = {0, 0}, std::optional<Piece> piece = std::nullopt,
         std::array<WallType, 4> walls = {WallType::None, WallType::None, WallType::None, WallType::None});

    // Returns the piece in this cell, if any.
    std::optional<Piece> piece() const;

    // Returns the walls in this cell.
    std::array<WallType, 4> walls() const;

    // Returns the wall type in the specified direction.
    WallType wall(Direction d) const;

    // Returns the position of this cell.
    Position pos() const;
};

// Represents a move in the game, which includes the player, piece ID, directions for movement, and wall placement
// direction. A valid move has 0, 1, or 2 movement directions, where if direction1 is null, direction2 must also be
// null. The wall placement direction is always required.
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

    // Getters for move properties
    PlayerColor player() const;
    PieceId piece_id() const;
    std::optional<Direction> direction1() const;
    std::optional<Direction> direction2() const;
    Direction wall_placement_direction() const;

    // Encode move as string
    std::string encode() const;
};

// Represents the game board, which is a 7x7 grid of cells.
class Board {
   private:
    std::array<std::array<Cell, 7>, 7> board_;

   public:
    Board() = default;

    // Gets the cell at the specified position. If the position is out of bounds, throws std::out_of_range.
    Cell get(Position pos) const;

    // Updates cell at c.pos() with c. If c.pos() is out of bounds, throws std::out_of_range.
    void set(Cell c);

    // Returns the immediate neighbors of the cell (up, down, left, right) which is accessible, i.e. not blocked by
    // other pieces or walls.
    std::vector<Cell> get_accessible_neighbors(Position pos) const;

    // Returns the piece with the specified player and piece ID. Throws std::runtime_error if the piece does not exist.
    Piece get_piece(PlayerColor player, PieceId pieceId) const;
    std::vector<Piece> get_pieces(PlayerColor player) const;

    // Returns the total area controlled by each player and the maximum area of a single piece for each player.
    struct GetTerritoryResult {
        int red_total, red_max, blue_total, blue_max;
    };
    GetTerritoryResult get_territory() const;

    // Checks if the move is legal according to the game rules.
    bool is_move_legal(const Move& move) const;

    // Returns a list of valid moves for the specified player.
    std::vector<Move> get_valid_moves(PlayerColor player) const;

    // Returns a new Board instance with the updated state after applying the move.
    // The move must be legal, otherwise it throws std::runtime_error.
    // The move DOES NOT modify the current Board instance.
    Board apply_move(const Move& move) const;

    // Checks if the game is over according to the game rules.
    bool is_game_over() const;
};

// Represents the game state, including the board, piece placements, and move history.
class Game {
   private:
    Board board_;
    std::vector<Piece> placements_;
    std::vector<Move> history_;

   public:
    Game();

    // Returns the board of the game.
    Board board() const;

    // Returns the history of moves made in the game.
    std::vector<Move> history() const;

    // Places a piece at the specified position for the specified player with the given piece ID.
    // This updates the board and adds the piece to the placements list.
    void place_piece(Position pos, PlayerColor player, PieceId piece_id);

    // Applies a move to the game state, updating the board and adding the move to the history.
    // The move must be legal, otherwise it throws std::runtime_error.
    void apply_move(Move move);

    // Encodes the game state as a string, which includes the board and move history.
    std::string encode() const;
};

// Abstract interface for players in the game which you should implement.
// Each player must implement the init, place, and move methods.
// You MUST NOT modify the game state directly, i.e. you cannot call Game::apply_move or Game::place_piece directly.
// Instead, you should return a Move or Position from the place and move methods, which will be applied by the game
// controller.
class Player {
   public:
    // Implement this method to initialize the player. You should save the player color, game state, and seed for later
    // use.
    virtual void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) = 0;

    // Implement this method to place the piece at one of the valid positions. You should choose a position from the
    // provided list. Return the position where you want to place the piece.
    virtual Position place(PieceId pieceId, const std::vector<Position>& valid_positions) = 0;

    // Implement this method to make a move. You should choose a move from the provided list of valid moves.
    // Return the move you want to make.
    virtual Move move(const std::vector<Move>& valid_moves) = 0;

    // Virtual destructor to ensure proper cleanup of derived classes. No need to care.
    virtual ~Player() = default;
};

enum Reason {
    BY_TOTAL_AREA,
    BY_LARGEST_AREA,    // When total area ties
    BY_LAST_PLACEMENT,  // When largest area also ties
    OPPONENT_TLE,
    OPPONENT_ILLEGAL_MOVE
};

struct GameOutcome {
    PlayerColor winner;
    Reason reason;
    std::string encoded_game;
    std::string message;
};

}  // namespace wallgo

namespace std {
ostream& operator<<(ostream& os, const wallgo::Move& move);
ostream& operator<<(ostream& os, const wallgo::Board& board);
}  // namespace std

#endif  // WALLGO_TYPES_H
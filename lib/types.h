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
    std::vector<Cell> placements_;
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

#endif  // WALLGO_TYPES_H
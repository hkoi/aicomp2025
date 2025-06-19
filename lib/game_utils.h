#ifndef GAME_UTIL_H
#define GAME_UTIL_H
#include "types.h"

using std::shared_ptr;
using namespace wallgo;

namespace game_util {

bool IsMoveLegal(const Board& game, const Move& move);
std::vector<Move> GetValidMoves(const Board& game, int player);
Board ApplyMove(const Board& board, const Move& move);
bool IsGameOver(const Board& board);

}  // namespace game_util
#endif

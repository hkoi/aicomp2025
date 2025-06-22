#ifndef GAME_UTIL_H
#define GAME_UTIL_H
#include "types.h"

using std::shared_ptr;
using namespace wallgo;

namespace game_util {

bool IsMoveLegal(const Game& game, const Move& move);
std::vector<Move> GetValidMoves(const Game& game, PlayerColor player);
Board ApplyMove(const Game& game, const Move& move);
bool IsGameOver(const Game& game);

}  // namespace game_util
#endif

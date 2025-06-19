#ifndef WALLGO_GAME_CONTROLLER_H
#define WALLGO_GAME_CONTROLLER_H

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "types.h"

namespace wallgo {

class GameController {
   private:
    std::vector<std::unique_ptr<Player>> players_;
    std::vector<std::shared_ptr<Game>> games_;
    std::ostream& output_data_;
    int seed_;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_time_;
    std::chrono::steady_clock steady_clock_;

    double getTimeSinceLastEvent() const;
    std::ostream& addEvent(int player);

   public:
    GameController(int seed, std::unique_ptr<Player> p1, std::unique_ptr<Player> p2, std::ostream& output_data);
    GameOutcome run();
};

}  // namespace wallgo

#endif  // WALLGO_GAME_CONTROLLER_H
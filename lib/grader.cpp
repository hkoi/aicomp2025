#include <iomanip>

#include "game_controller.h"
#include "types.h"

namespace red {
std::unique_ptr<wallgo::Player> get();
}

namespace blue {
std::unique_ptr<wallgo::Player> get();
}

int main() {
    // Initialize the game controller with players and seed
    int seed = 42;
    std::unique_ptr<wallgo::Player> player1 = red::get();
    std::unique_ptr<wallgo::Player> player2 = blue::get();

    std::cout << std::fixed << std::setprecision(5);

    wallgo::GameController controller(seed, std::move(player1), std::move(player2), std::cout);

    // Run the game
    wallgo::GameOutcome outcome = controller.run();

    // Output the result
    std::cout << "Winner: " << static_cast<int>(outcome.winner) << ", Reason: " << static_cast<int>(outcome.reason)
              << ", Message: " << outcome.message << std::endl;

    std::cout << "\n\ngame string:\n" << outcome.encoded_game << std::endl;
    return 0;
}
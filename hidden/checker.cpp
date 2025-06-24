#include <iostream>
#include <string>

int main() {
    std::string secret;
    std::cin >> secret;
    if (secret != "A5v29CsgPI0ExImG") {
        std::cerr << "Invalid secret key." << std::endl;
        return 1;
    }

    int score;
    std::string reason, res_str, encoded_game;
    std::cin >> score >> reason >> res_str >> encoded_game;

    std::cout << score << "\t" << reason << "\t" << res_str << "\t" << encoded_game << std::endl;
}

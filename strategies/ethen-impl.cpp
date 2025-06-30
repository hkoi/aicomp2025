#include <bits/stdc++.h>
#include <random>

#ifdef ONLINE_JUDGE
#include "aicomp.h"
#else
#include "../lib/types.h"
#endif

using namespace wallgo;

namespace {
class EthenImpl : public Player {
   private:
    const float INF = 13;
    std::mt19937 rng;
    std::shared_ptr<const Game> game;
    PlayerColor player;
    PlayerColor opponent;
	int place_count = 0;

	void bfs(const Board& board, PlayerColor player, std::array<std::array<float, 7>, 7>& dist) {
		std::queue<Position> Q;
		for (auto piece: board.get_pieces(player)) {
			Q.push(piece.pos);
			dist[piece.pos.r][piece.pos.c] = 0;
		}
		while (!Q.empty()) {
			Position pos = Q.front();
			Q.pop();

			for (Cell c: board.get_accessible_neighbors(pos)) {
				Position new_pos = c.pos();
				if (dist[new_pos.r][new_pos.c] == INF) {
					dist[new_pos.r][new_pos.c] = dist[pos.r][pos.c] + 1;
					Q.push(new_pos);
				}
			}
		}
	}

	const float GURANTEED_SELF_WIN_SCORE = 0.9;
	const float GURANTEED_OPPONENT_WIN_SCORE = -0.85;

	const float SEMI_GURANTEED_SELF_WIN_SCORE = 0.8;
	const float SEMI_GURANTEED_OPPONENT_WIN_SCORE = -0.75;

	const int CONSIDER_BOUNDARY = 6;
	const float NOT_GUARANTEED_WEIGHTING = 0.90;

	const int TOP_K = 5;

	float lookup_score(int dist_player, int dist_opponent) {
		if (dist_player == dist_opponent) return 0.0;
		if (dist_opponent == INF || dist_player == 0) return GURANTEED_SELF_WIN_SCORE;
		if (dist_player == INF || dist_opponent == 0) return GURANTEED_OPPONENT_WIN_SCORE;

		if (dist_player >= CONSIDER_BOUNDARY && dist_opponent >= CONSIDER_BOUNDARY) {
			return 0;
		}
		if (dist_player >= CONSIDER_BOUNDARY) {
			return SEMI_GURANTEED_OPPONENT_WIN_SCORE;
		}
		if (dist_opponent >= CONSIDER_BOUNDARY) {
			return SEMI_GURANTEED_SELF_WIN_SCORE;
		}

		// dist_player lower: more chance to win this cell
		// dist_opponent lower: more chance to lose this cell

		return (dist_opponent - dist_player) / (float)(std::max(dist_opponent, dist_player)) * NOT_GUARANTEED_WEIGHTING;
	}

	float calculate_state_score(const Board& board) {
		std::array<std::array<float, 7>, 7> dist_player = {};
		std::array<std::array<float, 7>, 7> dist_opponent = {};
		for (int i = 0; i < 7; i++) {
			for (int j = 0; j < 7; j++) {
				dist_player[i][j] = INF;
				dist_opponent[i][j] = INF;
			}
		}

		bfs(board, player, dist_player);
		bfs(board, opponent, dist_opponent);
		
		float score = 0;
		
		for (int i = 0; i < 7; i++) {
			for (int j = 0; j < 7; j++) {
				float cell_score = lookup_score(dist_player[i][j], dist_opponent[i][j]);
				score += cell_score;
			}
		}
		return score;
		

	}

   public:
    void init(PlayerColor player, std::shared_ptr<const Game> game, int seed) override {
        rng.seed(seed);
        this->game = game;
        this->player = player;
        this->opponent = (player == PlayerColor::Red ? PlayerColor::Blue : PlayerColor::Red);
    }

    Position place(PieceId pieceId, const std::vector<Position>& valid_positions) override {
		// hard code first move
		if (player == wallgo::PlayerColor::Red && place_count == 0) {
			place_count++;
			return Position{2, 2};
		}
		auto valid_positions_copy = valid_positions;
		std::shuffle(valid_positions_copy.begin(), valid_positions_copy.end(), rng);
		
		Position best_position = valid_positions[0];
		float best_score = -1e9;

		for (const Position& current_position : valid_positions_copy) {
			Board new_board = game->board();
			new_board.set(Cell{
				{current_position},
				Piece{player, current_position, pieceId},
				game->board().get(current_position).walls()
			});
			float new_board_score = calculate_state_score(new_board);
			if (new_board_score > best_score) {
				best_position = current_position;
				best_score = new_board_score;
			}
		}
		place_count++;
		return best_position;
    }

    Move move(const std::vector<Move>& valid_moves) override {
		auto valid_moves_copy = valid_moves;
		std::shuffle(valid_moves_copy.begin(), valid_moves_copy.end(), rng);
		
		std::vector<std::pair<Move, float>> move_scores;

		for (const Move& current_move : valid_moves_copy) {
			Board new_board = game->board().apply_move(current_move);
			float new_board_score = calculate_state_score(new_board);
			move_scores.push_back(std::pair<Move, float>{current_move, new_board_score});
		}

		std::sort(move_scores.begin(), move_scores.end(), [](const std::pair<Move, float>& a, const std::pair<Move, float>& b) {
			return a.second > b.second;
		});
		int candidate_count = std::min(TOP_K, (int)move_scores.size());

		float best_score = -1e9;
		Move best_move = move_scores[0].first;

		auto update_best = [&](float score, Move move) {
			if (score > best_score) {
				best_score = score;
				best_move = move;
			}
		};

		for (int i = 0; i < candidate_count; i++) {
			const auto& [move, score] = move_scores[i];
			Board immediate_board = game->board().apply_move(move);

			std::vector<Move> opponent_moves = immediate_board.get_valid_moves(opponent);
			if (opponent_moves.empty()) {
				update_best(score, move);
				continue;
			}

			float worst_score = 1e9;

			for (const Move& current_move : opponent_moves) {
				Board new_board = immediate_board.apply_move(current_move);
				float new_board_score = calculate_state_score(new_board);
				worst_score = std::min(worst_score, new_board_score);
			}
			update_best(worst_score, move);
		}

		return best_move;
    }
};
}  // namespace

#ifdef RED

namespace red {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new EthenImpl()); }

}  // namespace red

#endif  // RED

#ifdef BLUE

namespace blue {

// Provide a factory function for the engine to use
std::unique_ptr<wallgo::Player> get() { return std::unique_ptr<wallgo::Player>(new EthenImpl()); }

}  // namespace blue

#endif  // BLUE
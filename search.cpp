#include <iostream>

#include "search.h"
#include "evaluate.h"
#include "attack.h"
#include "movegen.h"

// Principal variation line
MoveList main_pv_line = {};

int num_ab = 0, num_q = 0;

void check_up(SearchInfo& info) {

	if (info.timed_search && get_time() > info.stop_time) {
		info.stopped = true;
	}
}

void search_position(Position& pos, SearchInfo& info) {

	Value score;
	Move best_move;

	for (int i = 1; i <= info.depth; i++) {
		num_ab = 0;
		num_q = 0;

		// perform the search
		score = alpha_beta(pos, info, &main_pv_line, i, -INFINITE_VALUE, INFINITE_VALUE);

		if (info.stopped) {
			break;
		}

		// print search results for current dept
		cout << "info score cp " << score << " depth " << i << " nodes " << info.nodes << " time " << get_time() - info.start_time << " pv ";
		print_move_list(main_pv_line);
	}

	best_move = main_pv_line.moves[0];
	cout << "bestmove " << print_move(best_move) << endl;
}

// Quiescence makes sure that there are no cheeky captures at the end of the search
Value Quiescence(Position& pos, SearchInfo& info, Value alpha, Value beta) {
	
	if (info.nodes % 2048 == 0)
		check_up(info);

	info.nodes++;

	if (is_repetition(pos) || pos.rule50 >= 100)
		return 0;

	if (pos.game_ply > MAX_DEPTH - 1)
		return evaluate(pos);

	Value current_eval = evaluate(pos);

	if (current_eval >= beta) {
		return beta;
	}

	if (alpha < current_eval) {
		alpha = current_eval;
	}

	MoveList captures = {};
	generate_captures(pos, captures);
	sort_moves(captures);

	current_eval = -INFINITE_VALUE;

	for (int index = 0; index < captures.count; index++) {
		pos.make_move(captures.moves[index]);
		current_eval = -Quiescence(pos, info, -beta, -alpha);
		pos.undo_move();

		if (info.stopped)
			return 0;

		if (current_eval >= beta) {
			return beta;
		}
		if (current_eval > alpha) {
			alpha = current_eval;
		}
	}

	return alpha;
}

// Alpha Beta is the main search algorithm for determening the best move
Value alpha_beta(Position& pos, SearchInfo& info, MoveList* pvline, int depth, Value alpha, Value beta) {

	MoveList temp_pv_line = {};

	// If we are at a leaf node, evaluate the position
	if (depth == 0) {
		pvline->count = 0;
		//return evaluate(pos);
		return Quiescence(pos, info, alpha, beta);
	}

	if (info.nodes % 2048 == 0)
		check_up(info);

	info.nodes++;

	if ((is_repetition(pos) || pos.rule50 >= 100) && pos.game_ply) {
		return 0;
	}

	if (pos.game_ply > MAX_GAME_MOVES - 1) {
		return evaluate(pos);
	}

	Piece our_king = create_piece(pos.to_move, KING);
	Square king_location = pos.piece_list[our_king][0];
	bool checked = in_check(pos);

	if (checked)
		depth++;

	MoveList mlist = {};
	generate_moves(pos, mlist);
	sort_moves(mlist);
	Move move;
	Value eval = -INFINITE_VALUE;

	for (int i = 0; i < mlist.count; i++) {

		move = mlist.moves[i];
		pos.make_move(move);
		eval = -alpha_beta(pos, info, &temp_pv_line, depth - 1, -beta, -alpha);
		pos.undo_move();

		if (info.stopped)
			return 0;

		if (eval >= beta)
			return beta;

		if (eval > alpha) {

			// store in hueristic array
			pos.cutoff_moves[move.from][move.to] += depth;

			alpha = eval;
			pvline->moves[0] = move;
			memcpy(pvline->moves + 1, temp_pv_line.moves, temp_pv_line.count * sizeof(Move));
			pvline->count = temp_pv_line.count + 1;
		}
	}

	// Checkmate and stalemate
	if (mlist.count == 0) {
		if (checked)
			return MATED + pos.game_ply;
		else
			return 0;
	}

	if (pos.rule50 >= 100)
		return 0;

	return alpha;
}

bool is_repetition(Position& pos) {

	Key current = pos.generate_position_key();
	int repeated = 1;

	for (int i = pos.game_ply - pos.rule50; i < pos.game_ply; i++) {
		if (pos.history_stack[i].id == current)
			repeated++;

		if (repeated >= 3)
			return true;
	}

	return false;
}
#include "perft.h"

U64 perft(Position& pos, int depth) {

	int num_moves = 0;
	U64 nodes = 0;

	if (depth == 0)
		return 1;

	MoveList mlist = {};
	generate_moves(pos, mlist);
	sort_moves(mlist);

	for (int i = 0; i < mlist.count; i++) {
		pos.make_move(mlist.moves[i]);
		nodes += perft(pos, depth - 1);
		pos.undo_move();
	}

	return nodes;
}
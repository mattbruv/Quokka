#ifndef __MOVEGEN_H__
#define __MOVEGEN_H__

#include "types.h"
#include "position.h"

namespace MoveGen {
	void init();
}

void generate_moves(Position& pos, MoveList& list);
void generate_captures(Position& pos, MoveList& list);
void sort_moves(MoveList& list);
MoveList generate_pseudo_legal_moves(Position& pos);
bool is_legal_move(Position& pos, Move m);
int move_in_list(string& str, MoveList& list);
void add_move(Position& pos, MoveList& list, Square from, Square to, Piece promotion = NO_PIECE, bool castle = false, MoveScore score = 0);
void add_pawn_move(Position& pos, MoveList& list, Square from, Square to, Piece promotion = NO_PIECE);
void print_move_list(MoveList& list);

#endif // !__MOVEGEN_H__
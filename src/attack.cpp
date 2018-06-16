#include "attack.h"
#include "position.h"

int KnightMoves[8] = { 21, 19, 12, 8, -8, -12, -19, -21 };
int BishopMoves[4] = { 11, 9, -9, -11 };
int RookMoves[4] = { 10, 1, -1, -10 };
int KingMoves[8] = { 11, 10, 9, 1, -1, -9, -10, -11 };

// square_attacked() is a function which 
bool square_attacked(Position& pos, Square square, Color side) {

	// Pawns
	if (side != WHITE) {
		if (pos.piece_at(square + DELTA_NE) == B_PAWN || pos.piece_at(square + DELTA_NW) == B_PAWN)
			return true;
	} else {
		if (pos.piece_at(square + DELTA_SE) == W_PAWN || pos.piece_at(square + DELTA_SW) == W_PAWN)
			return true;
	}

	Piece attacker;
	Square offset;

	// Knight
	for (int i = 0; i < 8; i++)
	{
		attacker = pos.piece_at(square + KnightMoves[i]);
		if (type_of(attacker) == KNIGHT && color_of(attacker) == side)
			return true;
	}

	// Bishop and Queen
	for (int i = 0; i < 4; i++)
	{
		offset = square;

		while (true)
		{
			offset += BishopMoves[i];
			attacker = pos.piece_at(offset);

			if (!square_on_board(offset)) break;
			if (attacker == NO_PIECE) continue;
			if (color_of(attacker) == side)
				if (type_of(attacker) == BISHOP || type_of(attacker) == QUEEN) return true;
				else break;
			else
				break;
		}
	}

	// Rook and Queen
	for (int i = 0; i < 4; i++)
	{
		offset = square;

		while (true)
		{
			offset += RookMoves[i];
			attacker = pos.piece_at(offset);

			if (!square_on_board(offset)) break;
			if (attacker == NO_PIECE) continue;
			if (color_of(attacker) == side)
				if (type_of(attacker) == ROOK || type_of(attacker) == QUEEN) return true;
				else break;
			else
				break;
		}
	}

	// King
	for (int i = 0; i < 8; i++)
	{
		attacker = pos.piece_at(square + KingMoves[i]);
		if (type_of(attacker) == KING && color_of(attacker) == side)
			return true;
	}

	return false;
}

bool in_check(Position& pos) {
	Piece our_king = create_piece(pos.to_move, KING);

	if (square_attacked(pos, pos.piece_list[our_king][0], !pos.to_move))
		return true;
	
	return false;
}

// Test if a 120 index is on the legal board
inline bool square_on_board(Square s) {
	return (to64(s) != OFFBOARD) ? true : false;
}

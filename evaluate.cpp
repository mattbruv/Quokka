#include "evaluate.h"
#include "movegen.h"
#include "attack.h"

const Value bishop_pair_bonus = 100;
const Value passed_pawn_bonus[8] = { 0, 5, 10, 20, 35, 60, 100, 200 };
const Value rook_open_file_bonus = 10;
const Value rook_semi_open_file_bonus = 5;
const Value queen_open_file_bonus = 5;
const Value queen_semi_open_file_bonus = 3;
const Value double_pawn_penalty = -15;

// Maximum centipawn value for the engine to consider a position as an endgame
const Value endgame_material = (value_of(ROOK) + 2 * value_of(KNIGHT) + 2 * value_of(PAWN));

// Value of each type of piece from pawn to king
Value piece_values[7] = { 0, 100, 250, 300, 500, 900, INFINITE_VALUE };

// helper function to return the value of a type of piece
inline Value value_of(PieceType ptype) {
	return piece_values[ptype];
}

// Array which holds information on the number of pawns on a file for each side
int pawns_on_file[2][8] = {};

// Table for mirroring the index of the lookup tables for black
const int mirror64[64] = {
	56, 57, 58, 59, 60, 61, 62, 63,
	48, 49, 50, 51, 52, 53, 54, 55,
	40, 41, 42, 43, 44, 45, 46, 47,
	32, 33, 34, 35, 36, 37, 38, 39,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	8, 9, 10, 11, 12, 13, 14, 15,
	0, 1, 2, 3, 4, 5, 6, 7
};

// The following piece square tables are the optimal squares for that piece type
// These encourage the engine to put it's pieces on good squares

Value pawn_table[64] = {
	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
	10	,	10	,	0	,	-10	,	-10	,	0	,	10	,	10	,
	5	,	0	,	0	,	5	,	5	,	0	,	0	,	5	,
	0	,	0	,	10	,	20	,	20	,	10	,	0	,	0	,
	5	,	5	,	5	,	10	,	10	,	5	,	5	,	5	,
	10	,	10	,	10	,	20	,	20	,	10	,	10	,	10	,
	20	,	20	,	20	,	30	,	30	,	20	,	20	,	20	,
	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	
};

Value knight_table[64] = {
	0	,	-10	,	0	,	0	,	0	,	0	,	-10	,	0	,
	0	,	0	,	0	,	5	,	5	,	0	,	0	,	0	,
	0	,	0	,	10	,	10	,	10	,	10	,	0	,	0	,
	0	,	0	,	10	,	20	,	20	,	10	,	5	,	0	,
	5	,	10	,	15	,	20	,	20	,	15	,	10	,	5	,
	5	,	10	,	10	,	20	,	20	,	10	,	10	,	5	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0		
};

Value bishop_table[64] = {
	0	,	0	,	-10	,	0	,	0	,	-10	,	0	,	0	,
	0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
	0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
	0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
	0	,	10	,	15	,	20	,	20	,	15	,	10	,	0	,
	0	,	0	,	10	,	15	,	15	,	10	,	0	,	0	,
	0	,	0	,	0	,	10	,	10	,	0	,	0	,	0	,
	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	
};

Value rook_table[64] = {
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0	,
	25	,	25	,	25	,	25	,	25	,	25	,	25	,	25	,
	0	,	0	,	5	,	10	,	10	,	5	,	0	,	0		
};

Value king_endgame_table[64] = {
	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,	-50	,
	50	,	50	,	50	,	50	,	50	,	50	,	50	,	50	,
	100	,	100	,	100	,	100	,	100	,	100	,	100	,	100	,
	200	,	200	,	200	,	200	,	200	,	200	,	200	,	200	,
	400	,	400	,	400	,	400	,	400	,	400	,	400	,	400	,
	900 ,	900 ,	900 ,	900 ,	900 ,	900 ,	900 ,	900 ,
	1300,	1300,	1300,	1300,	1300,	1300,	1300,	1300,
	9999,	9999,	9999,	9999,	9999,	9999,	9999,	9999,
};

// evaluate() evaluates the given position, and returns it's score in centipawns
Value evaluate(Position& pos) {

	memset(pawns_on_file, 0, sizeof(pawns_on_file));

	Value score;
	Color us = pos.to_move;
	Color them = !us;
	PieceType type;

	score = pos.material[us] - pos.material[them];

	// Bishop Pair
	if (pos.piece_num[create_piece(us, BISHOP)] >= 2 && pos.piece_num[create_piece(them, BISHOP)] < 2)
		score += bishop_pair_bonus;
	if (pos.piece_num[create_piece(us, BISHOP)] < 2 && pos.piece_num[create_piece(them, BISHOP)] >= 2)
		score -= bishop_pair_bonus;

	// Loop through all the pieces
	for (int i = W_PAWN; i <= B_KING; i++) {
		
		type = type_of(i);

		// Loop through each piece in the piece list
		for (int j = 0; j < pos.piece_num[i]; j++) {

			Piece piece = i;
			Square square = pos.piece_list[piece][j];

			if (color_of(piece) == us) // if this is our piece, add to the value
				score += table_value(pos, piece, square, us);
			else
				score -= table_value(pos, piece, square, us);
		}

		Piece white_king = pos.piece_list[create_piece(us, KING)][0];
		Piece black_king = pos.piece_list[create_piece(them, KING)][0];

		Rank white_rank = rank_of(to64(white_king));
		Rank black_rank = rank_of(to64(black_king));

		if (pos.to_move == WHITE) {
			if (white_rank == 7) {
				if (black_rank == 7) {
					score = 0;
				}
				else {
					score = MATE - pos.game_ply;
				}
			}
		}
		else {
			if (white_rank == 7) {
				if (black_rank != 7)
					score = -MATE - pos.game_ply;
				else
					score = MATE - pos.game_ply;
			}
		}
	}

	return score;
}

// A function which returns the relative value for a piece on a square
Value table_value(Position& pos, Piece p, Square s, Color side) {

	//s = (color_of(p) == WHITE) ? to64(s) : mirror64[to64(s)];
	s = to64(s);

	switch (type_of(p)) {
		/*
		case PAWN:   return pawn_table[s];
		case KNIGHT: return knight_table[s];
		case BISHOP: return bishop_table[s];
		case ROOK:   return rook_table[s];*/
		case KING:   return king_endgame_table[s];
		default:     return 0;
	}
}

// determines if a position is in the endgame for the side to move
bool is_endgame(Position& pos) {
	return (pos.material[!pos.to_move] <= endgame_material) ? true : false;
}

// determines if a file is open of enemy and friendly pawns
bool is_open_file(Piece p, Square s) {
	s = to64(s);
	File testfile = file_of(s);
	Color them = !color_of(p);
	// if they dont have any pawns on the file, and we dont have any pawns on the file return true
	return (!pawns_on_file[them][testfile] && !pawns_on_file[!them][testfile]) ? true : false;
}

// same as the above function, except we only test for one pawn
bool is_half_open_file(Piece p, Square s) {
	s = to64(s);
	File testfile = file_of(s);
	Color them = !color_of(p);
	
	// there is probably an easier way to do this but I am tired
	if (pawns_on_file[them][testfile] && !pawns_on_file[!them][testfile])
		return true;
	if (!pawns_on_file[them][testfile] && pawns_on_file[!them][testfile])
		return true;

	return false;
}
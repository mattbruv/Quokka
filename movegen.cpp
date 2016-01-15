#include <iostream>
#include <algorithm>

#include "movegen.h"
#include "attack.h"

using namespace std;

// Offset array for piece movement
Square offset[7][8] = {
	0, 0, 0, 0, 0, 0, 0, 0,           // No Piece
	0, 0, 0, 0, 0, 0, 0, 0,           // Pawn
	21, 19, 12, 8, -8, -12, -19, -21, // Knight
	11, 9, -9, -11, 0, 0, 0, 0,       // Bishop
	10, 1, -1, -10, 0, 0, 0, 0,       // Rook
	11, 10, 9, 1, -1, -9, -10, -11,   // Queen
	11, 10, 9, 1, -1, -9, -10, -11    // King
};

// Lookup values for Most Valuable Victim - Least Valuable Aggressor
const MoveScore victim_score[13] = { 0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 };
MoveScore MVV_LVA[13][13];

namespace MoveGen {

	void init() {

		Piece attacker;
		Piece victim;

		for (attacker = W_PAWN; attacker <= B_KING; attacker++) {
			for (victim = W_PAWN; victim <= B_KING; victim++) {
				MVV_LVA[victim][attacker] = victim_score[victim] + 6 - (victim_score[attacker] / 100);
			}
		}

	}

};

// Determines if the specified piece is a slider
bool slider[7] = {
	false, false, false, true, true, true, false
};

void get_psuedo_legals(Position& pos, MoveList& list) {
	list = generate_pseudo_legal_moves(pos);
}

void get_psuedo_legal_captures(Position& pos, MoveList& list) {

	MoveList pos_moves = {};
	get_psuedo_legals(pos, pos_moves);

	for (int i = 0; i < pos_moves.count; i++) {
		if (pos.piece_at(pos_moves.moves[i].to) != NO_PIECE)
			list.moves[list.count++] = pos_moves.moves[i];
	}
}

void generate_moves(Position& pos, MoveList& list) {

	MoveList pseudo_legals = generate_pseudo_legal_moves(pos);

	for (int i = 0; i < pseudo_legals.count; i++) {
		if (is_legal_move(pos, pseudo_legals.moves[i])) {
			Piece p = pos.piece_at(pseudo_legals.moves[i].from);
			list.moves[list.count++] = pseudo_legals.moves[i];
		}
	}
}

void generate_captures(Position& pos, MoveList& list) {

	MoveList pos_moves = {};
	generate_moves(pos, pos_moves);

	for (int i = 0; i < pos_moves.count; i++) {
		if (pos.piece_at(pos_moves.moves[i].to) != NO_PIECE)
			list.moves[list.count++] = pos_moves.moves[i];
	}

}

bool move_compare(Move m1, Move m2) {
	return m1.score > m2.score;
}

void sort_moves(MoveList& list) {

	sort(list.moves, list.moves + list.count, move_compare); 

}

MoveList generate_pseudo_legal_moves(Position& pos) {

	MoveList list = {};

	Color our_side = pos.to_move;
	Square N, E, S, W, NE, NW;

	N = (our_side == WHITE) ? DELTA_N : DELTA_S;
	E = (our_side == WHITE) ? DELTA_E : DELTA_W;
	S = (our_side == WHITE) ? DELTA_S : DELTA_N;
	W = (our_side == WHITE) ? DELTA_W : DELTA_E;
	NE = (our_side == WHITE) ? DELTA_NE : DELTA_SW;
	NW = (our_side == WHITE) ? DELTA_NW : DELTA_SE;

	Piece our_pawns = create_piece(our_side, PAWN);

	// Loop from white/black Knight to white/black King and generate moves for each
	for (int i = create_piece(our_side, KNIGHT); i <= create_piece(our_side, KING); i++) {

		// Set our piece based on our side
		Piece piece = create_piece(our_side, type_of(i));
		PieceType ptype = type_of(piece);
		Piece attacked = NO_PIECE;
		Square from_square = SQ_NONE, to_square = SQ_NONE;

		// Loop through each piece in the list
		for (int j = 0; j < pos.piece_num[piece]; j++) {

			from_square = pos.piece_list[piece][j]; // get the piece's starting square
			int offset_num = 0;
			int direction;

			// Loop through each offset direction
			while (offset[ptype][offset_num] && offset_num < 8) {

				direction = offset[ptype][offset_num];
				to_square = from_square + direction;

				// Add each step to the offset direction and make move if applicable
				while (true) {
					attacked = pos.piece_at(to_square);

					if (!square_on_board(to_square)) break; // we can't go here if it's off the board
					if (type_of(attacked) != NO_PIECE_TYPE && color_of(attacked) == our_side) break; // cannot capture our own color piece
					add_move(pos, list, from_square, to_square); // Add the move
					if (!slider[ptype]) break; // if this piece can't slide, stop going outwards
					if (type_of(attacked) != NO_PIECE_TYPE) break; // we've captured a piece, we can't go further
					to_square += direction;
				}
				offset_num++;
			}
		}
	}

	Square king_square = SQ_NONE;
	Piece our_king = create_piece(our_side, KING);
	king_square = pos.piece_list[our_king][0];

	// If we are not in check currently
	if (!square_attacked(pos, king_square, !our_side)) {

		if (our_side == WHITE) {
			if (!square_attacked(pos, F1, !our_side) && !square_attacked(pos, G1, !our_side) && (pos.castling_perms & WKCA))
				if (pos.piece_at(F1) == NO_PIECE && pos.piece_at(G1) == NO_PIECE)
					add_move(pos, list, E1, G1, NO_PIECE, true); // Castle Kingside
			if (!square_attacked(pos, D1, !our_side) && !square_attacked(pos, C1, !our_side) && (pos.castling_perms & WQCA))
				if (pos.piece_at(D1) == NO_PIECE && pos.piece_at(C1) == NO_PIECE && pos.piece_at(B1) == NO_PIECE)
					add_move(pos, list, E1, C1, NO_PIECE, true); // Castle Queenside
		}
		else {
			if (!square_attacked(pos, F8, !our_side) && !square_attacked(pos, G8, !our_side) && (pos.castling_perms & BKCA))
				if (pos.piece_at(F8) == NO_PIECE && pos.piece_at(G8) == NO_PIECE)
					add_move(pos, list, E8, G8, NO_PIECE, true); // Castle Kingside
			if (!square_attacked(pos, D8, !our_side) && !square_attacked(pos, C8, !our_side) && (pos.castling_perms & BQCA))
				if (pos.piece_at(D8) == NO_PIECE && pos.piece_at(C8) == NO_PIECE && pos.piece_at(B8) == NO_PIECE)
					add_move(pos, list, E8, C8, NO_PIECE, true); // Castle Queenside
		}
	}

	return list;
}

// add_pawn_move() handles everything to do with pawn moves
void add_pawn_move(Position& pos, MoveList& list, Square from, Square to, Piece promotion) {

	Square N = (pos.to_move == WHITE) ? DELTA_N : DELTA_S;

	// we can tell if the pawn is entering the 8th rank because the destination square + north will be offboard
	if (!square_on_board(to + N)) {
		for (int i = KNIGHT; i <= QUEEN; i++) {
			add_move(pos, list, from, to, create_piece(pos.to_move, i), false, 1000000 + (i * 10));
		}
	}
	else {
		add_move(pos, list, from, to);
	}

}

// add_move() adds a move to the move list
void add_move(Position& pos, MoveList& list, Square from, Square to, Piece promotion, bool castle, MoveScore score) {
	
	Piece attacker = pos.piece_at(from);
	Piece attacked = pos.piece_at(to);

	// Rank captures & non captures
	if (attacked != NO_PIECE) {
		score += 900000 + MVV_LVA[attacked][attacker];
	} else {
		score += pos.cutoff_moves[from][to];
	}

	list.moves[list.count++] = create_move(from, to, promotion, castle, score);
}

// is_legal_move() determines if the move given will leave the king in check or not
bool is_legal_move(Position& pos, Move m) {

	Piece our_king = create_piece(pos.to_move, KING);
	Piece their_king = create_piece(!pos.to_move, KING);

	pos.make_move(m);

	bool is_legal = true;

	// If our king isn't in check after the move, we can add it to the list
	if (square_attacked(pos, pos.piece_list[our_king][0], pos.to_move)) {
		bool is_legal = false;
	}

	// If the enemy king isn't in check after the move, it is legal
	if (square_attacked(pos, pos.piece_list[their_king][0], !pos.to_move)) {
		bool is_legal = false;
	}

	pos.undo_move();

	return is_legal;
}

// move_in_list() returns the location of a move if it's in the specified move list
int move_in_list(string& str, MoveList& list) {
	string array_move;
	for (int i = 0; i < list.count; i++) {
		array_move = print_move(list.moves[i]);
		if (array_move == str)
			return i;
	}
	return -1;
}

void print_move_list(MoveList& list) {
	for (int i = 0; i < list.count; i++) {
		cout << print_move(list.moves[i]) << " ";
	}
	cout << endl;
}
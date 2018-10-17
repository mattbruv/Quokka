#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>

#include "types.h"
#include "position.h"
#include "movegen.h"
#include "attack.h"
#include "evaluate.h" // value_of

using namespace std;

const string PieceToChar(" PNBRQKpnbrqk");

Square sq120_to_64[120];
Square sq64_to_120[120];

PieceType piece_type[13] = {
	NO_PIECE_TYPE,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

// Position hash variables
Key piece_keys[13][120];
Key side_key;
Key castle_keys[16];

// Helper function to create a move
Move create_move(Square from, Square to, Piece promotion, bool castle, int score) {
	Move m;
	m.from = from;
	m.to = to;
	m.promotion = promotion;
	m.castle = castle;
	m.score = score;
	return m;
}

// Convert a 120 based square to it's algebraic notation
string coord(Square s) {

	ostringstream oss;
	char file;
	char rank;

	s = to64(s);
	file = 'a' + file_of(s);
	rank = '1' + rank_of(s);
	oss << file << rank;
	return oss.str();
}

// Convert a move into readable text
string print_move(Move m) {
	ostringstream oss;
	oss << coord(m.from) << coord(m.to);
	if (m.promotion != NO_PIECE)
		oss << char(tolower(PieceToChar[m.promotion]));
	return oss.str();
}

// Convert a move string into the move
void parse_UCI_move(Position& pos, string& str) {
	
	MoveList mlist = {};

	if (str.length() == 5)
		str[4] = char(tolower(str[4]));

	generate_moves(pos, mlist);

	int index = move_in_list(str, mlist);

	if (index != -1) {
		pos.make_move(mlist.moves[index]);
	}
}

// rand64() returns a random 64 bit integer used for hashing the board position
inline Key rand64() {
	Key r30 = RAND_MAX*rand()+rand();
	Key s30 = RAND_MAX*rand()+rand();
	Key t4  = rand() & 0xf;

	Key res = (r30 << 34) + (s30 << 4) + t4;

	return res;
}

// Initialize the hash key array
void init_hash_keys() {
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 120; j++) {
			piece_keys[i][j] = rand64();
		}
	}
	side_key = rand64();
	for (int i = 0; i < 16; i++) {
		castle_keys[i] = rand64();
	}
}

// Initializes the lookup tables for converting 120 based squares to their 64 based counterparts and vice versa
void init_lookup_tables() {

	// Set offboard values for move generation
	for (int i = 0; i < 120; i++)
		sq120_to_64[i] = SQ_NONE;

	File file;
	Rank rank;
	Square sq120, sq64 = 0;

	// Insert lookup values
	for (rank = RANK_1; rank <= RANK_8; rank++) {
		for (file = FILE_A; file <= FILE_H; file++) {
			sq120 = FR2SQ(file, rank);
			sq64_to_120[sq64] = sq120;
			sq120_to_64[sq120] = sq64;
			sq64++;
		}
	}
}

// Position::init() is a function which initializes all necessary helper data
// which is required for use by a position object
void Position::init() {
	init_lookup_tables();
	init_hash_keys();
}

// Default constructor
Position::Position() {
	clear();
}

// Overloaded constructor
Position::Position(const string fen) {
	clear();
	parse_fen(fen);
}

// Clear the position object and set everything to default data
void Position::clear() {
	memset(this, 0, sizeof(Position));
	en_passant_target = SQ_NONE;
}

// Position::parse_fen() parses a Forsyth-Edwards Notation string to be used on the internal game board
void Position::parse_fen(const string& fen) {

	char col, row, token;
	size_t p;
	Square sq = to64(A8);
	istringstream ss(fen);

	clear();
	ss >> noskipws;

	// 1. Piece placement
	while ((ss >> token) && !isspace(token))
	{
		if (isdigit(token))
			sq += (token - '0'); // Add files

		else if (token == '/')
			sq -= 16;

		else if ((p = PieceToChar.find(token)) != string::npos)
		{
			add_piece(to120(sq), p);
			sq++;
		}
	}

	// 2. Side to move
	ss >> token;
	to_move = (token == 'w') ? WHITE : BLACK;
	ss >> token;

	// 3. Castling Rights
	while ((ss >> token) && !isspace(token))
	{
		switch (token)
		{
			case 'K': castling_perms |= WKCA; break;
			case 'Q': castling_perms |= WQCA; break;
			case 'k': castling_perms |= BKCA; break;
			case 'q': castling_perms |= BQCA; break;
		}
	}

	// 4. En Passant
	if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
		((ss >> row) && (row == '3' || row == '6')))
	{
		sq = FR2SQ((col - '0') - 49, (row - '0') - 1);
		en_passant_target = sq;
	}

	// 5. Halfmove and fullmove
	ss >> skipws >> rule50 >> game_ply;
	game_ply = max(2 * (game_ply - 1), 0) + int(to_move == BLACK);
}

// Position::make_move() moves a piece while keeping piece lists in sync.
void Position::make_move(Move m, bool save) {

	Piece p = piece_at(m.from);
	Piece attacked = piece_at(m.to);
	m.captured = attacked;

	assert(type_of(p) != NO_PIECE);

	if (save)
		take_snapshot(m);

	remove_piece(m.from, p);

	if (attacked != NO_PIECE)
		remove_piece(m.to, attacked);

	// Change piece if we are promoting
	if (m.promotion > NO_PIECE)
		p = m.promotion;

	add_piece(m.to, p);

	// Save the position state to the history and then change any positional values
	if (save) {
		handle_en_passant(p, m);
		parse_castling(p, m);

		// Set 50 move rule to 0 if a pawn moved or a piece was captured
		if (type_of(p) == PAWN || attacked != NO_PIECE)
			rule50 = 0;
		else
			rule50++;

		to_move = (to_move == WHITE) ? BLACK : WHITE;

		if (to_move == WHITE)
			pos_key ^= side_key;

		if (en_passant_target != SQ_NONE)
			pos_key ^= piece_keys[NO_PIECE][en_passant_target];

		pos_key ^= castle_keys[castling_perms];
	}

	if (m.castle)
		do_castling(m);
}

// Position::undo_move() takes back the last move in the position object based on the move list
void Position::undo_move() {

	assert(game_ply > 0);

	Snapshot snap = history_stack[--game_ply];
	Move m = snap.move;

	castling_perms = snap.castling_perms;
	en_passant_target = snap.en_passant_target;
	rule50 = snap.rule50;
	to_move = (to_move == WHITE) ? BLACK : WHITE;

	// Move our piece back to its original square
	Piece p = piece_at(m.to);

	remove_piece(m.to, p);

	// If it was a capture, restore the original piece
	if (m.captured != NO_PIECE)
		add_piece(m.to, m.captured);

	// If it was a pawn promotion, restore the pawn
	if (m.promotion > NO_PIECE)
		p = (to_move == WHITE) ? W_PAWN : B_PAWN;

	add_piece(m.from, p);

	// Undo en passant
	if (type_of(p) == PAWN && m.to == en_passant_target) {
		if (to_move == WHITE) {
			add_piece(m.to + DELTA_S, B_PAWN);
		}
		else {
			add_piece(m.to + DELTA_N, W_PAWN);
		}
	}

	if (m.castle)
		undo_castling(m);

	pos_key = snap.id;
}

// Position::parse_castling() forbids castling if the rooks or king move or if the rook is captured
void Position::parse_castling(Piece p, Move m) {

	PieceType moved = type_of(p);

	if (type_of(m.captured) == ROOK) {
		switch (m.to) {
			case H8: clear_bit(castling_perms, BKCA); break;
			case A8: clear_bit(castling_perms, BQCA); break;
			case H1: clear_bit(castling_perms, WKCA); break;
			case A1: clear_bit(castling_perms, WQCA); break;
		}
	}
	
	if (moved == ROOK) {
		switch (m.from) {
			case A8: clear_bit(castling_perms, BQCA); break;
			case H8: clear_bit(castling_perms, BKCA); break;
			case A1: clear_bit(castling_perms, WQCA); break;
			case H1: clear_bit(castling_perms, WKCA); break;
		}
	}

	// exit if we already can't castle
	if (!castling_perms || (moved != ROOK && moved != KING))
		return;

	if (to_move == WHITE) {
		if (m.from == E1) {
			clear_bit(castling_perms, WKCA);
			clear_bit(castling_perms, WQCA);
		}
		if (m.from == A1 && (castling_perms & WQCA))
			clear_bit(castling_perms, WQCA);
		if (m.from == H1 && (castling_perms & WKCA))
			clear_bit(castling_perms, WKCA);
	}
	else {
		if (m.from == E8) {
			clear_bit(castling_perms, BKCA);
			clear_bit(castling_perms, BQCA);
		}
		if (m.from == A8 && (castling_perms & WQCA))
			clear_bit(castling_perms, BQCA);
		if (m.from == H8 && (castling_perms & WKCA))
			clear_bit(castling_perms, BKCA);
	}
}

// Position::handle_en_passant() writes the en-passant square if applicable
void Position::handle_en_passant(Piece p, Move m) {
	
	if (type_of(p) != PAWN) {
		en_passant_target = SQ_NONE;
		return;
	}

	// If the move is capturing our target, capture the target pawn
	if (m.to == en_passant_target) {
		if (to_move == WHITE)
			remove_piece(m.to + DELTA_S, B_PAWN);
		else
			remove_piece(m.to + DELTA_N, W_PAWN);
	}

	// If the pawn moved from the 2nd rank to the 4th
	if (to_move == WHITE) {
		if (rank_of(to64(m.from)) == 1 && rank_of(to64(m.to)) == 3)
			en_passant_target = m.from += DELTA_N;
		else
			en_passant_target = SQ_NONE;
	}
	// If the pawn moved from the 7th rank to the 5th
	else {
		if (rank_of(to64(m.from)) == 6 && rank_of(to64(m.to)) == 4)
			en_passant_target = m.from += DELTA_S;
		else
			en_passant_target = SQ_NONE;
	}
}

// Position::do_castling() performs the castling action.
// This doesn't check if the move was valid, that is left up to the move generator
void Position::do_castling(Move m) {

	m.castle = false;

	switch (m.to) {
		case G1: m.from = H1; m.to = F1; break;
		case C1: m.from = A1; m.to = D1; break;
		case G8: m.from = H8; m.to = F8; break;
		case C8: m.from = A8; m.to = D8; break;
	}
		
	make_move(m, false);
}

// Position::undo_castling() reverses the castling action.
void Position::undo_castling(Move m) {

	m.castle = false;

	switch (m.to) {
		case G1: m.from = F1; m.to = H1; break;
		case C1: m.from = D1; m.to = A1; break;
		case G8: m.from = F8; m.to = H8; break;
		case C8: m.from = D8; m.to = A8; break;
	}

	make_move(m, false);
}

// Position::piece_at() returns the piece on the specified square
Piece Position::piece_at(Square s) {

	return board[s];
}

// Position::remove_piece() Removes a piece in the 120 based board array and piece list
void Position::remove_piece(Square s, Piece p) {

	int index = SQ_NONE;
	board[s] = NO_PIECE;

	// loop through piece list and get index of the piece on from square
	for (int i = 0; i < piece_num[p]; i++)
	{
		if (piece_list[p][i] == s) {
			index = i;
			break;
		}
	}

	assert(index != SQ_NONE);

	// this is a somewhat clever way of "removing" a piece.
	// we are basically swapping its index value with the one at the end of the piece list, pseudo "removing" it.
	// if a piece is added later, the value at the end of the array is replaced, everything >= piece_list[piece_num[p]] is garbage
	piece_num[p]--;
	piece_list[p][index] = piece_list[p][piece_num[p]];

	if (type_of(p) != KING)
		material[color_of(p)] -= value_of(type_of(p));

	pos_key ^= piece_keys[p][s];
}

// Position::put_piece() Inserts a piece in the 120 based board array and piece list
void Position::add_piece(Square s, Piece p) {

	board[s] = p;
	piece_list[p][piece_num[p]++] = s;
	
	if (type_of(p) != KING)
		material[color_of(p)] += value_of(type_of(p));

	pos_key ^= piece_keys[p][s];
}

// Position::take_snapshot() saves the current state of the board to the history stack
void Position::take_snapshot(Move m) {

	Snapshot snap;

	snap.castling_perms = castling_perms;
	snap.en_passant_target = en_passant_target;
	snap.rule50 = rule50;
	snap.move = m;

	// If we have no base position key, generate one, otherwise use the dynamically updated one
	snap.id = (game_ply == 0) ? generate_position_key() : pos_key;

	if (game_ply == 0) {
		pos_key = generate_position_key();
	}
	
	snap.id = pos_key;

	history_stack[game_ply++] = snap;
}

// Position::generate_position_key() generates a unique hash key for the position to check for repetition draws
Key Position::generate_position_key() {

	Key poskey = 0;

	// Add piece locations from piece list for each type and number of piece
	for (int i = W_PAWN; i <= B_KING; i++) {
		for (int j = 0; j < piece_num[i]; j++) {
			poskey ^= piece_keys[i][j];
		}
	}

	if (to_move == WHITE)
		poskey ^= side_key;

	if (en_passant_target != SQ_NONE)
		poskey ^= piece_keys[NO_PIECE][en_passant_target];

	poskey ^= castle_keys[castling_perms];

	return poskey;
}

// Position::print_board() displays the internal board to the console in a pretty fashion
void Position::print_board() {

	Square square = 0, dark = 0;
	char piece;

	cout << "\n +---+---+---+---+---+---+---+---+\n";

	for (Rank r = RANK_8; r >= RANK_1; r--)
	{
		for (File f = FILE_A; f <= FILE_H; f++)
		{
			square = FR2SQ(f, r);
			piece = PieceToChar[board[square]];

			cout << " | ";
			cout << ((dark % 2 && piece == ' ') ? '.' : piece);
			dark++;
		}
		dark--;
		cout << " |\n +---+---+---+---+---+---+---+---+\n";
	}

	cout << ((to_move == WHITE) ? "white" : "black") << " to move" << endl;

	if (en_passant_target != SQ_NONE)
		cout << "en-passant square: " << coord(en_passant_target) << endl;

	cout << "castling: ";
	if (castling_perms & WKCA) cout << "K";
	if (castling_perms & WQCA) cout << "Q";
	if (castling_perms & BKCA) cout << "k";
	if (castling_perms & BQCA) cout << "q";
	if (!castling_perms) cout << "n/a";
	cout << endl;

	cout << "ply: " << game_ply << endl;
	cout << "50 move rule: " << rule50 << endl;
}

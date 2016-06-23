#ifndef __TYPES_H__
#define __TYPES_H__

#include <cassert>
#include <cctype>
#include <string>

using namespace std;

typedef int Square, Color, File, Rank, Piece, PieceType, Moves, MoveScore, Value;
typedef unsigned char Byte;
typedef unsigned long long Key, U64;

const string NAME = "Quokka 2.0";
const string AUTHOR = "Matt P";

const int MAX_GAME_MOVES = 1024;
const int MAX_POSITION_MOVES = 256;
const int MAX_DEPTH = 64;
const int INFINITE_VALUE = 100000; // a theoretical "infinite" to value the king
const int MATED = -INFINITE_VALUE + 100; // this value has to be larger than -infinity so we know the beta value changed
const int MATE = INFINITE_VALUE - 100;

// Square values
enum {
	A1 = 21, B1, C1, D1, E1, F1, G1, H1,
	A2 = 31, B2, C2, D2, E2, F2, G2, H2,
	A3 = 41, B3, C3, D3, E3, F3, G3, H3,
	A4 = 51, B4, C4, D4, E4, F4, G4, H4,
	A5 = 61, B5, C5, D5, E5, F5, G5, H5,
	A6 = 71, B6, C6, D6, E6, F6, G6, H6,
	A7 = 81, B7, C7, D7, E7, F7, G7, H7,
	A8 = 91, B8, C8, D8, E8, F8, G8, H8,

	OFFBOARD = 99,
	SQ_NONE = -1,

	DELTA_N = 10,
	DELTA_E = 1,
	DELTA_W = -1,
	DELTA_S = -10,

	DELTA_NN = DELTA_N + DELTA_N,
	DELTA_NE = DELTA_N + DELTA_E,
	DELTA_SE = DELTA_S + DELTA_E,
	DELTA_SS = DELTA_S + DELTA_S,
	DELTA_SW = DELTA_S + DELTA_W,
	DELTA_NW = DELTA_N + DELTA_W
};

// Side colors
enum { WHITE, BLACK };

// Files and Ranks
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

// Piece names and colors
enum {
	NO_PIECE,
	W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
	B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};

// Piece types
enum {
	NO_PIECE_TYPE,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

// Castling bit permissions
enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

// Structures

// The Move structure holds information about the move
struct Move {
	Square from;
	Square to;
	Piece captured;
	Piece promotion;
	bool castle;
	MoveScore score; // for search ordering
};

// The MoveList structure contains all of the moves for a position and the count
struct MoveList {
	Move moves[MAX_POSITION_MOVES];
	int count;
};

// The Snapshot structure holds information about a specific chess position
// This is needed to undo moves and makes up the history stack
struct Snapshot {
	Key id;
	Byte castling_perms;
	Square en_passant_target;
	Move move;
	int rule50;
};

// The SearchInfo structure holds parameters for a search
struct SearchInfo {
	int start_time;
	int stop_time;
	int depth;
	int moves_to_go;
	long nodes;
	bool timed_search;
	bool quit;
	bool stopped;
};

// Global Functions

extern Square FR2SQ(File f, Rank r);
extern inline Square to64(Square s);
extern inline Square to120(Square s);
extern File file_of(Square s);
extern inline Rank rank_of(Square s);
extern string print_move(Move m);
extern inline Color color_of(Piece p);
extern inline PieceType type_of(Piece p);
extern inline bool square_on_board(Square s);
extern Move create_move(Square from, Square to, Piece promotion = NO_PIECE, bool castle = false, int score = 0);
extern inline Piece create_piece(Color side, PieceType ptype);
extern Value value_of(PieceType ptype);
extern int get_time();

#endif // !__TYPES_H__
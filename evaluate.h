#ifndef __EVALUATE_H__
#define __EVALUATE_H__

#include "types.h"
#include "position.h"
#include <iostream>

Value evaluate(Position& pos);
Value table_value(Position& pos, Piece p, Square s, Color side);
bool is_endgame(Position& pos);
bool is_pawn_passed(Piece p, Square s);
bool is_open_file(Piece p, Square s);
bool is_half_open_file(Piece p, Square s);
bool is_pawn_doubled(Piece p, Square s);

#endif // !__EVALUATE_H__

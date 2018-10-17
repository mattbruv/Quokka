#ifndef __ATTACK_H__
#define __ATTACK_H__

#include "types.h"
#include "position.h"

bool square_attacked(Position& pos, Square square, Color side);
bool in_check(Position& pos);

// Test if a 120 index is on the legal board
inline bool square_on_board(Square s) {
	return (to64(s) != OFFBOARD) ? true : false;
}

#endif

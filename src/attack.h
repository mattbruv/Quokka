#ifndef __ATTACK_H__
#define __ATTACK_H__

#include "types.h"
#include "position.h"

bool square_attacked(Position& pos, Square square, Color side);
bool in_check(Position& pos);

#endif
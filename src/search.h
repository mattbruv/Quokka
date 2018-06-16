#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "types.h"
#include "position.h"

void check_up(SearchInfo& info);
void search_position(Position& pos, SearchInfo& info);
Value alpha_beta(Position& pos, SearchInfo& info, MoveList* pvline, int depth, Value alpha, Value beta);
Value Quiescence(Position& pos, SearchInfo& info, Value alpha, Value beta);
bool is_repetition(Position& pos);

#endif
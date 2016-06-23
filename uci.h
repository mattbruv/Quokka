#ifndef __UCI_H__
#define __UCI_H__

#include <string>
#include <sstream>
#include "types.h"
#include "position.h"
#include "attack.h"
#include "evaluate.h"
#include "movegen.h"
#include "search.h"
#include "perft.h"

namespace UCI {
	void loop();
}

void debug(Position& pos);
void do_perft(Position& pos, istringstream& iss);
void position(Position& pos, istringstream& iss);
void go(Position& pos, SearchInfo info, istringstream& iss);
void make_move(Position& pos, istringstream& iss);
int get_time();


#endif // !__UCI_H__
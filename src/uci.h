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
	void init();
	void loop();
}

void debug();
void do_perft(istringstream& iss);
void position(istringstream& iss);
void go(istringstream& iss);
void make_move(istringstream& iss);
int get_time();


#endif // !__UCI_H__
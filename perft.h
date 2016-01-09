#ifndef __PERF_H__
#define __PERF_H__

#include "types.h"
#include "movegen.h"
#include "position.h"

U64 perft(Position& pos, int depth);

#endif // !__PERF_H__
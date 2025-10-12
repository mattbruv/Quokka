#include <iostream>
#include "position.h"
#include "movegen.h"
#include "uci.h"

extern "C"
{
	const char *int_sqrt(int depth)
	{
		static Position pos;
		pos.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

		SearchInfo info = {};
		info.stopped = false;
		info.depth = depth;

		static std::string output; // static to keep it alive after return
		std::ostringstream oss;
		auto old_buf = std::cout.rdbuf(oss.rdbuf()); // redirect cout to oss

		search_position(pos, info);
		std::cout << pos.generate_position_key();

		std::cout.rdbuf(old_buf); // restore cout
		output = oss.str();		  // store captured string

		return output.c_str(); // return const char*
	}
}

int main()
{
	cout << NAME << " by " << AUTHOR << endl;

	Position::init();
	MoveGen::init();
	UCI::loop();

	return 0;
}
#include <iostream>
#include "position.h"
#include "movegen.h"
#include "uci.h"

extern "C"
{
	int int_sqrt(int x)
	{
		return 42;
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
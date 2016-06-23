#include <iostream>
#include "types.h"
#include "position.h"
#include "movegen.h"
#include "uci.h"

int main()
{
	cout << NAME << " by " << AUTHOR << endl;

	Position::init();
	MoveGen::init();
	UCI::loop();

	return 0;
}
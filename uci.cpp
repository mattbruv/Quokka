#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <Windows.h>

#include "uci.h"

// FEN string of the initial position
const string start_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void UCI::loop() {

	Position pos(start_FEN);
	SearchInfo info = {};

	string command, token;

	// Main UCI loop
	while (true) {

		if (!getline(cin, command)) // wait for user input
			command = "quit";

		istringstream iss(command);

		iss >> skipws >> token;

		if (token == "quit") {
			break;
		}
		else if (token == "uci") {
			cout << "id " << NAME << endl;
			cout << "id author " << AUTHOR << endl;
			cout << "uciok" << endl;
		}
		else if (token == "ucinewgame") pos.parse_fen(start_FEN);
		else if (token == "go")         go(pos, info, iss);
		else if (token == "position")   position(pos, iss);
		else if (token == "setoption")  {}
		else if (token == "isready")    cout << "readyok" << endl;
		else if (token == "p")          debug(pos);
		else if (token == "m")          make_move(pos, iss);
		else if (token == "u")          pos.undo_move();
		else if (token == "set")        pos.parse_fen("rnbqkb1r/ppp2ppp/4p3/3p2P1/3P4/4PN2/PPP2PP1/RN1QKB1R b KQkq - 0 6");
		else if (token == "perft")      do_perft(pos, iss);
		else
			cout << "Unknown command: " << command << endl;
	}
}

// Make a move
void make_move(Position& pos, istringstream& iss) {
	string token;
	iss >> token;

	parse_UCI_move(pos, token);
	debug(pos);
}

void debug(Position& pos) {
	MoveList mlist = {};
	generate_moves(pos, mlist);
	pos.print_board();
	print_move_list(mlist);
	cout << "Evaluation in Centipawns: " << evaluate(pos) << endl;
	cout << "is endgame? " << (is_endgame(pos) ? "true" : "false") << endl;
	cout << "key: " << uppercase << hex << pos.history_stack[pos.game_ply].id << dec << endl;
}

void do_perft(Position& pos, istringstream& iss) {
	string token;
	iss >> token;

	int depth = (token == "") ? 1 : stoi(token);

	cout << "Nodes at depth " << depth << ": " << perft(pos, depth) << endl;
}

// position() is called when engine receives the "position" UCI command.
// The function sets up the position described in the given fen string ("fen")
// or the starting position ("startpos") and then makes the moves given in the
// following move list ("moves").
void position(Position& pos, istringstream& iss) {

	string token, fen;

	iss >> token;

	if (token == "startpos") {
		fen = start_FEN;
		iss >> token;
	}
	else if (token == "fen") {
		while (iss >> token && token != "moves") {
			fen += token + " ";
		}
	}
	else
		return;

	pos.parse_fen(fen);

	// Parse move list (if any)
	while (iss >> token) {
		parse_UCI_move(pos, token);
	}
}

// setoption() is called when engine receives the "setoption" UCI command. The
// function updates the UCI option ("name") to the given value ("value").
void setoption(istringstream& iss) {}

// go() is called when engine receives the "go" UCI command. The function sets
// the thinking time and other parameters from the input string, and starts the search.
void go(Position& pos, SearchInfo info, istringstream& iss) {
	
	string token;

	int depth = MAX_DEPTH, movestogo = 30, movetime = -1;
	int time = -1, inc = 0;

	while (iss >> token) {
		if (token == "wtime" && pos.to_move == WHITE)      iss >> time;
		else if (token == "btime" && pos.to_move == BLACK) iss >> time;
		else if (token == "winc" && pos.to_move == WHITE)  iss >> inc;
		else if (token == "binc" && pos.to_move == BLACK)  iss >> inc;
		else if (token == "movestogo")                     iss >> movestogo;
		else if (token == "depth")                         iss >> depth;
		else if (token == "movetime")                      iss >> movetime;
	}

	if (movetime != -1) {
		time = movetime;
		movestogo = 1;
	}

	info.start_time = get_time();
	info.depth = depth;

	if (time != -1) {
		time /= movestogo;
		time -= 50;
		info.stop_time = info.start_time + time + inc;
		info.timed_search = true;
	}

	//cout << "time: " << time << " start: " << info.start_time << " stop: " << info.stop_time << " depth: " << info.depth << endl;
	//cout << "Searching for " << info.stop_time - info.start_time << " seconds." << endl;
	
	int start_time = get_time();
	search_position(pos, info);
	int end_time = get_time();

	//cout << "finished in " << (end_time - start_time) / 1000 << " seconds" << endl;
}

// Return the time in milliseconds
int get_time() {
	return GetTickCount64();
}
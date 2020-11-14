#include <iostream>
#include <chrono>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>

#include "uci.h"

// FEN string of the initial position
const string start_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/*
	Had to do quite a bit to enable this program to run with threads.
	Had to make the position, info, and thread itself have global scope
	Had to make it so any change of the position or info object would first end the search
	Now the program input doens't get blocked during a search.
	It's not beautiful code, but it got the job done.
	It feels like polishing a turd at this point, 6 years later,
	but I want to make the engine work better on lichess
	and not crash or burn time too much.
*/
std::thread searchThread;
Position pos;
SearchInfo info = {};

void stopSearch() {
	// set search to stopped and join the thread.
	info.stopped = true;
	if (searchThread.joinable()) {
		searchThread.join();
	}
}

void UCI::loop() {

	string command, token;
	pos.parse_fen(start_FEN);

	// Main UCI loop
	while (true) {
		if (!getline(cin, command)) // wait for user input
			command = "quit";

		istringstream iss(command);

		iss >> skipws >> token;

		if (token == "quit") {
			stopSearch();
			break;
		}
		else if (token == "uci") {
			cout << "id name " << NAME << endl;
			cout << "id author " << AUTHOR << endl;
			cout << "uciok" << endl;
		}
		else if (token == "ucinewgame") {
			stopSearch();
			pos.parse_fen(start_FEN);
		}
		else if (token == "go")         go(iss);
		else if (token == "position")   position(iss);
		else if (token == "setoption")  {}
		else if (token == "stop")       stopSearch();
		else if (token == "isready")    cout << "readyok" << endl;
		else if (token == "p")          debug();
		else if (token == "m")          make_move(iss);
		else if (token == "u") {
			stopSearch();
			pos.undo_move();
		}
		else if (token == "set") {
			stopSearch();
			pos.parse_fen("rnbqkb1r/ppp2ppp/4p3/3p2P1/3P4/4PN2/PPP2PP1/RN1QKB1R b KQkq - 0 6");
		}
		else if (token == "perft")      do_perft(iss);
		else
			cout << "Unknown command: " << command << endl;
	}
}

// Make a move
void make_move(istringstream& iss) {
	stopSearch();
	string token;
	iss >> token;

	parse_UCI_move(pos, token);
	debug();
}

void debug() {
	stopSearch();
	MoveList mlist = {};
	generate_moves(pos, mlist);
	pos.print_board();
	print_move_list(mlist);
	cout << "Evaluation in Centipawns: " << evaluate(pos) << endl;
	cout << "is endgame? " << (is_endgame(pos) ? "true" : "false") << endl;
	cout << "key: " << uppercase << hex << pos.history_stack[pos.game_ply].id << dec << endl;
}

void do_perft(istringstream& iss) {
	stopSearch();
	string token;
	iss >> token;

	int depth = (token == "") ? 1 : stoi(token);

	cout << "Nodes at depth " << depth << ": " << perft(pos, depth) << endl;
}

// position() is called when engine receives the "position" UCI command.
// The function sets up the position described in the given fen string ("fen")
// or the starting position ("startpos") and then makes the moves given in the
// following move list ("moves").
void position(istringstream& iss) {

	stopSearch();

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
void go(istringstream& iss) {

	stopSearch();

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
	info.stopped = false;

	if (time != -1) {
		time /= movestogo;
		time -= 50;
		info.stop_time = info.start_time + time + inc;
		info.timed_search = true;
	}

	//cout << "time: " << time << " start: " << info.start_time << " stop: " << info.stop_time << " depth: " << info.depth << endl;
	//cout << "Searching for " << info.stop_time - info.start_time << " seconds." << endl;
	
	//int start_time = get_time();
	// search_position(pos, info);
	searchThread = std::thread(search_position, std::ref(pos), std::ref(info));
	//int end_time = get_time();
	// if the thread goes out of scope and not joined it will crash

	//cout << "finished in " << (end_time - start_time) << " milliseconds" << endl;
}

// Return the time in milliseconds
int get_time() {
	using namespace std::chrono;
	milliseconds ms = duration_cast<milliseconds>(
	    system_clock::now().time_since_epoch()
	);
	return ms.count();
}

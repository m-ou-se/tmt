// This file is part of Turing Machine Tool.
// 
// Turing Machine Tool is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Turing Machine Tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Turing Machine Tool.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <cstdio>
#include <vector>
#include <map>
#include <sstream>
#include <deque>
#include <fstream>
#include <set>
#include <csignal>
#include <cstdlib>

using namespace std;

class TuringMachine {
	
	public:
		pair<string, string> current;
		map<pair<string,string>, pair<pair<string, string>, int> > rules;
		map<string, pair<pair<string, string>, int> > default_rules;
		
		vector<string> tape_end;
		vector<string> tape_begin;
		
		set<string> halt_states;
		
		TuringMachine() : current(make_pair("start", "-")) {}
		
		bool halted;
		
		bool step() {
			if (halt_states.find(current.first) != halt_states.end()) { halted = true; return false; }
			map<pair<string,string>, pair<pair<string, string>, int> >::iterator rule = rules.find(current);
			if (rule == rules.end()) {
				map<string, pair<pair<string, string>, int> >::iterator default_rule = default_rules.find(current.first);
				if (default_rule == default_rules.end()) default_rule = default_rules.find("-");
				if (default_rule == default_rules.end()) { halted = false; return false; }
				current = default_rule->second.first;
				move(default_rule->second.second);
				return true;
			}
			current = rule->second.first;
			move(rule->second.second);
			return true;
		}
		
		void move(int direction) {
			while(direction){
				vector<string> & tape_from = direction > 0 ? tape_end   : tape_begin;
				vector<string> & tape_to   = direction > 0 ? tape_begin : tape_end  ;
				if (!tape_to.empty() || current.second != "-") tape_to.push_back(current.second);
				if (tape_from.empty()) {
					current.second = "-";
				} else {
					current.second = tape_from.back();
					tape_from.pop_back();
				}
				direction += direction < 0 ? 1 : -1;
			}
		}
		
		ostream & print_tape(ostream & out) {
			for (vector<string>::iterator i = tape_begin.begin(); i != tape_begin.end(); i++) out << *i << ' ';
			out << '[' << current.second << ']';
			for (vector<string>::reverse_iterator i = tape_end.rbegin(); i != tape_end.rend(); i++) out << ' ' << *i;
			return out;
		}
		
};

istream * open_stream(string const & filename) {
	if (filename == "-") return &cin;
	ifstream * f = new ifstream(filename.c_str());
	if (f->is_open()) return f;
	cerr << "Unable tot open file `" << filename << "'." << endl;
	delete f;
	return 0;
}

void close_stream(istream * s) {
	if (s && s != &cin) delete s;
}

const int unknown_move = -2;
int parse_move(string move) {
	int m = unknown_move;
	if      (move == "r" || move == "R" || move == ">" || move == "->" || move == "=>") m =  1;
	else if (move == "l" || move == "L" || move == "<" || move == "<-" || move == "<=") m = -1;
	else if (move == "-" || move == "0" || move == "." || move == "--") m = 0;
	if (m == unknown_move) {
		cerr << "Unknown movement `" << move << "'." << endl;
	}
	return m;
}

sig_atomic_t interrupted = 1;
void interrupt(int x) { if (interrupted++) exit(1); }

int main(int argc, char * * argv) {
	
	signal(SIGINT, interrupt);
	
	deque<pair<istream *, string> > input_streams;
	
	if (argc <= 1) {
		input_streams.push_back(make_pair(&cin,"[stdin]"));
	} else {
		for (size_t i = 1; i < argc; i++) {
			if (argv[i][0] == '-' && argv[i][1] != '\0') {
				input_streams.push_back(pair<istream *, string>(0, &argv[i][1]));
			} else {
				if (istream * f = open_stream(argv[i])) input_streams.push_back(make_pair(f,argv[i]));
			}
		}
	}
	
	TuringMachine tm;
	
	while (true) {
		
		string line;
		while (!input_streams.empty()) {
			if (input_streams.front().first) {
				if (getline(*input_streams.front().first, line)) break;
				pair<istream *, string> s = input_streams.front();
				if (s.first->bad()) cerr << "Unable to read from `" << s.second << "'." << endl;
				close_stream(s.first);
				input_streams.pop_front();
			} else {
				line = input_streams.front().second;
				input_streams.pop_front();
				break;
			}
		}
		if (line.empty() && input_streams.empty()) break;
		stringstream input(line);
		
		string cmd;
		input >> cmd;
		
		if (cmd == "version") {
			cout << PACKAGE_STRING << endl;
			
		} else if (cmd == "reset") {
			tm = TuringMachine();
			
		} else if (cmd == "rule") {
			string state, tape, new_state; 
			if (input >> state) {
				if (input >> tape) {
					if (input >> new_state) {
						string new_tape, move;
						if (!(input >> new_tape )) new_tape = "-";
						if (!(input >> move     )) move     = "-";
						int m = parse_move(move);
						if (m != unknown_move) tm.rules[make_pair(state, tape)] = make_pair(make_pair(new_state, new_tape), m);
					} else {
						tm.rules.erase(make_pair(state, tape));
					}
				} else {
					cerr << "No tape symbol given." << endl;
				}
			} else {
				cerr << "No state given." << endl;
			}
			
		} else if (cmd == "clearrules") {
			tm.rules.clear();
			
		} else if (cmd == "defaultrule") {
			string state;
			if (input >> state) {
				string new_state, new_tape, move;
				if (!(input >> new_state)) {
					new_state = state;
					state = "-";
				}
				if (new_state == "-") {
					tm.default_rules.erase(state);
				} else {
					if (!(input >> new_tape )) new_tape = "-";
					if (!(input >> move     )) move     = "-";
					int m = parse_move(move);
					if (m != unknown_move) tm.default_rules[state] = make_pair(make_pair(new_state, new_tape), m);
				}
			} else {
				cerr << "No state given." << endl;
			}
			
		} else if (cmd == "cleardefaultrules") {
			tm.default_rules.clear();
			
		} else if (cmd == "clearallrules") {
			tm.        rules.clear();
			tm.default_rules.clear();
			
		} else if (cmd == "halt") {
			string s;
			if (input >> s) {
				tm.halt_states.insert(s);
			} else {
				cerr << "No state given." << endl;
			}
			
		} else if (cmd == "nohalt") {
			string s;
			if (input >> s) {
				tm.halt_states.erase(s);
			} else {
				cerr << "No state given." << endl;
			}
			
		} else if (cmd == "dump") {
			cout << "state" << '\t' << tm.current.first << endl;
			cout << "tape" << '\t';
			for (vector<string>::iterator i = tm.tape_begin.begin(); i != tm.tape_begin.end(); i++) cout << *i << ' ';
			cout << tm.current.second;
			for (vector<string>::reverse_iterator i = tm.tape_end.rbegin(); i != tm.tape_end.rend(); i++) cout << ' ' << *i;
			cout << endl;
			if (!tm.tape_begin.empty()) cout << "move" << '\t' << tm.tape_begin.size() << endl;
			for (map<pair<string,string>, pair<pair<string, string>, int> >::iterator i = tm.rules.begin(); i != tm.rules.end(); i++) {
				cout << "rule"                 << '\t'
				     << i->first.first         << '\t'
				     << i->first.second        << '\t'
				     << i->second.first.first  << '\t'
				     << i->second.first.second << '\t'
				     << (i->second.second ? i->second.second < 0 ? "<-" : "->" : "-") << endl;
			}
			for (map<string, pair<pair<string, string>, int> >::iterator i = tm.default_rules.begin(); i != tm.default_rules.end(); i++) {
				cout << "defaultrule"          << '\t'
				     << i->first               << '\t'
				     << i->second.first.first  << '\t'
				     << i->second.first.second << '\t'
				     << (i->second.second ? i->second.second < 0 ? "<-" : "->" : "-") << endl;
			}
			for (set<string>::iterator i = tm.halt_states.begin(); i != tm.halt_states.end(); i++) cout << "halt" << '\t' << *i << endl;
			
		} else if (cmd == "begin") {
			tm.move(-tm.tape_begin.size());
			
		} else if (cmd == "end") {
			tm.move(tm.tape_end.size());
			
		} else if (cmd == "right" || cmd == "move") {
			int distance;
			if (!(input >> distance)) distance = 1;
			tm.move(distance);
			
		} else if (cmd == "left") {
			int distance;
			if (!(input >> distance)) distance = 1;
			tm.move(-distance);
			
		} else if (cmd == "clearl") {
			tm.tape_begin.clear();
			
		} else if (cmd == "clearr") {
			tm.tape_end.clear();
			
		} else if (cmd == "clear") {
			tm.tape_begin.clear();
			tm.tape_end  .clear();
			tm.current.second = "-";
			
		} else if (cmd == "tape") {
			if (input >> tm.current.second) {
				tm.tape_begin.clear();
				tm.tape_end  .clear();
				do { tm.move(1); } while (input >> tm.current.second);
				tm.move(-tm.tape_begin.size());
			} else {
				tm.print_tape(cout) << endl;
			}
			
		} else if (cmd == "tapestring") {
			tm.tape_begin.clear();
			tm.tape_end  .clear();
			tm.current.second = "-";
			char c;
			while (input >> c) { tm.current.second = c; tm.move(1); }
			tm.move(-tm.tape_begin.size());
			
		} else if (cmd == "readtape") {
			string filename;
			if (!(input >> filename)) filename = "-";
			if (istream * f = open_stream(filename)) {
				tm.tape_begin.clear();
				tm.tape_end  .clear();
				tm.current.second = "-";
				while (*f >> tm.current.second) tm.move(1);
				if (f->bad()) cerr << "Unable to read from `" << filename << "'." << endl;
				tm.move(-tm.tape_begin.size());
				close_stream(f);
			}
			
		} else if (cmd == "readstring") {
			string filename;
			if (!(input >> filename)) filename = "-";
			if (istream * f = open_stream(filename)) {
				tm.tape_begin.clear();
				tm.tape_end  .clear();
				tm.current.second = "-";
				char c;
				while (*f >> c) { tm.current.second = c; tm.move(1); }
				if (f->bad()) cerr << "Unable to read from `" << filename << "'." << endl;
				tm.move(-tm.tape_begin.size());
				close_stream(f);
			}
			
		} else if (cmd == "readline") {
			string filename;
			if (!(input >> filename)) filename = "-";
			if (istream * f = open_stream(filename)) {
				tm.tape_begin.clear();
				tm.tape_end  .clear();
				tm.current.second = "-";
				string l;
				getline(*f, l);
				stringstream ss(l);
				char c;
				while (ss >> c) { tm.current.second = c; tm.move(1); }
				if (f->bad()) cerr << "Unable to read from `" << filename << "'." << endl;
				tm.move(-tm.tape_begin.size());
				close_stream(f);
			}
			
		} else if (cmd == "state") {
			if (!(input >> tm.current.first)) cout << tm.current.first << endl;
			
		} else if (cmd == "read" || cmd == "r") {
			cout << tm.current.second << endl;
			
		} else if (cmd == "write" || cmd == "w") {
			if (!(input >> tm.current.second)) tm.current.second = "-";
			
		} else if (cmd == "put" || cmd == "putr" || cmd == "p") {
			while (input >> tm.current.second) tm.move(1);
			
		} else if (cmd == "putl") {
			while (input >> tm.current.second) tm.move(-1);
			
		} else if (cmd == "insertr") {
			string s;
			if (!(input >> s)) s = "-";
			tm.tape_end.push_back(s);
			
		} else if (cmd == "insertl" || cmd == "insert" || cmd == "i") {
			string s;
			if (!(input >> s)) s = "-";
			tm.tape_begin.push_back(s);
			
		} else if (cmd == "step" || cmd == "s" || cmd == "steptrace" || cmd == "ts") {
			bool trace = cmd == "steptrace" || cmd == "ts";
			unsigned int steps;
			if (!(input >> steps)) steps = 1;
			while (steps--) {
				if (!tm.step()) {
					if (tm.halted) cerr << "Halted." << endl;
					else           cerr << "No rule for the current situation." << endl;
					break;
				}
				if (trace) { cout << tm.current.first << ": "; tm.print_tape(cout) << endl; }
			}
			
		} else if (cmd == "run" || cmd == "trace" || cmd == "runtrace") {
			bool trace = cmd == "trace" || cmd == "runtrace";
			cerr << "Running . . . "; cerr.flush();
			if (trace) cerr << endl;
			interrupted = 0;
			while(tm.step() && !interrupted) if (trace) { cout << tm.current.first << ": "; tm.print_tape(cout) << endl; }
			if (interrupted) cerr << "Interrupted." << endl;
			else if (tm.halted) cerr << "Done." << endl;
			else cerr << "Error." << endl << "No rule for the current situation." << endl;
			interrupted = 1;
			
		} else if (cmd == "input") {
			string filename;
			if (!(input >> filename)) filename = "-";
			if (istream * f = open_stream(filename)) input_streams.push_front(make_pair(f,filename));
			
		} else if (cmd == "return") {
			istream * s = input_streams.front().first;
			if (s != &cin) delete s;
			input_streams.pop_front();
			
		} else if (cmd == "exit") {
			exit(0);
			
		} else if (cmd == "") {
			
		} else {
			cerr << "Unknown command `" << cmd << "'." << endl;
			continue;
		}
		
		if (input.peek() != EOF) cerr << "Extra character(s) ignored." << endl;
		
	}
	
}

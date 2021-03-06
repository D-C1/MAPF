/* Written by Brandon Wu */

#include "Search.h"
#include "Globals.h"
#include <limits>
//#include <list>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <unordered_map>
using namespace std;

bool mincmp(Node*, Node*);

Search::Search(int n, Point* init, Point* goal, Grid* g, Distance* d,
	unordered_map<int,Apos_t>* cat):
	n(n), init(init), grid(g), dlt(d), cat(cat), exp_cnt(1) {
	
	current = NULL;

	/*if (!dlt) {
		dlt = new Distance(grid);
		cout << "Created transposition table\n";
	}*/
	this->goal = new Point[n];
	for (int i=0; i<n; i++)
		this->goal[i] = goal[i];

	/*	Add initial state to open list */
	Node* tmp = new Node;
	tmp->p = NULL;
	tmp->s = new State(init, n);
	tmp-> f = tmp->s->h(goal);
	tmp->turn = 0;
	open.push((Node_t)tmp);
}

Search::~Search() {
	delete [] goal;
	///delete dlt;
	while(!open.empty()) {
		Node_t t = open.top();
		delete t.n->s;
		delete t.n;
		open.pop();
	}
	for (auto it = closed.begin(); it != closed.end(); it++) {
		delete (*it)->s;
		delete (*it);

	}
}

int Search::expand(void) {
	/* Choose node on open with min f cost */
	Node* nd = NULL;		//Node to expand

	if (open.empty()) {
		cout << "ERROR: NULL chosen for expansion\n";
		return false;
	}

	Node_t nt = open.top();
	nd = nt.n;
	open.pop();

	int myf = nd->f;
	int mylen = open.size();
	/* Check if node chosen for exp is goal */
	if (is_goal(nd))
		return 1;
	
	if (num_expansions() > EXPLIM) {
		cout << "Exceeded expansion threshold";
		return 2;
	}
	/* Get adj list for position of agent about to move */
	int turn = nd->turn;

	bool* valid_m = nd->s->valid_moves(turn, grid);

	/* Disallow move in reverse direction */
	int lastmove = (nd->p) ? getdir(nd->s->get_pos(turn),
		nd->p->s->get_pos(turn)) : WAIT;

	for (int i=0; i<DIM+1;i++) {
		if (valid_m[i]) { //&& i != lastmove) {

			if (cat != NULL) {
				Apos_t tmp_m = nd->s->movecheck(Move((Card)i, turn));
				if ((cat->find(tmp_m.timestep)) != cat->end()) {
						auto it = cat->find(tmp_m.timestep);
						if (it->second == tmp_m) {
							//cout << "Avoiding Point " << 
								//grid->hash_pt(&(tmp_m.pos)) << endl;
							continue;
						}
				}
			}
			open.push((Node_t)generate(nd,i));
		}
	}

	if (turn) {
		delete nd->s;
		delete nd;
	}
	else closed.push_back(nd);

	delete [] valid_m;
	
	mylen = open.size();
	return 0;
}

Node* Search::generate(Node* p, int dir) {
	Node* child = new Node;
	Move m((Card)dir, p->turn);
	
	exp_cnt++;	//Increment num of nodes expanded

	child->p = (p->turn) ? p->p : p;
	child->turn = (p->turn+1 == n) ? 0 : p->turn+1;
	child->s = new State(n, *(p->s), m);
	
	// Replace MANHATTAN DIST
	int g = p->s->g();
	int h = child->s->h(goal, dlt);
	//child->f = p->s->g() + child->s->h(goal, dlt);	// true dist
	child->f = (g+h < h) ? h : g+h;	// Set to INT_MAX if overflow
	
		
	child->dir = dir;

	return child;
}

/* Return true if node is a goal node */
bool Search::is_goal(Node* nd) {
	if (nd->turn) return false;

	for (int i=0; i<n; i++) {
		Point* p = nd->s->get_pos(i);
		if (goal[i].x != p->x || goal[i].y != p->y)
			return false;
	}
	current = nd;	// Set goal node
	cout << "Found goal with cost " << nd->s->g()/n << "!\n";
	cost_p = nd->s->g()/n;
	cout << "\tNum Expansions = " << num_expansions() << " nodes\n";
	
	return true;
}

vector<int>* Search::backtrace(Node* walk) {
	/* Returns array of vectors of moves for each agent
	 *	moves[i] is a vector of moves for agent 'i'
	 */
	if (!walk) return NULL;

	vector <int>* moves = new vector<int>[n];
	do {
		State* parent = walk->p->s;
		State* current = walk->s;
		for (int i=0; i<n; i++) {
			Point* a = parent->get_pos(i);
			Point* b = current->get_pos(i);
			moves[i].push_back(getdir(a,b));
		}
		walk = walk->p;
	} while (walk->p);

	for (int i=0; i<n; i++) {
		cout << "\nMoves for Agent " << i << endl;
		reverse(moves[i].begin(), moves[i].end());
		for (auto it = moves[i].begin(); it != moves[i].end(); it++) 
			cout << dir2str(*it) + "\n";
	}
	return moves;
}

int*	Search::reconstruct_path(int agent, const vector<int>& tr) {
	if (agent >= n) return NULL;

	int* path = new int[tr.size()+1];
	Point init_s = init[agent];
	
	for (int i=0; i<tr.size(); i++) {
		path[i] = grid->hash_pt(&init_s);
		init_s = move_dir(&init_s, tr[i]);
	}
	path[tr.size()] = grid->hash_pt(&init_s);
	return path;
}

vector<int>* Search::path(bool print=false) {
	if (!current) return NULL;		
	
	vector<int>*	pos = new vector<int>[n];
	vector<int>*	moves = backtrace(current);
	
	for (int i=0; i<n; i++) {
		if (print) cout << "Player " << i << endl;
		int* arr = reconstruct_path(i, moves[i]);
		for (int j=0; j<moves[i].size()+1; j++) {
			if (print) cout << arr[j] << " ";
			pos[i].push_back(arr[j]);
		}
		if (print) cout << endl;
		delete [] arr;
	}
	delete [] moves;
	return pos;	// Return list of pos for ea agent in search
}

bool mincmp(Node* a, Node* b) {
	return a->f > b->f;
}

#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <tuple>

typedef std::pair<int, int> edge_t;

const edge_t NULLEDGE = std::pair(-1, -1);

struct edge_compare 
{
	bool operator ()(edge_t a, edge_t b) 
	{
		if(a.second == b.second) return a.first > b.first;
		return a.second > b.second;
	}
};

std::map<int, int> in_map;
std::vector<edge_t > out_map;

std::vector<std::priority_queue<edge_t, std::vector<edge_t >, edge_compare> > S;
std::vector<std::set<int> > T;
std::vector<std::vector<edge_t > > v;

edge_t last(int b_method, int x) 
{
	if(S[x].size() == bvalue(b_method, x)) return S[x].top();
	return NULLEDGE;
}

void insert(int b_method, int u, edge_t edge)
{
	if(S[u].size() == bvalue(b_method, u)) S[u].pop();
	S[u].push(edge);
}

void new_verticle(int num, int in_num) 
{
	in_map[in_num] = num;
	out_map.push_back(std::make_pair(num, in_num));
	v.push_back(std::vector<edge_t >());
	S.push_back(std::priority_queue<edge_t, std::vector<edge_t >, edge_compare>());
}

edge_t find_edge(int b_method, int u)
{
	edge_compare cmp;
	edge_t x = NULLEDGE; 
	for(auto it = v[u].begin(); it != v[u].end(); ++it)
		if(cmp(*it, x) && cmp(*it, last(b_method, it->first)) && T[u].count(it->first) == 0) x = *it;
	return x;
}

void make_suitor(int b_method, int u, edge_t edge)
{
	edge_t y = last(b_method, edge.first);
	insert(b_method, edge.first, std::make_pair(u, edge.second));
	T[u].insert(edge.first);
	if(y != NULLEDGE)
	{
		T[y.first].erase(edge.first);
		edge_t z = find_edge(b_method, y.first);
		if(z != NULLEDGE) make_suitor(b_method, y.first, z);
	}
}

int main(int argc, char* argv[]) 
{
   if (argc != 4) 
	{
      std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
      return 1;
   }

   int thread_count = std::stoi(argv[1]);
   int b_limit = std::stoi(argv[3]);
   std::string input_filename{argv[2]};
	std::ifstream infile;
	infile.open(input_filename);
	int n_verticles = 0;
	int a,b,w;
	while(infile >> a >> b >> w) 
	{
		if(in_map.count(a) == 0) new_verticle(n_verticles++, a);
		if(in_map.count(b) == 0) new_verticle(n_verticles++, b);
		v[in_map[a]].push_back(std::make_pair(b, w));
		v[in_map[b]].push_back(std::make_pair(a, w));
	}
	edge_t x;
   for (int b_method = 0; b_method < b_limit + 1; b_method++) 
	{
		for(int u = 0; u < n_verticles; ++u) 
		{
			for(unsigned int i = 0; i < bvalue(b_method, u); ++i) 
			{
				x = find_edge(b_method, u);
				if(x == NULLEDGE) break;
				make_suitor(b_method, u, x);
			}
		}
    }
}

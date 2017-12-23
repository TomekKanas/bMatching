#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <tuple>
#include <sstream>
#include <cassert>

typedef std::pair<int, int> edge_t;

const edge_t NULLEDGE = edge_t(-1, -1);
const edge_t INFEDGE = edge_t(1e9 + 9, 1e9 + 9);

std::map<int, int> in_map;
std::vector<int> out_map;

struct edge_compare 
{
	bool operator ()(edge_t a, edge_t b) 
	{
		if(a.second == b.second) return out_map[a.first] > out_map[b.first];
		return a.second > b.second;
	}
};

std::vector<std::priority_queue<edge_t, std::vector<edge_t >, edge_compare> > S;
std::vector<std::set<int> > T;
std::vector<std::vector<edge_t > > v;

edge_t last(int b_method, int x) 
{
	//std::cout << "S(" << x << ") size = " << S[x].size() << " b = " << bvalue(b_method, out_map[x]) << std::endl;
	if(bvalue(b_method, out_map[x]) == 0) return INFEDGE;
	if(S[x].size() == bvalue(b_method, out_map[x])) return S[x].top();
	return NULLEDGE;
}

void insert(int b_method, int u, edge_t edge)
{
	assert(bvalue(b_method, out_map[u]) > 0);
	//std::cout << "S(" << u << ") size = " << S[u].size()  << " b = " << bvalue(b_method, out_map[u]) << std::endl;
	if(S[u].size() == bvalue(b_method, out_map[u])) S[u].pop();
	S[u].push(edge);
}

void new_verticle(int num, int in_num) 
{
	in_map[in_num] = num;
	out_map.push_back(in_num);
	v.push_back(std::vector<edge_t >());
	S.push_back(std::priority_queue<edge_t, std::vector<edge_t >, edge_compare>());
	T.push_back(std::set<int>());
}

edge_t find_edge(int b_method, int u)
{
	edge_compare cmp;
	edge_t x = NULLEDGE; 
	for(auto it = v[u].begin(); it != v[u].end(); ++it)
		if(cmp(*it, x) && cmp(edge_t(u, it->second), last(b_method, it->first)) && T[u].count(it->first) == 0) x = *it;
	return x;
}

int make_suitor(int b_method, int u, edge_t edge)
{
	edge_t y = last(b_method, edge.first);
	insert(b_method, edge.first, std::make_pair(u, edge.second));
	T[u].insert(edge.first);

	//std::cout << "Inserting: " << u << " -> " << edge.first << " value: " << edge.second << " Removing: " << edge.first << " -> " << y.first << " value: " << y.second << std::endl;

	if(y != NULLEDGE)
	{
		T[y.first].erase(edge.first);
		edge_t z = find_edge(b_method, y.first);
		if(z != NULLEDGE) return make_suitor(b_method, y.first, z) + edge.second - y.second;
	}
	return edge.second - (y.second < 0 ? 0 : y.second);
}

int main(int argc, char* argv[]) 
{
   if (argc != 4) 
	{
      std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
      return 1;
   }

   int b_limit = std::stoi(argv[3]);
   std::string input_filename{argv[2]};
	std::ifstream infile;
	infile.open(input_filename);
	int n_verticles = 0;
	int a,b,w;
	int res = 0;
	std::string line;
	std::istringstream iss; 
	while(std::getline(infile, line)) 
	{
		if(line[0] != '#')
		{ 
			iss = std::istringstream(line);
			if(iss >> a >> b >> w) 
			{
				if(in_map.count(a) == 0) new_verticle(n_verticles++, a);
				if(in_map.count(b) == 0) new_verticle(n_verticles++, b);
				v[in_map[a]].push_back(std::make_pair(in_map[b], w));
				v[in_map[b]].push_back(std::make_pair(in_map[a], w));
			}
			else
			{
				std::cerr << "Incorrect input. Error in: " << line << std::endl;
				return 0;
			}
		}
	}
	edge_t x;
   for (int b_method = 0; b_method < b_limit + 1; b_method++) 
	{
		for(int u = 0; u < n_verticles; ++u) 
		{
			for(unsigned int i = 0; i < bvalue(b_method, out_map[u]); ++i) 
			{
				x = find_edge(b_method, u);
				if(x == NULLEDGE) break;
				res += make_suitor(b_method, u, x);

				//std::cout << " " << res << std::endl;
			}
		}
		std::cout << res/2 << std::endl;
		res = 0;
		for(int i = 0; i < n_verticles; ++i)
		{
			S[i] = std::priority_queue<edge_t, std::vector<edge_t >, edge_compare>();
			T[i].clear();
		}
   }
}

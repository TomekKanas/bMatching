#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <tuple>
#include <sstream>
#include <thread>
#include <mutex>
#include <future>
#include <algorithm>

typedef std::pair<int, int> edge_t;

const edge_t NULLEDGE = edge_t(-1, -1);

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
std::vector<std::mutex> mut;
std::mutex Smut, ochrona;

edge_t last(int b_method, int x) 
{
	std::lock_guard<std::mutex> lock(Smut);
	//std::cout << "S(" << x << ") size = " << S[x].size() << " b = " << bvalue(b_method, out_map[x]) << std::endl;
	if(S[x].size() == bvalue(b_method, out_map[x])) return S[x].top();
	return NULLEDGE;
}

void insert(int b_method, int u, edge_t edge)
{
	std::lock_guard<std::mutex> lock(Smut);
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
	mut.push_back(std::mutex());
}

edge_t find_edge(int b_method, int u)
{
	edge_compare cmp;
	edge_t x = NULLEDGE; 
	for(auto it = v[u].begin(); it != v[u].end(); ++it)
		if(cmp(*it, x) && cmp(*it, last(b_method, it->first)) && T[u].count(it->first) == 0) x = *it;
	return x;
}

int suitor(const std::vector<int> Q, std::vector<int>& q, const std::vector<int>& b, std::vector<int>& db, int b_method)
{
	int i;
	int res = 0; 
	for(auto it = Q.begin(); it != Q.end(); ++it)
	{
		i = 1;
		while(i < b[*it])
		{
			edge_t p = find_edge(b_method, *it);
			if(p != NULLEDGE)
			{
				std::lock_guard<std::mutex> lock(mut[p.first]);
				if(find_edge(b_method, *it) == p)
				{
					++i;
					edge_t y = last(b_method, p.first);
					insert(b_method, y.first, std::make_pair(*it, p.second));
					T[*it].insert(p.first);
					res += p.second;

					if(y != NULLEDGE)
					{
						std::lock_guard<std::mutex> lockk(ochrona);
						T[y.first].erase(p.first);
						q.push_back(y.first);
						++db[y.first];
						res -= y.second;
					}
				}
			}
			else break;
		}
	}
	return res;
}

int main(int argc, char* argv[]) 
{
   if (argc != 4) 
	{
      std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
      return 1;
   }

   int thread_count = std::stoi(argv[1]);
	--thread_count;
   int b_limit = std::stoi(argv[3]);
   std::string input_filename{argv[2]};
	std::ifstream infile;
	infile.open(input_filename);
	int n_verticles = 0;
	int res = 0;
	std::string line;
	std::istringstream iss; 
	while(std::getline(infile, line)) 
	{
		int a,b,w;
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
   for (int b_method = 0; b_method < b_limit + 1; b_method++) 
	{
		std::vector<int> b,nb;
		std::vector<int> Q,q;
		std::vector<std::future<int> > futures;
		for(int i = 0; i < n_verticles; ++i) 
		{
			b.push_back(bvalue(b_method, out_map[i]));
			nb.push_back(0);
			Q.push_back(i);
		}
		while(!Q.empty())
		{
			for(int i = 0; i < thread_count; ++i)
			{
				std::vector<int> pq{};
				if(i == thread_count - 1) pq = Q;
				else 
					for(size_t j = 0; j < Q.size() / thread_count; ++j)
					{
						pq.push_back(Q.back());
						Q.pop_back();
					}
				futures.push_back(std::async([&pq, &q, &b, &nb, b_method]
					{
						return suitor(pq, q, b, nb, b_method);
					}));
			}
			for(auto it = futures.begin(); it != futures.end(); ++it) res += it->get();
			std::sort(q.begin(), q.end());
			Q.clear();
			Q.push_back(q.front());
			for(size_t i = 1; i < q.size(); ++i)
				if(q[i] != q[i - 1]) Q.push_back(q[i]);
			q.clear();
			b = nb;
			for(int i = 0; i < n_verticles; ++i) nb[i] = 0;
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

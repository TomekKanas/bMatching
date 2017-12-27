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
#include <cassert>
#include <condition_variable>

typedef std::pair<int, int> edge_t;

const edge_t NULLEDGE = edge_t(-1, -1);
const edge_t INFEDGE = edge_t(1e9 + 9, 1e9 + 9);
const int NUM_COPY = 10;
const int BREAK_MAIN = 0;

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
std::vector<std::vector<edge_t >::iterator> vit;
std::vector<std::unique_ptr<std::mutex> > mut;
std::vector<std::unique_ptr<std::mutex> > Smut;
std::mutex Qmut, ochrona;
std::condition_variable empty_queue, finish;
std::atomic_int working{0};
bool end = false;


void new_verticle(int num, int in_num) 
{
	in_map[in_num] = num;
	out_map.push_back(in_num);
	v.push_back(std::vector<edge_t >());
	S.push_back(std::priority_queue<edge_t, std::vector<edge_t >, edge_compare>());
	T.push_back(std::set<int>());
	mut.push_back(std::make_unique<std::mutex>());
	Smut.push_back(std::make_unique<std::mutex>());
}

edge_t last(int b_method, int x) 
{
	if(bvalue(b_method, out_map[x]) == 0) return INFEDGE;
	//TODO: Don't sure wether this is needed
	std::lock_guard<std::mutex> lock(*Smut[x]);
	if(S[x].size() == bvalue(b_method, out_map[x])) return S[x].top();
	return NULLEDGE;
}

void insert(int b_method, int u, edge_t edge)
{
	assert(bvalue(b_method, out_map[u]) > 0);
	std::lock_guard<std::mutex> lock(*Smut[u]);
	if(S[u].size() == bvalue(b_method, out_map[u])) S[u].pop();
	S[u].push(edge);
}

edge_t find_edge(int b_method, int u)
{
	edge_compare cmp;
	for(vit[u]; vit[u] != v[u].end(); ++vit[u])
		if(cmp(edge_t(u, vit[u]->second), last(b_method, vit[u]->first))) return *vit[u];
	return NULLEDGE;
}


//	std::mutex output;



int suitor(std::vector<int>& Q, std::vector<int>& q, const std::vector<int>& b, std::vector<int>& db, int b_method)
{
	std::set<int> tempT{};
	std::vector<std::pair<int, int>> inserted{};
	std::vector<std::pair<int, int>> todelete{};
	int i;
	int res = 0; 

	for(auto it = Q.begin(); it != Q.end(); ++it)
	{
		i = 0;
		while(i < b[*it])
		{
			edge_t p = find_edge(b_method, *it);
			if(p != NULLEDGE)
			{
				//output.lock();
				//std::cout << "Adding: " << *it << " -> " <<  p.first << " value: " << p.second << std::endl;
				//output.unlock();

				std::lock_guard<std::mutex> lock(*mut[p.first]); 
				if(find_edge(b_method, *it) == p)
				{
					++vit[*it];
					++i;
					edge_t y = last(b_method, p.first);
					insert(b_method, p.first, std::make_pair(*it, p.second));
					inserted.push_back(std::make_pair(*it, p.first));
					res += p.second;

					if(y != NULLEDGE)
					{
						//output.lock();
						//std::cout << "Removing: " << y.first << " -> " << p.first << " value: " << y.second << std::endl;
						//output.unlock();

						assert(y.second <= p.second);
						todelete.push_back(std::make_pair(y.first, p.first));
						res -= y.second;
					}
				}
			}
			else break;
		}
	}
	
	ochrona.lock();
	for(auto it = inserted.begin(); it != inserted.end(); ++it) T[it->first].insert(it->second);
	for(auto it = todelete.begin(); it != todelete.end(); ++it)
	{
		q.push_back(it->first);
		++db[it->first];
	}
	ochrona.unlock();

	//output.lock();
	//std::cout << "Returning: " << res << std::endl;
	//output.unlock();

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
   int b_limit = std::stoi(argv[3]);
   std::string input_filename{argv[2]};
	std::ifstream infile;
	infile.open(input_filename);
	int n_verticles = 0;
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
	//TODO: uwspółbieżnić
	for(int i = 0; i < n_verticles; ++i)
		sort(v[i].begin(), v[i].end(), edge_compare());

	std::atomic_int res;
	int b_method = 0;
	std::vector<std::thread> threads;
	std::vector<int> b,nb;
	std::vector<int> Q,q;
		
	for(int i = 1; i < thread_count; ++i)
		threads.push_back(std::thread([&res, &Q, &q, &b, &nb, &b_method]
		{
			std::vector<int> toprocess{};
			while(!end)
			{
				toprocess.clear();
				std::unique_lock<std::mutex> lk(Qmut);
				empty_queue.wait(lk, [&Q]{return !Q.empty() || end;});
				if(end) break;
				++working;
				for(int i = 0; i < NUM_COPY; ++i)
				{
					toprocess.push_back(Q.back());
					Q.pop_back();
					if(Q.empty()) break;
				}
				lk.unlock();
				res += suitor(toprocess, q, b, nb, b_method);
				--working;
				finish.notify_all();
			}
		}));


	for (b_method = 0; b_method < b_limit + 1; ++b_method) 
	{
		res = 0;
		b.clear();
		nb.clear();
		Qmut.lock();
		Q.clear();
	
		vit.clear();
		for(int i = 0; i < n_verticles; ++i)
			vit.push_back(v[i].begin());

		for(int i = 0; i < n_verticles; ++i) 
		{
			b.push_back(bvalue(b_method, out_map[i]));
			nb.push_back(0);
			Q.push_back(i);
		}
		Qmut.unlock();
		while(!Q.empty())
		{
			empty_queue.notify_all();
			std::vector<int> toprocess{};
			//TODO: nie jestem pewien czy to przyspieszy
			while(true)
			{
				toprocess.clear();
				std::unique_lock<std::mutex> lk(Qmut);
				if(Q.empty()) break;
				for(int i = 0; i < NUM_COPY; ++i)
				{
					toprocess.push_back(Q.back());
					Q.pop_back();
					if(Q.empty()) break;
				}
				lk.unlock();
				res += suitor(toprocess, q, b, nb, b_method);
			}
			std::unique_lock<std::mutex> lock(ochrona);
			finish.wait(lock, []{return working == 0;});
			//TODO: Nie jestem pewien czy to działa
			for(int i = 0; i < n_verticles; ++i)
			{
				b[i] = nb[i];
				nb[i] = 0;
			}
			std::sort(q.begin(), q.end());
			if(!q.empty()) Q.push_back(q.front());
			for(size_t i = 1; i < q.size(); ++i)
				if(q[i] != q[i - 1]) Q.push_back(q[i]);
			q.clear();
		}
		std::cout << res/2 << std::endl;
		for(int i = 0; i < n_verticles; ++i)
		{
			S[i] = std::priority_queue<edge_t, std::vector<edge_t >, edge_compare>();
			T[i].clear();
		}
   }
	end = true;
	empty_queue.notify_all();
	for(auto it = threads.begin(); it != threads.end(); ++it) it->join();
	return 0;
}

//============================================================================
// Name        : Minimum Spanning Tree.cpp
// Author      : Hemant Koti
// UB Name     : hemantko
// UBID        : 50338178
// References  : Lecture slides
// Description : Minimum Spanning Tree.cpp in C++, Ansi-style
//============================================================================

#include <iostream>
#include <vector>
#include <queue>
#include <limits>

using namespace std;

class minheap {
public:
	int operator()(pair<int, int> &a, pair<int, int> &b) {
		return a > b;
	}
};

/**
 * MST using Prim's function
 *
 * @param  graph adjacency list vector
 * @param  visited boolean array of visited vertices
 * @param  n number of vertices
 *
 */
void MSTPrim(vector<pair<int, int>> graph[], bool visited[], int n) {
	int totalWeight = 0;
	int edges[n], d[n];

	for (int i = 0; i < n; i++) {
		edges[i] = INT32_MIN;
		d[i] = INT32_MAX;
	}
	d[0] = 0;

	priority_queue<pair<int, int>, vector<pair<int, int>>, minheap> Q;
	Q.push(make_pair(0, 0)); // Q.insert()

	while (!Q.empty()) {

		int u = Q.top().second; // Q.extract_min()
		Q.pop();

		if (visited[u])
			continue;
		visited[u] = true;

		for (vector<pair<int, int>>::iterator it = graph[u].begin(); it != graph[u].end(); it++) {
			int v = it->first, w = it->second;
			if (!visited[v] && w < d[v]) {
				d[v] = w; // Q.decrease_key()
				edges[v] = u;
				Q.push(make_pair(w, v));
			}
		}
	}

	for (int i = 0; i < n; i++)
		totalWeight += d[i];
	cout << totalWeight << endl;

	for (int i = 1; i < n; i++)
		cout << edges[i] + 1 << " " << i + 1 << endl;
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv) {

	int n, m;
	int u, v, w;

	cin >> n >> m;
	vector<pair<int, int>> graph[n];
	bool visited[n] = { false };

	while (m--) {
		cin >> u >> v >> w;
		graph[u - 1].push_back(make_pair(v - 1, w));
		graph[v - 1].push_back(make_pair(u - 1, w));
	}

	MSTPrim(graph, visited, n);

	return 0;
}

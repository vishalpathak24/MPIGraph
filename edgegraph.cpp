#include <iostream>
#include <map>
#include <vector>

using namespace std;

typedef struct Edge{
	int p;
	int q;
}Edge;

class EdgeGraph{
	private:
		map <int, vector <int> > EdgeList;

	public:
		int pushEdge(int p,int q){
			EdgeList[p].push_back(q);
		}
};

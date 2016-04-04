/*
 * Author : Vishal Pathak
 * Topic : Conversion Program for facebook
 */
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <vector>
#include <assert.h>
#include <algorithm>

using namespace std;

/** PROGRAM COMTROL DEFS **/
#define _DBG_ 0

/** DEFINING CONSTANTS **/

typedef struct Edge{
	int p;
	int q;
}Edge;

class EdgeGraph{
	private:
		map <int, vector <int> > EdgeList;
		
	public:
		int lastp;
		int lastq;
		
		typedef map<int, vector <int> >::iterator iterator;

		EdgeGraph(){
			lastp=-1;
			lastq=-1;			
		}

		/* copy Constructor */
		EdgeGraph(const EdgeGraph &graph){
			this->EdgeList = graph.EdgeList;
			this->lastp = graph.lastp;
			this->lastq = graph.lastq;
		}

		void pushEdge(int p,int q){
			if(!this->hasEdge(p,q)){
				EdgeList[p].push_back(q);
				if (p >= lastp){
					lastp = p;
					lastq = q;
				}
				if ( p == lastp){
					if(q > lastq)
						lastq = q;
				}
			}
		}

		vector <int> * pullEdge(int p){
			
			if(p == lastp){ /* this case is not handled */
			lastp = -1; /* Reset lastp */
			}
			
			if(EdgeList.find(p) != EdgeList.end())
				return &EdgeList[p];
			else
				return NULL;
		}



	bool hasEdge(int p,int q){
		return EdgeList.find(p) != EdgeList.end() && find(EdgeList[p].begin(),EdgeList[p].end(),q) != EdgeList[p].end();
	}

	iterator begin(){
		return EdgeList.begin();
	}

	iterator end(){
		return EdgeList.end();
	}

	void clear(){
		EdgeList.clear();
	}
	
	int printGraph(ofstream &file){
		int count =0;

		for(iterator it = EdgeList.begin(); it != EdgeList.end(); it++){
			unsigned int len;
			len = it->second.size();
			for(unsigned int i=0;i<len;i++){
				cout<<"("<<it->first<<','<<it->second[i]<<") \n";
				file<<it->first<<" "<<it->second[i]<<'\n';
				count++;
			}
		}
		return count;
	}

	void printLast(void){
		cout<<"My last node is ("<<lastp<<","<<lastq<<")\n";
	}


};


int main(){
	string path = "./facebook_combined.txt"; 
	int Nlines = 88234;
	//int Nedges = 2*88234;
   	int NVertex = 500;
   	int NedgesMade;
   	ifstream graphFile("./facebook_combined.txt",ifstream::in);
   	ofstream newgraphFile("./facebook_combined_N500.txt",ofstream::out);
   	EdgeGraph graph;
   	do{
   		int p ,q;
   		graphFile>>p>>q;
   		if(p<NVertex && q<NVertex){
	   		graph.pushEdge(p,q);
	   		graph.pushEdge(q,p);
	   	}
   	}while(!graphFile.eof());

   	NedgesMade = graph.printGraph(newgraphFile);

   	graphFile.close();
   	newgraphFile.close();
   	cout<<"Nedges in graph "<<NedgesMade<<'\n';
   	return 0;
}



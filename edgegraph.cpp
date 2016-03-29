#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include <assert.h>
#include <algorithm>

using namespace std;

#ifndef GRAPH_TAG
#define GRAPH_TAG 2
#endif

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

		vector <int> * getEdge(int p){
			/* Does Not remove the Edge Detail from Graph */
			if(EdgeList.find(p) != EdgeList.end())
				return &EdgeList[p];
			else
				return NULL;

		}

		int sendEdges(int edgeNo,int d_id,MPI_Comm comm = MPI_COMM_WORLD){
			vector <int> *edge = pullEdge(edgeNo);
			if(edge != NULL){
				int len;
				len = (*edge).size();
				assert(MPI_Send(&edgeNo,1,MPI_INT,d_id,GRAPH_TAG,comm) == MPI_SUCCESS );
				assert(MPI_Send(&len,1,MPI_INT,d_id,GRAPH_TAG,comm) == MPI_SUCCESS );
				for (int i = 0; i < len ; i++){
				assert(MPI_Send(&((*edge)[i]),1,MPI_INT,d_id,GRAPH_TAG,comm) == MPI_SUCCESS );
#if _DBG_
				cout<<"Sending ("<<edgeNo<<","<<(*edge)[i]<<")\n";
#endif
				}
				/* clearing edge sent */
				(*edge).clear();
				iterator it = EdgeList.find(edgeNo);
				EdgeList.erase(it);


			}else{
				int buf = -1;
				assert(MPI_Send(&buf,1,MPI_INT,d_id,GRAPH_TAG,comm) == MPI_SUCCESS );
			}
			return MPI_SUCCESS;
		}

		int recvEdges(int s_id,MPI_Comm comm = MPI_COMM_WORLD){
			int buff,len,p,q;
			MPI_Status status;
			assert(MPI_Recv(&buff,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
			if(buff != -1){
				/* Not NULL CASE */
				p = buff;
				assert(MPI_Recv(&len,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
				for(int i = 0 ; i < len ; i++){
					assert(MPI_Recv(&q,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
					this->pushEdge(p,q);
#if _DBG_
				cout<<"Recv Edge ("<<p<<","<<q<<")\n";
#endif
				}
			}
			return MPI_SUCCESS;
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
	
	void printGraph(){
		
		for(iterator it = EdgeList.begin(); it != EdgeList.end(); it++){
			int len;
			len = (int) it->second.size();
			for(int i=0;i<len;i++){
				cout<<"("<<it->first<<','<<it->second[i]<<") \n";
			}
		}

	}

	void printLast(void){
		cout<<"My last node is ("<<lastp<<","<<lastq<<")\n";
	}


};

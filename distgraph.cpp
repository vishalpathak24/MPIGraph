#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include <assert.h>
#include <algorithm>

using namespace std;

#ifndef Edge
typedef struct Edge{
	int p;
	int q;
}Edge;
#endif

#ifndef GRAPH_TAG
#define GRAPH_TAG 2
#endif


class DistGraph{
	private:
		//map <int, vector <int> > EdgeList;
		bool *EdgeList;
		int *NEdges; //Number of Edges from a vertex btw (k_min to k_max)

	public:
		int lastp;
		int lastq;
		int k_max;
		int k_min;
		int N_Vertex;

		DistGraph(int k_min,int k_max,int N_Vertex){
			lastp=-1;
			lastq=-1;
			this.k_max = k_max;
			this.k_min = k_min;
			EdgeList = (bool *)callac((k_max-k_min)*sizeof(bool)); //initialize with zero
			NEdges = (int *)callac((k_max-k_min)*sizeof(int)); //initialize with zero
			this.N_Vertex = N_Vertex;
		}

		/* copy Constructor */
		EdgeGraph(const EdgeGraph &graph){
			this->EdgeList = graph.EdgeList;
			this->lastp = graph.lastp;
			this->lastq = graph.lastq;
			this->k_max = graph.k_max;
			this->k_min = graph.k_min;
			this.N_Vertex = graph.N_Vertex;
		}

		bool pushEdge(int p,int q){/* Returns True if pushEdge Created New Edge, can be used for finding if next round is needed*/
			int i,j;
			bool oldEdge = this->EdgeList[p-k_min][q];

			if(p>= k_min && p<=k_min){/* P is btw k */
				this->EdgeList[p-k_min][q] = true;
			}else{
				cout<<"THIS EDGE IS NOT MY RESPONSIBLITY .... Edge given to me "(<<p<<","<<q<<")\n";
				exit(-1);
			}


			if(q>= k_min && q<=k_min)/* Symmetric data is kept by this process only */
				this->EdgeList[q-k_min][p] = true;
			
			if (p >= lastp){
				lastp = p;
				lastq = q;
			}

			if ( p == lastp){
				if(q > lastq)
					lastq = q;
			}

			if(oldEdge != true){
					NEdges[p-k_min]++;
					return true;
			}
			return false;
		}

		int * pullEdge(int p){
			
			if(p == lastp){ /* this case is not handled */
			lastp = -1; /* Reset lastp */
			}
			
			return this->EdgeList[p-k_min];
		}

	
	bool hasEdge(int p,int q){
	
		if(p <= k_max && p>=k_min){
			return this->EdgeList[p][q];
		}else{
			cout<<"ASKED FOR OUT OF K EDGE TERMINATING....\n";
			exit(-1);
		}

		return false;
		
	}

	bool recvEdges(int s_id,MPI_Comm comm = MPI_COMM_WORLD){
			bool flag=false,pushEdgeflg;

			int buff,len,p,q;
			MPI_Status status;
			assert(MPI_Recv(&buff,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
			if(buff != -1){
				/* Not NULL CASE */
				p = buff;
				assert(MPI_Recv(&len,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
				for(int i = 0 ; i < len ; i++){
					assert(MPI_Recv(&q,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
					pushEdgeflg=this->pushEdge(p,q);
					flag = flag||pushEdgeflg;
#if _DBG_
				cout<<"Recv Edge ("<<p<<","<<q<<")\n";
#endif
				}
			}
			return flag;
	}

};

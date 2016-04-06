#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include <assert.h>
#include <algorithm>
#include <stdlib.h> //calloc 

using namespace std;


#ifndef GRAPH_TAG
#define GRAPH_TAG 2
#endif


class DistGraph{
	private:
		//map <int, vector <int> > EdgeList;
		bool **EdgeList;
		int *NEdges_PVert; //Number of Edges from a vertex btw (k_min to k_max)

	public:
		int lastp;
		int lastq;
		int k_max;
		int k_min;
		int N_Vertex;
		int N_Edges;

		DistGraph(int k_min,int k_max,int N_Vertex){
			lastp=-1;
			lastq=-1;
			this->k_max = k_max;
			this->k_min = k_min;
			EdgeList = new bool *[(k_max-k_min)+1]();//(bool **)calloc((k_max-k_min)+1,sizeof(bool)); //initialize with zero
			for(int i=0;i<=(k_max-k_min);i++)
				EdgeList[i] = new bool[N_Vertex]();//(bool *)calloc(N_Vertex,sizeof(bool));

			NEdges_PVert = new int[(k_max-k_min)+1];//(int *)calloc((k_max-k_min)+1,sizeof(int)); //initialize with zero
			this->N_Vertex = N_Vertex;
			N_Edges = 0; 
		}


		bool pushEdge(int p,int q){/* Returns True if pushEdge Created New Edge, can be used for finding if next round is needed*/

#if _DBG_
		cout<<"put Edge ("<<p<<','<<q<<")\n";
#endif

			bool oldEdge = this->EdgeList[p-k_min][q];

			if(p>= k_min && p<=k_max){/* P is btw k */
				EdgeList[p-k_min][q] = true;
			}else{
				cout<<"THIS EDGE IS NOT MY RESPONSIBLITY .... Edge given to me ("<<p<<","<<q<<")\n";
				exit(-1);
			}


			if(q>= k_min && q<=k_max){/* Symmetric data is kept by this process only */
				this->EdgeList[q-k_min][p] = true;
					if(oldEdge != true){
						N_Edges++;
					}
			}
			
			if (p >= lastp){
				lastp = p;
				lastq = q;
			}

			if ( p == lastp){
				if(q > lastq)
					lastq = q;
			}

			if(oldEdge != true){
					NEdges_PVert[p-k_min]++;
					N_Edges++;
					return true;
			}
			return false;
		}
	
	bool hasEdge(int p,int q){
#if _DBG_
		//cout<<"has Edge ("<<p<<','<<q<<")\n";
#endif
		if(p <= k_max && p>=k_min){
			return this->EdgeList[p-k_min][q];
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

	void printGraph(){
		cout<<"PRINTING DISTRIBUTED GRAPH.. \n";	
		for(int k=k_min;k<=k_max;k++)
			for(int j=0;j<N_Vertex;j++)
				if(this->hasEdge(k,j))
					cout<<"("<<k<<','<<j<<")\n";
	}

	int getTotalEdges(){
		return N_Edges;
	}
};

#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include <assert.h>

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
		void pushEdge(int p,int q){
			EdgeList[p].push_back(q);
		
		}

		vector <int> * pullEdge(int p){
			if(EdgeList.find(p) != EdgeList.end())
				return &EdgeList[p];
			else
				return NULL;
		}

		int sendEdge(int edgeNo,int d_id,MPI_Comm comm = MPI_COMM_WORLD){
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

			}else{
				int buf = -1;
				assert(MPI_Send(&buf,1,MPI_INT,d_id,GRAPH_TAG,comm) == MPI_SUCCESS );
			}
			return MPI_SUCCESS;
		}

		int recvEdge(int s_id,MPI_Comm comm = MPI_COMM_WORLD){
			int buff,len,p,q;
			MPI_Status status;
			assert(MPI_Recv(&buff,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
			if(buff != -1){
				/* Not NULL CASE */
				p = buff;
				assert(MPI_Recv(&len,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
				for(int i = 0 ; i < len ; i++){
					assert(MPI_Recv(&q,1,MPI_INT,s_id,GRAPH_TAG,comm,&status) == MPI_SUCCESS);
					EdgeList[p].push_back(q);
#if _DBG_
				cout<<"Recv Edge ("<<p<<","<<q<<")\n";
#endif
				}
			}
			return MPI_SUCCESS;
		}

};

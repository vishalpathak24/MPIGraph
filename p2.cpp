#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <vector>
#include <assert.h>

/** PROGRAM COMTROL DEFS **/
#define _DBG_ 1


/** DEFINING CONSTANTS **/
#define ROOT_PR 0


#define _USE_HEADER_
#include "edgegraph.cpp"



/** methods **/
int processID(vector <Edge> &edgeMap,int vertex); // Returns process id of process holding vertex and -1 if vertex detail is in multiple process 

using namespace std;

int main(int argc, char** argv){
	int myrank, nprocs;

   EdgeGraph eGraph;
   vector <Edge> edgeMap; //Maintains last edge of each process


   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

#ifdef _DBG_
   cout<<"myrank is "<<myrank<<'\n';
#endif

   /*string path = "./facebook_combined.txt"; int Nedges = 88234;
   int NVertex = 4039;*/

   string path = "./small_graph.txt"; //int Nedges = 16;
   int NVertex = 9;
   int NVertexPP = NVertex/nprocs;
   int NVertexRem = NVertex - NVertexPP*nprocs;

   int startk,endk;

   if(myrank < NVertexRem){ /* Remainder Process */
      startk = (NVertexPP +1)*myrank;
      endk = startk + (NVertexPP);
   }else{
      startk = NVertexRem + NVertexPP*myrank; //(NVerterxPP +1)*nVertexRem + NVertexPP*(myrank-nVertexRem)
      endk = startk + (NVertexPP-1);
   }

#if _DBG_
   cout<<"My startk = "<<startk<<"\n My endk ="<<endk<<'\n';
#endif

   ifstream graph_file;
   graph_file.open(path.c_str());
   int line=0,p,q;
   do{
      graph_file>>p>>q;
      line++;
   }while(p<startk);

   int n=0;
   do{
      n++;
      eGraph.pushEdge(p,q);
      if(!graph_file.eof())
         graph_file>>p>>q;
      else
         break;

      line++;   
   }while(p<=endk);

   graph_file.close();

#if _DBG_
   eGraph.printGraph();
#endif

   MPI_Barrier(MPI_COMM_WORLD);

/* creating copy of Edge Graph */
   EdgeGraph tcGraph = eGraph;   //Transitive Closure Graph
   EdgeGraph tempGraph;          //Keeps generated Edges for further comparison
   EdgeGraph toSendGraph;        // Temp Graph keeping edges to be sent
   bool nxtRound = false;
   
   do{
#if _DBG_
   cout<<"Rounds Running .... \n";
#endif

      nxtRound = false;
      
      for(int k=startk;k<=endk;k++){
         for(int i=0;i<NVertex;i++){
            if(tcGraph.hasEdge(k,i)){
               for(int j=0;j<NVertex;j++){
                  if( i != j && tcGraph.hasEdge(k,j) ){ // M[i][k] and M[k][i]
                     
                     if(i>= startk && i<=endk && !tcGraph.hasEdge(i,j)){
#if _DBG_
                        cout<<"Adding a edge ("<<i<<','<<j<<") \n";
#endif                        
                        tcGraph.pushEdge(i,j);
                        nxtRound = true;
                     }else{
                        if(!tempGraph.hasEdge(i,j)){
                           tempGraph.pushEdge(i,j);
                           toSendGraph.pushEdge(i,j);
                           nxtRound = true;
                        }
                     }
                     
                  }
               }
            }
         }
      }
              
      /* Sending/Recv of Edges prepared */
      for(int i=0;i<nprocs;i++){
         if(myrank == i){
            for(int j=0;j<nprocs;j++){
               if(myrank != j){//recvEdges from the process except itself
                  int newNodes;
                  MPI_Status status;
                  MPI_Recv(&newNodes,1,MPI_INT,j,GRAPH_TAG,MPI_COMM_WORLD,&status);
                  for(int temp = 0;temp<newNodes;temp++)
                     tcGraph.recvEdges(j); 
               }
            }
         }else{
            vector <int> toSendBuff; 
            int startIndex,endIndex;
            
            if(i < NVertexRem){ /* Remainder Process */
               startIndex = (NVertexPP +1)*i;
               endIndex = startIndex + (NVertexPP);
            }else{
               startIndex = NVertexRem + NVertexPP*i; //(NVerterxPP +1)*nVertexRem + NVertexPP*(myrank-nVertexRem)
               endIndex = startIndex + (NVertexPP-1);
            }

            for(EdgeGraph::iterator it = toSendGraph.begin(); it != toSendGraph.end() ; it++){               
               if(it->first >= startIndex && it->first <= endIndex){
                  toSendBuff.push_back(it->first);
               }
            }

            int newNodes;
            newNodes = (int) toSendBuff.size();
            MPI_Send(&newNodes,1,MPI_INT,i,GRAPH_TAG,MPI_COMM_WORLD);
            for(int temp=0;temp < newNodes; temp++){
               toSendGraph.sendEdges(toSendBuff[temp],i);
            }

            toSendBuff.clear();
         }
      
      }

      toSendGraph.clear();

      /* Deciding of NextRound */ 
      bool nxtRoundBuff;
#if _DBG_
      cout<<"nxtRound = "<<nxtRound<<'\n';
#endif      
      MPI_Allreduce(&nxtRound,&nxtRoundBuff,1,MPI::BOOL,MPI_LOR,MPI_COMM_WORLD);
      nxtRound = nxtRoundBuff;

   }while(nxtRound==true);

#if _DBG_
   cout<<"Rounds Complete... X \n Graph is \n";
#endif
   tcGraph.printGraph();


   MPI_Finalize();
	return 0;
}

int processID(vector <Edge> &edgeMap,int vertex){
   int i;
   for(i=0;i<(int)(edgeMap.size()-1);i++){
      if(edgeMap[i].p > vertex)
         return i;
      if(edgeMap[i].p == vertex) /* vertex is in border */
         return -1;
   }
   return i; //if its in the last process
}

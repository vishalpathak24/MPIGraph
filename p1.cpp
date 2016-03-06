/*
 * Author : Vishal Pathak
 * Topic : Transitive Colsure in Distributed Edge Graph
 */
 
#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <vector>
#include <assert.h>
#include <time.h>

/** PROGRAM COMTROL DEFS **/
#define _DBG_ 0
#define _TIMECALC_ 1

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

   string path = "./small_graph.txt"; int Nedges = 16;
   int NVertex = 9;

   int EdgePP = Nedges/nprocs; //Edges per Process;
   int rEdges = Nedges - EdgePP*nprocs;	//Remainder Edges;
   int NEdges;

   if(myrank < rEdges){
	   NEdges = EdgePP + 1;
   }else{
	   NEdges = EdgePP;
   }

   int startLine = 0;
   for(int i=0;i<myrank;i++){
	   if(i<rEdges){
		   startLine  = startLine + (EdgePP+1);
	   }else{
		   startLine = startLine + EdgePP;
	   }
   }
#if _DBG_
   cout<<"I will start for Line = "<<startLine;
#endif

   ifstream graph_file;
   graph_file.open(path.c_str());
   int line=0,p,q;
   while(line<startLine){
      graph_file>>p>>q;
      line++;
   }
   int n=0;
   while(line<startLine+NEdges){
      graph_file>>p>>q;
      eGraph.pushEdge(p,q);
      line++;
      n++;
   }

   graph_file.close();

/* Redistributing Edges */

int pbuff,qbuff;
   
   for(int i=0;i<nprocs;i++){
      if(myrank == i){
         pbuff = eGraph.lastp;
         qbuff = eGraph.lastq;
      }
      MPI_Bcast(&pbuff,1,MPI_INT,i,MPI_COMM_WORLD);
      MPI_Bcast(&qbuff,1,MPI_INT,i,MPI_COMM_WORLD);
      Edge e;
      e.p = pbuff;
      e.q = qbuff;
      edgeMap.push_back(e);  
   }

   if(myrank != ROOT_PR){
      /* 0th process will not send anything */
       eGraph.sendEdges(edgeMap[myrank-1].p,myrank-1); //Sends -1 signal if nothing to be sent
   }
   if(myrank != (nprocs -1)){
      /* Last Process will not receive any edges */
      eGraph.recvEdges(myrank+1);
   }

   eGraph.printLast();

   /* Updating edgeMap */
   edgeMap.clear();
   for(int i=0;i<nprocs;i++){
      if(myrank == i){
         pbuff = eGraph.lastp;
         qbuff = eGraph.lastq;
      }
      MPI_Bcast(&pbuff,1,MPI_INT,i,MPI_COMM_WORLD);
      MPI_Bcast(&qbuff,1,MPI_INT,i,MPI_COMM_WORLD);
      Edge e;
      e.p = pbuff;
      e.q = qbuff;
      edgeMap.push_back(e);  
   }

#if _DBG_
   cout<<"**PRINTING EDGE MAP**\n";
   for(int i=0;i<(int)edgeMap.size();i++){
      cout<<"For process ="<<i<<" p="<<edgeMap[i].p<<" q="<<edgeMap[i].q<<"\n";
   }
#endif

   int startk,endk;
   if(myrank == ROOT_PR){
      startk = 0;
   }else{
      startk = edgeMap[myrank - 1].p+1;
   }

   endk = edgeMap[myrank].p;

#if _DBG_
   cout<<"My startk = "<<startk<<"\n my endk = "<<endk;
#endif

/* creating copy of Edge Graph */
   EdgeGraph tcGraph = eGraph;   //Transitive Closure Graph
   EdgeGraph tempGraph;          //Keeps generated Edges for further comparison
   EdgeGraph toSendGraph;        // Temp Graph keeping edges to be sent
   bool nxtRound = false;

#if _TIMECALC_
   struct timespec startTime;
   if(myrank == ROOT_PR){
      assert(clock_gettime(CLOCK_MONOTONIC,&startTime) == 0);
   }
#endif
   
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
            if( i == ROOT_PR){
               startIndex = 0;
            }else{
              startIndex = edgeMap[i-1].p+1;
            }
            
            endIndex = edgeMap[i].p;

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

#if _TIMECALC_
   if(myrank == ROOT_PR){
      struct timespec endTime;
      assert(clock_gettime(CLOCK_MONOTONIC,&endTime) == 0);
      cout<<"Diffrence is time is"<<endTime.tv_nsec-startTime.tv_nsec<<'\n';
   }
#endif

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

/*
 * Author : Vishal Pathak
 * Topic : Transitive Colsure in Distributed Vertex Graph
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
#define _CLUSTER_OUT_ 1

/** DEFINING CONSTANTS **/
#define ROOT_PR 0
#define GRAPH_TAG 2
#define READY_RECV_TAG 1

#define _USE_HEADER_
#include "edgegraph.cpp"
#include "distgraph.cpp"


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

#if _DBG_
   cout<<"myrank is "<<myrank<<'\n';
#endif

#if _CLUSTER_OUT_
   ofstream globalStatus;
   char buff[30];
   sprintf(buff,"./VertexOutput/status_%d.txt",myrank);
   ofstream myStatus((const char*)buff,ofstream::out);
   if(myrank == ROOT_PR){
      globalStatus.open("./VertexOutput/status.txt",ofstream::out);
      globalStatus<<"We have started \n";
   }
#endif

   string path = "./facebook_combined_N200_E1924.txt";//int Nedges = 88234;
   int NVertex = 200;

   /*string path = "./small_graph.txt"; //int Nedges = 16;
   int NVertex = 9;*/
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

#if _CLUSTER_OUT_
   myStatus<<"My startk = "<<startk<<"\n my endk = "<<endk<<endl;
   myStatus<<"D_K,N_Edges,TimeTaken\n";
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
   DistGraph tcGraph(startk,endk,NVertex);//Transitive Closure Graph
/* Initilizing DistGraph */
#if _DBG_
   cout<<"Initilizing Transitive closure graph \n";
#endif   

   for(map < int,vector <int> >::iterator it = eGraph.begin();it != eGraph.end();it++){
      cout<<"Getting Edge for "<<it->first<<"\n";
      unsigned int len;
      len = (it->second).size();
      for(unsigned int i=0;i<len;i++){
#if _DBG_
         cout<<"putting Edge ("<<it->first<<","<<it->second[i]<<")\n";
#endif
         tcGraph.pushEdge(it->first,it->second[i]);
      }
   }


#if _DBG_
   cout<<"Initilization Transitive closure graph Done \n";
#endif

/* creating copy of Edge Graph */
   //EdgeGraph tempGraph;          //Keeps generated Edges for further comparison
   EdgeGraph toSendGraph;        // Temp Graph keeping edges to be sent
   bool nxtRound = true,flagBuff;
   bool nxtRoundBuff = false;


#if _TIMECALC_
   MPI_Barrier(MPI_COMM_WORLD);
   double startTime;
   if(myrank == ROOT_PR){
      startTime = MPI_Wtime();
   }
#endif

#if _CLUSTER_OUT_
   long unsigned int roundCount =0;
#endif 

   do{
#if _DBG_
   cout<<"Rounds Running .... \n";
#endif

#if _CLUSTER_OUT_
   double mystartTime;
   mystartTime = MPI_Wtime();
   int totalEdges = tcGraph.getTotalEdges();
   if(myrank == ROOT_PR){
      cout<<"Round "<<roundCount<<" started \n";
   }
#endif

   if(nxtRound == true){ /* if we prepared that nextRound is not required this process don't have any new edges to process */
      
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
                        //if(!tempGraph.hasEdge(i,j)){
                        //   tempGraph.pushEdge(i,j);
                           toSendGraph.pushEdge(i,j);
                        //   nxtRound = true; Next Round will be decided in end of receiving edges
                        //}
                     }
                     
                  }
               }
            }
         }
      }
   }
      /* Sending/Recv of Edges prepared */

#if _CLUSTER_OUT_
   #if _TIMECALC_
      double myendTime = MPI_Wtime();
      myStatus<<endk-startk<<","<<totalEdges<<","<<myendTime-mystartTime<<"\n";
   #endif
#endif

#if _DBG_
      cout<<"Sending and receiving New Edges prepared.. \n";
#endif

#if _CLUSTER_OUT_
      cout<<"I "<<myrank<<"at Sending/Reciving Message for Round\n";
#endif

      for(int i=0;i<nprocs;i++){
         if(myrank == i){

            cout<<"I "<<myrank<<"Reciving Message for Round\n";

            for(int j=0;j<nprocs;j++){
               if(myrank != j){//recvEdges from the process except itself
                  int newNodes;
                  MPI_Status status;
                  /* Sending Process j signal that its ready to receive data */
                  int tempBuff=1;
                  MPI_Send(&tempBuff,1,MPI_INT,j,READY_RECV_TAG,MPI_COMM_WORLD);
                  cout<<"I "<<myrank<<" Sent Signal Ready Recv to "<<j<<"\n";

                  MPI_Recv(&newNodes,1,MPI_INT,j,GRAPH_TAG,MPI_COMM_WORLD,&status);
                  for(int temp = 0;temp<newNodes;temp++){
                     flagBuff=tcGraph.recvEdges(j);
                     nxtRound = nxtRound || flagBuff; 
                  }
               }
            }
#if _CLUSTER_OUT_
      cout<<"I "<<myrank<<"Reciving Message done for Round\n";
#endif
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

            unsigned int newNodes;
            newNodes = toSendBuff.size();
            /* Waiting for Signal Before Sending Data */
            int tempBuff;
            MPI_Status status;
            MPI_Recv(&tempBuff,1,MPI_INT,i,READY_RECV_TAG,MPI_COMM_WORLD,&status);
            cout<<"I "<<myrank<<" Recv Signal Ready Recv from "<<i<<"\n";

            MPI_Send(&newNodes,1,MPI_INT,i,GRAPH_TAG,MPI_COMM_WORLD);
            for(unsigned int temp=0;temp < newNodes; temp++){
               toSendGraph.sendEdges(toSendBuff[temp],i);
            }

            toSendBuff.clear();
         }
      
      }

      toSendGraph.clear();

      /* Deciding of NextRound */ 
      nxtRoundBuff = false;
#if _DBG_
      cout<<"nxtRound = "<<nxtRound<<'\n';
#endif      
      MPI_Allreduce(&nxtRound,&nxtRoundBuff,1,MPI::BOOL,MPI_LOR,MPI_COMM_WORLD);

#if _CLUSTER_OUT_
   #if _TIMECALC_
      if(myrank == ROOT_PR){
         double myRoundEndTime = MPI_Wtime();
         globalStatus<<"Round "<<++roundCount<<" took "<<myRoundEndTime-mystartTime<<"\n";
      }
   #endif
#endif

   }while(nxtRoundBuff==true);

#if _CLUSTER_OUT_
   if(myrank == ROOT_PR){
      globalStatus<<"Round has Ended \n";
      globalStatus.close();
   }
   myStatus.close();
#endif


#if _TIMECALC_
   MPI_Barrier(MPI_COMM_WORLD);
   if(myrank == ROOT_PR){
      double endTime;
      endTime = MPI_Wtime();
      int nNodes;
      cout<<"Enter No of Nodes";
      cin>>nNodes;
      cout<<"Diffrence is time is"<<endTime-startTime<<'\n';
      ofstream timingFile("./Timingresult_vert.txt",ofstream::app);
      timingFile<<nNodes<<','<<nprocs<<','<<endTime-startTime<<'\n';
      timingFile.close();
   }
#endif

#if _DBG_
   cout<<"Rounds Complete... X \n Graph is \n";
   tcGraph.printGraph();
#endif

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

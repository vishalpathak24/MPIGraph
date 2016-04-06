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
   sprintf(buff,"./EdgeOutput/status_%d.txt",myrank);
   ofstream myStatus((const char*)buff,ofstream::out);

   if(myrank == ROOT_PR){
      globalStatus.open("./EdgeOutput/status.txt",ofstream::out);
      globalStatus<<"We have started \n";
   }
#endif

   string path = "./facebook_combined_N500_E8674.txt"; int Nedges = 8674;
   int NVertex = 500;

   /*string path = "./small_graph.txt"; int Nedges = 16;
   int NVertex = 9;*/

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

#if _DBG_
   cout<<"**PRINTING TEMP EDGE MAP**\n";
   for(unsigned int i=0;i<edgeMap.size();i++){
      cout<<"For process ="<<i<<" p="<<edgeMap[i].p<<" q="<<edgeMap[i].q<<"\n";
   }
#endif

   if(myrank != ROOT_PR){
      /* 0th process will not send anything */
       eGraph.sendEdges(edgeMap[myrank-1].p,myrank-1); //Sends -1 signal if nothing to be sent
   }
   if(myrank != (nprocs -1)){
      /* Last Process will not receive any edges */
      eGraph.recvEdges(myrank+1);
   }

MPI_Barrier(MPI_COMM_WORLD); //Wait for every one to complete send and Receive
/* Clearing the sent Edges from indiviual Process */
   if(myrank != ROOT_PR){
      /* 0th process will not clear anything */
       eGraph.clearEdge(edgeMap[myrank-1].p); //Sends -1 signal if nothing to be sent
   }

/*#if _DBG_
   cout<<"Printing the Graph after Redistributing I have \n";
   eGraph.printGraph();
#endif*/
//eGraph.printGraph();

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
   for(unsigned int i=0;i<edgeMap.size();i++){
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
   cout<<"My startk = "<<startk<<"\n my endk = "<<endk<<endl;
#endif

#if _CLUSTER_OUT_
   myStatus<<"My startk = "<<startk<<"\n my endk = "<<endk<<endl;
   myStatus<<"D_K,N_Edges,TimeTaken\n";
#endif

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

//   EdgeGraph tempGraph;                   //Keeps generated Edges for further comparison
   EdgeGraph toSendGraph;                   // Temp Graph keeping edges to be sent
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
                           flagBuff = tcGraph.pushEdge(i,j);
                           nxtRound = nxtRound||flagBuff;

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
      nxtRoundBuff=false;
#if _DBG_
      cout<<"My nxtRound = "<<nxtRound<<'\n';
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
      int nNodes=1;
      cout<<"Enter No of Nodes";
      cin>>nNodes;
      cout<<"Diffrence is time is"<<endTime-startTime<<'\n';
      ofstream timingFile("./Timingresult_edge.txt",ofstream::app);
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

#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <vector>

#define _USE_HEADER_
#include "edgegraph.cpp"

/** PROGRAM COMTROL DEFS **/
#define _DBG_ 1

/** CONSTANTS **/
#define ROOT_PR 0

using namespace std;

int main(int argc, char** argv){
	int myrank, nprocs;

   EdgeGraph eGraph;
   vector <Edge> edgeMap; //Maintains last edge of each process


   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

   string path = "./facebook_combined.txt"; int Nedges = 88234;

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
#if _DBG_
      cout<<"("<<p<<","<<q<<")\n";
#endif
      eGraph.pushEdge(p,q);
      line++;
      n++;
   }
#if _DBG_
   cout<<"Total edges read = "<<n<<"\n";
#endif 
   graph_file.close();

   int pbuff,qbuff;
   for(int i=0;i<nprocs;i++){
      if(myrank == i){
         pbuff = p;
         qbuff = q;
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
   for(int i=0;i<edgeMap.size();i++){
      cout<<"For process ="<<i<<" p="<<edgeMap[i].p<<" q="<<edgeMap[i].q<<"\n";
   }
#endif

if(myrank == ROOT_PR){
   int start,end;
   cout<<"Enter the start vertex \n";
   cin>>start;
   cout<<"Enter the end vertex \n";
   cin>>end;
}


	return 0;
}

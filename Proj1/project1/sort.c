#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef OPENMP_HARNESS
#include <omp.h>
#endif

#ifdef MPI_HARNESS
#include <mpi.h>
#endif

#ifdef HYBRID_HARNESS
#include <omp.h>
#include <mpi.h>
#endif

#include "sort.h"
#include "graph.h"

struct Graph *countSortEdgesBySource (struct Graph *graph)
{
    printf("*** START Count Sort Edges By Source *** \n");
    int i;
    int key;
    int pos;
    struct Edge *sorted_edges_array = newEdgeArray(graph->num_edges);

    // auxiliary arrays, allocated at the start up of the program
    int *vertex_count = (int *)malloc(graph->num_vertices * sizeof(int)); // needed for Counting Sort

    for(i = 0; i < graph->num_vertices; ++i)
    {
        vertex_count[i] = 0;
    }

    // count occurrence of key: id of a source vertex
    for(i = 0; i < graph->num_edges; ++i)
    {
        key = graph->sorted_edges_array[i].src;
        vertex_count[key]++;
    }

    // transform to cumulative sum
    for(i = 1; i < graph->num_vertices; ++i)
    {
        vertex_count[i] += vertex_count[i - 1];
    }

    // fill-in the sorted array of edges
    for(i = graph->num_edges - 1; i >= 0; --i)
    {
        key = graph->sorted_edges_array[i].src;
        pos = vertex_count[key] - 1;
        sorted_edges_array[pos] = graph->sorted_edges_array[i];
        vertex_count[key]--;
    }



    free(vertex_count);
    free(graph->sorted_edges_array);

    graph->sorted_edges_array = sorted_edges_array;

    return graph;

}

#ifdef OPENMP_HARNESS
struct Graph *radixSortEdgesBySourceOpenMP (struct Graph *graph)
{

    printf("*** START Radix Sort Edges By Source OpenMP *** \n");

    int radix_n = 0;
    int nbit = 8; //8
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256
    int radix_max_loop = 32/nbit;
    int thread_max = omp_get_max_threads(); // 4

    // auxiliary arrays, allocated at the start up of the program
    int *vertex_count = (int*)malloc(thread_max*num_keys*sizeof(int));
    struct Edge *sorted_edges_array = (struct Edge *)malloc ((graph->num_edges)*sizeof(struct Edge));
    
    for ( radix_n = 0; radix_n < radix_max_loop; ++radix_n)
    {   
        graph = CountSortEdgesBySource(graph,  radix_n,  nbit, &vertex_count, &sorted_edges_array);
    //return graph;    
    } // end of radix for loop
    free(sorted_edges_array);
    free(vertex_count);

    return graph;
}

struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int** vertex_count, struct Edge ** sorted_edges_array){

    int thread_max = omp_get_max_threads(); // 4
    


    int radix_mask = (1 << nbit ) -1;
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256
    //printf("num_keys %d\n", (int)num_keys);
    
    memset(*vertex_count, 0, thread_max*num_keys*sizeof(int));
    //struct Edge **sorted_edges_array = &ptr;
    int num_edges = graph->num_edges;
    //int num_vertices = graph->num_vertices;
    struct Edge **edges_array = &graph->sorted_edges_array;
    struct Edge * temp_array_pointer = NULL;
    //printf("vector count=%d\n",num_vertices);
    
    //for(int k=0;k<num_edges;++k){
    //    printf("src[%d]=%d\n",k,(*edges_array)[k].src);
    //}
    //printf("=========   %d th radix ==========\n",radix_n);
    //printEdgeArray2( graph->sorted_edges_array, graph->num_edges,radix_n,nbit);
#pragma omp parallel default(shared)
    {
        int i=0;
        int tid=0;
        int tid_start=0;
        int tid_end=0;
        int base=0;
        int key=0;
        int pos=0;

        tid = omp_get_thread_num();
        //printf("tid=%d\n",tid);
        tid_start = tid*(num_edges/thread_max +1);
        tid_end = (tid+1)*(num_edges/thread_max +1)-1;
        if(tid_end >= num_edges) tid_end = num_edges-1; 
        
        // count occurrence of key: id of a source vertex
        //for(tid=0; tid < thread_max; ++tid){    
            for(i = tid_start; i <= tid_end; ++i) {
                key = ( (*edges_array)[i].src >> (radix_n *nbit) ) & radix_mask;
                (*vertex_count)[(tid*num_keys)+key]++;
                //printf("[step 2] tid,i=[%d,%d], src=%d, key=%d, vc[%d][%d]=%d\n", tid,i,(*edges_array)[i].src,key, tid,key,vertex_count[(tid*num_keys)+key]);
            }
        //}
        //printf("done tid=%d\n",tid);

        #pragma omp barrier
        #pragma omp single
        //if(tid==0) 
        {
            base=0;
            // transform to cumulative sum
            //printf("thread id=%d\n",tid);
            
            for(int j = 0; j < num_keys; ++j) {
                for(int ttid =0;ttid <  thread_max; ++ttid ){
                    base = base + (*vertex_count)[(ttid*num_keys)+j];
                    //printf("[step 3] ttid, i=[%d, %d] ,base= %d, vc[%d][%d]= %d\n",ttid,i,base,ttid,i,vertex_count[(ttid*num_keys)+i]);
                    (*vertex_count)[(ttid*num_keys)+j] = base;   
                }
            }
        }
        #pragma omp barrier

        // fill-in the sorted array of edges
        //for(tid=0; tid < thread_max; ++tid){
            for(i = tid_end; i >= tid_start; --i) {
                key = ( (*edges_array)[i].src >> (radix_n *nbit) ) & radix_mask;
                pos = (*vertex_count)[(tid*num_keys)+key] - 1;
                //printf("[step 4][key,pos,v_cnt[k]]: %d %d %d\n",key,pos,vertex_count[(tid*num_keys)+key]);
                (*sorted_edges_array)[pos] = (*edges_array)[i];
                (*vertex_count)[(tid*num_keys)+key]--;               
            }
        //}    
    }
    //printEdgeArray2( graph->sorted_edges_array, graph->num_edges,radix_n,nbit);
    //printf("=========   end of %d radix==========\n",radix_n);
    
    temp_array_pointer = *edges_array;
    *edges_array = *sorted_edges_array;
    *sorted_edges_array = temp_array_pointer;   

    return graph;
}

#endif

#ifdef MPI_HARNESS
struct Graph *radixSortEdgesBySourceMPI (struct Graph *graph)
{

    printf("*** START Radix Sort Edges By Source MPI*** \n");

    int radix_n = 0;
    int nbit = 8; //8   // nbit in radix sort should be power of 2
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256
    int radix_max_loop = 32/nbit; 

    // auxiliary arrays, allocated at the start up of the program
    int *vertex_count = (int*)malloc(num_keys*sizeof(int));
    struct Edge *sorted_edges_array = (struct Edge *)malloc ((graph->num_edges)*sizeof(struct Edge));
    

    for ( radix_n = 0; radix_n < radix_max_loop; ++radix_n)
    {   
        graph = CountSortEdgesBySource(graph,  radix_n,  nbit, &vertex_count, &sorted_edges_array);
    //return graph;    
    } // end of radix for loop
    free(sorted_edges_array);
    free(vertex_count);

    return graph;
}
struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int**vertex_count, struct Edge **sorted_edges_array){

    int thread_max; // 4    
    MPI_Comm_size(MPI_COMM_WORLD, &thread_max);

    int radix_mask = (1 << nbit ) -1;
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256

    memset(*vertex_count, 0, num_keys*sizeof(int));
    memset(*sorted_edges_array, 0, (graph->num_edges)*sizeof(struct Edge));
    int num_edges = graph->num_edges;
    struct Edge **edges_array_global = &graph->sorted_edges_array;

    {
        int i=0;
        int world_rank=0;
        int tid_start=0;
        int tid_end=0;
        int base=0;
        int key=0;
        int pos=0;
        int world_size;
        int tag=0;
	    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

        tid_start = world_rank*(num_edges/thread_max +1);
        tid_end = (world_rank+1)*(num_edges/thread_max +1)-1;
        if(tid_end >= num_edges) tid_end = num_edges-1; 
        
        // count occurrence of key: id of a source vertex
            for(i = tid_start; i <= tid_end; ++i) {
                key = ( (*edges_array_global)[i].src >> (radix_n *nbit) ) & radix_mask;
                (*vertex_count)[key]++;
            }
        if(world_size > 1 ){
            if(world_rank== world_size-1){
                base=0;
                MPI_Send(&base, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
            }
            // transform to cumulative sum
            for(int j = 0; j < num_keys; ++j) {
                    if(world_rank == 0){
                        MPI_Recv(&base, 1, MPI_INT, world_size-1, tag, MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE);
                        base += (*vertex_count)[j];
                        (*vertex_count)[j] = base;
                        MPI_Send(&base, 1, MPI_INT, world_rank+1, tag, MPI_COMM_WORLD);                        
                    }else if(world_rank < world_size-1){
                        MPI_Recv(&base, 1, MPI_INT, world_rank-1, tag, MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE);
                        base += (*vertex_count)[j];
                        (*vertex_count)[j] = base;
                        MPI_Send(&base, 1, MPI_INT, world_rank+1, tag, MPI_COMM_WORLD);
                    }else if(world_rank== world_size-1){
                        MPI_Recv(&base, 1, MPI_INT, world_rank-1, tag, MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE);
                        base += (*vertex_count)[j];
                        (*vertex_count)[j] = base;
                        MPI_Send(&base, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                    }
            }
            if(world_rank==0){
                MPI_Recv(&base, 1, MPI_INT, world_size-1, tag, MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE);
            }
        } else { // if world size == 1
            for(int j = 0; j < num_keys; ++j) {
                base += (*vertex_count)[j];
                (*vertex_count)[j] = base;
            }
        }
        // fill-in the sorted array of edges
            for(i = tid_end; i >= tid_start; --i) {
                key = ( (*edges_array_global)[i].src >> (radix_n *nbit) ) & radix_mask;
                pos = (*vertex_count)[key] - 1;
                (*sorted_edges_array)[pos] = (*edges_array_global)[i];
                (*vertex_count)[key]--;               
            }
            
            MPI_Allreduce( *sorted_edges_array, *edges_array_global, 2*num_edges, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }
    return graph;
}
#endif

#ifdef HYBRID_HARNESS
struct Graph *radixSortEdgesBySourceHybrid (struct Graph *graph)
{

    printf("*** START Radix Sort Edges By Source Hybrid*** \n");
    int radix_n = 0;
    int nbit = 8; //8   // nbit in radix sort should be power of 2
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256
    int radix_max_loop = 32/nbit; 
    int thread_max = omp_get_max_threads(); // 4
    
    // auxiliary arrays, allocated at the start up of the program
    int *vertex_count = (int*)malloc(num_keys*thread_max*sizeof(int));
    struct Edge *sorted_edges_array = (struct Edge *)malloc ((graph->num_edges)*sizeof(struct Edge));
    
    for ( radix_n = 0; radix_n < radix_max_loop; ++radix_n)
    {   
        graph = CountSortEdgesBySource(graph,  radix_n,  nbit, &vertex_count, &sorted_edges_array);
   
    } // end of radix for loop
    free(sorted_edges_array);
    free(vertex_count);

    return graph;
}
struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int**vertex_count, struct Edge **sorted_edges_array){

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int tag=0;
    
    int thread_max = omp_get_max_threads();

    int radix_mask = (1 << nbit ) -1;
    size_t num_keys =1; // use size_t data type because int data type will overflow at 2^32
    for(int i=0;i<nbit;i++) num_keys *= 2; // 2^8 = 256

    memset(*vertex_count, 0, thread_max*num_keys*sizeof(int));
    memset(*sorted_edges_array, 0, (graph->num_edges)*sizeof(struct Edge));
    int num_edges = graph->num_edges;
    struct Edge **edges_array_global = &graph->sorted_edges_array;

    #pragma omp parallel default(shared)
    {
        int i=0;
        int world_rank=0;
        int tid_start=0;
        int tid_end=0;
        int rid_start=0;
        int rid_end=0;
        int base=0;
        int key=0;
        int pos=0;

        int tid = omp_get_thread_num();
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

        
        rid_start = world_rank*(num_edges/world_size +1);
        rid_end = (world_rank+1)*(num_edges/world_size +1)-1;
        if(rid_end >= num_edges) rid_end = num_edges-1;

        int num_ele = rid_end - rid_start +1;
        tid_start = rid_start+ (tid*(num_ele/thread_max +1));
        tid_end = rid_start+ ((tid+1)*(num_ele/thread_max +1))-1;
        if(tid_end >= (rid_start+num_ele)) tid_end = rid_start+ num_ele-1; 
        
        // count occurrence of key: id of a source vertex
            for(i = tid_start; i <= tid_end; ++i) {
                key = ( (*edges_array_global)[i].src >> (radix_n *nbit) ) & radix_mask;
                (*vertex_count)[(tid*num_keys)+key]++;
            }
        #pragma omp master
        {
            //MPI_Barrier(MPI_COMM_WORLD);
        }
        #pragma omp barrier
        #pragma omp master
        {
            if(world_size > 1){
                if(world_rank== world_size-1){
                    base=0;
                    MPI_Send(&base, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                }
                // transform to cumulative sum
                for(int j = 0; j < num_keys; ++j) {
                        if(world_rank == 0){
                            MPI_Recv(&base, 1, MPI_INT, world_size-1, tag, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);
                            for(int ttid=0;ttid<thread_max;++ttid){
                                base += (*vertex_count)[(ttid*num_keys)+j];
                                (*vertex_count)[(ttid*num_keys)+j] = base;
                            }
                            MPI_Send(&base, 1, MPI_INT, world_rank+1, tag, MPI_COMM_WORLD);
                        }else if(world_rank < world_size-1){
                            MPI_Recv(&base, 1, MPI_INT, world_rank-1, tag, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);
                            for(int ttid=0;ttid<thread_max;++ttid){
                                base += (*vertex_count)[(ttid*num_keys)+j];
                                (*vertex_count)[(ttid*num_keys)+j] = base;
                            }
                            MPI_Send(&base, 1, MPI_INT, world_rank+1, tag, MPI_COMM_WORLD);
                        }else if(world_rank== world_size-1){
                            MPI_Recv(&base, 1, MPI_INT, world_rank-1, tag, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE);
                            for(int ttid=0;ttid<thread_max;++ttid){
                                base += (*vertex_count)[(ttid*num_keys)+j];
                                (*vertex_count)[(ttid*num_keys)+j] = base;
                            }
                        
                            MPI_Send(&base, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
                        }
                }
                if(world_rank==0){
                    MPI_Recv(&base, 1, MPI_INT, world_size-1, tag, MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE);
                }
            }else if(world_size==1){  // if world size ==1
                base =0;
                for(int j = 0; j < num_keys; ++j) {
                    for(int ttid=0;ttid<thread_max;++ttid){
                        base += (*vertex_count)[(ttid*num_keys)+j];
                        (*vertex_count)[(ttid*num_keys)+j] = base;
                    }
                }
            } // end of if world size == 0
            //MPI_Barrier(MPI_COMM_WORLD);
        } // end of if omp master
        #pragma omp barrier

        // fill-in the sorted array of edges
            for(i = tid_end; i >= tid_start; --i) {
                key = ( (*edges_array_global)[i].src >> (radix_n *nbit) ) & radix_mask;
                pos = (*vertex_count)[(tid*num_keys)+key] - 1;
                (*sorted_edges_array)[pos] = (*edges_array_global)[i];
                (*vertex_count)[(tid*num_keys)+key]--;               
            }
        #pragma omp barrier
        #pragma omp master
        {
            MPI_Allreduce( *sorted_edges_array, *edges_array_global, 2*num_edges, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        }        
    }
    return graph;
}
#endif

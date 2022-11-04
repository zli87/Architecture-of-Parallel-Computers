#ifndef SORT_H
#define SORT_H

#include "edgelist.h"
#include "graph.h"
// Order edges by id of a source vertex,
// using the Counting Sort
// Complexity: O(E + V)

#ifdef OPENMP_HARNESS
struct Graph *radixSortEdgesBySourceOpenMP2 (struct Graph *graph);
struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int** vertex_count, struct Edge ** sorted_edges_array);
#endif

#ifdef MPI_HARNESS
struct Graph *radixSortEdgesBySourceMPI (struct Graph *graph);
struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int**, struct Edge **);
#endif

#ifdef HYBRID_HARNESS
struct Graph *radixSortEdgesBySourceHybrid (struct Graph *graph);
struct Graph * CountSortEdgesBySource(struct Graph *graph, int radix_n, int nbit, int**, struct Edge **);

#endif

struct Graph *countSortEdgesBySource (struct Graph *graph);
struct Graph *radixSortEdgesBySource (struct Graph *graph);

extern int numThreads;

#endif

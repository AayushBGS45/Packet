/*
 * routing.h — Routing algorithm interface
 *
 * Two routing algorithms are provided:
 *
 * 1. Dijkstra's algorithm (shortest-cost path)
 *    - Uses weighted link costs.
 *    - Guarantees the minimum total routing cost.
 *    - Time complexity: O(V²) with the array-based implementation here.
 *      An O((V + E) log V) version using a priority queue is possible
 *      but unnecessary for networks up to MAX_ROUTERS = 20 nodes.
 *
 * 2. Breadth-First Search (minimum-hop path)
 *    - Treats all links as having equal weight (weight = 1).
 *    - Guarantees the fewest router-to-router hops.
 *    - Time complexity: O(V + E).
 *    - Useful when latency per hop dominates over link cost.
 *
 * Key difference:
 *   Dijkstra minimises total COST.
 *   BFS minimises total HOP COUNT.
 *   These may yield different paths (see README for an example).
 */

#ifndef ROUTING_H
#define ROUTING_H

#include "network.h"
#include "router.h"
#include <stdbool.h>

/* ── constants ───────────────────────────────────────────────────────── */

/** Sentinel returned when no path exists. */
#define NO_PATH INF_COST

/* ── data types ──────────────────────────────────────────────────────── */

/**
 * RouteAlgorithm — which algorithm to use when computing paths.
 */
typedef enum {
    ALGO_DIJKSTRA = 1,   /* minimum cost */
    ALGO_BFS      = 2    /* minimum hops */
} RouteAlgorithm;

/**
 * PathResult — output of a routing computation.
 *
 * Fields
 * ------
 * path[]      : node indices from source to destination.
 * hop_count   : number of entries in path[] (= number of hops + 1 nodes).
 * total_cost  : sum of link costs along the path (Dijkstra) or hop
 *               count (BFS; each hop has cost 1).
 * total_delay : sum of link delays (ms) along the path.
 * reachable   : false when no path exists.
 */
typedef struct {
    int  path[MAX_ROUTERS];
    int  hop_count;
    int  total_cost;
    int  total_delay_ms;
    bool reachable;
} PathResult;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * routing_dijkstra — compute the minimum-cost path using Dijkstra's
 *                    algorithm.
 *
 * Failed routers (active == false) are excluded from the search.
 * A router must be active to act as source, destination, or intermediate.
 *
 * @param net         Network topology.
 * @param routers     Router state array.
 * @param num_routers Total number of routers.
 * @param src         Source router index.
 * @param dst         Destination router index.
 * @param result      Populated on return.
 */
void routing_dijkstra(const Network *net,
                      const Router routers[], int num_routers,
                      int src, int dst,
                      PathResult *result);

/**
 * routing_bfs — compute the minimum-hop path using BFS.
 *
 * Treats the network as unweighted (each link has cost 1).
 * Failed routers are still excluded.
 *
 * @param net         Network topology.
 * @param routers     Router state array.
 * @param num_routers Total number of routers.
 * @param src         Source router index.
 * @param dst         Destination router index.
 * @param result      Populated on return.
 */
void routing_bfs(const Network *net,
                 const Router routers[], int num_routers,
                 int src, int dst,
                 PathResult *result);

/**
 * routing_compute — dispatch to the selected algorithm.
 *
 * @param algo  ALGO_DIJKSTRA or ALGO_BFS.
 */
void routing_compute(RouteAlgorithm algo,
                     const Network *net,
                     const Router routers[], int num_routers,
                     int src, int dst,
                     PathResult *result);

/**
 * routing_print_path — display the reconstructed path to stdout.
 *
 * Example output:
 *   Router-A -> Router-B -> Router-D   (cost 6, 2 hops, 20 ms delay)
 */
void routing_print_path(const PathResult *result,
                        const Router routers[]);

/**
 * routing_algo_name — return a human-readable name for the algorithm.
 */
const char *routing_algo_name(RouteAlgorithm algo);

#endif /* ROUTING_H */

/*
 * routing.c — Routing algorithm implementations
 *
 * Dijkstra's algorithm
 * --------------------
 * Classic O(V²) array-based implementation.  For each of V iterations we
 * pick the unvisited node with the minimum tentative distance and relax
 * its neighbours.  V ≤ MAX_ROUTERS = 20, so this is fast enough.
 *
 * BFS (Breadth-First Search)
 * --------------------------
 * Uses a simple circular queue.  Visits nodes level-by-level, expanding
 * all neighbours before going deeper.  Because all edges have weight 1 in
 * this unweighted view, BFS guarantees the minimum hop count.
 */

#include "routing.h"
#include "network.h"
#include "router.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* ── internal helper: reconstruct path from parent[] array ─────────── */

/**
 * reconstruct_path — fill result->path[] from the parent[] array.
 *
 * parent[i] = the node we came from when we first visited i.
 * We trace backwards from dst to src and then reverse the array.
 */
static void reconstruct_path(int parent[], int src, int dst,
                              PathResult *result,
                              const Network *net)
{
    /* Trace backwards from dst → src */
    int tmp[MAX_ROUTERS];
    int len = 0;
    int cur = dst;

    while (cur != -1 && len < MAX_ROUTERS) {
        tmp[len++] = cur;
        cur = parent[cur];
    }

    /* Reverse into result->path[] */
    for (int i = 0; i < len; i++) {
        result->path[i] = tmp[len - 1 - i];
    }
    result->hop_count = len;

    /* Accumulate delay along the path */
    result->total_delay_ms = 0;
    for (int i = 0; i + 1 < len; i++) {
        int u = result->path[i];
        int v = result->path[i + 1];
        result->total_delay_ms += net->attr[u][v].delay_ms;
    }

    (void)src; /* src is always result->path[0] after reverse */
}

/* ── Dijkstra's algorithm ────────────────────────────────────────────── */

void routing_dijkstra(const Network *net,
                      const Router routers[], int num_routers,
                      int src, int dst,
                      PathResult *result)
{
    memset(result, 0, sizeof *result);
    result->reachable = false;

    /* Sanity: source and destination must be active */
    if (!router_is_valid(num_routers, src) ||
        !router_is_valid(num_routers, dst) ||
        !routers[src].active ||
        !routers[dst].active) {
        return;
    }

    if (src == dst) {
        /* Trivial path of length 1 */
        result->path[0]      = src;
        result->hop_count    = 1;
        result->total_cost   = 0;
        result->total_delay_ms = 0;
        result->reachable    = true;
        return;
    }

    int  dist[MAX_ROUTERS];
    bool visited[MAX_ROUTERS];
    int  parent[MAX_ROUTERS];

    for (int i = 0; i < num_routers; i++) {
        dist[i]    = INF_COST;
        visited[i] = false;
        parent[i]  = -1;
    }
    dist[src] = 0;

    for (int iter = 0; iter < num_routers; iter++) {
        /* Pick the unvisited node with the smallest tentative distance */
        int u   = -1;
        int min = INF_COST;

        for (int i = 0; i < num_routers; i++) {
            if (!visited[i] && dist[i] < min) {
                min = dist[i];
                u   = i;
            }
        }

        if (u == -1) {
            break;   /* remaining nodes are unreachable */
        }

        visited[u] = true;

        /*
         * Relax all edges from u.
         * A neighbour v is considered only when:
         *   - there is a direct link (cost > 0)
         *   - v has not been finalised
         *   - v's router is active (failed routers are avoided)
         */
        for (int v = 0; v < num_routers; v++) {
            if (!visited[v] &&
                net->cost[u][v] > 0 &&
                routers[v].active) {

                int new_dist = dist[u] + net->cost[u][v];
                if (new_dist < dist[v]) {
                    dist[v]   = new_dist;
                    parent[v] = u;
                }
            }
        }
    }

    if (dist[dst] == INF_COST) {
        return;   /* destination is unreachable */
    }

    result->total_cost = dist[dst];
    result->reachable  = true;
    reconstruct_path(parent, src, dst, result, net);
}

/* ── BFS: minimum-hop routing ────────────────────────────────────────── */

void routing_bfs(const Network *net,
                 const Router routers[], int num_routers,
                 int src, int dst,
                 PathResult *result)
{
    memset(result, 0, sizeof *result);
    result->reachable = false;

    if (!router_is_valid(num_routers, src) ||
        !router_is_valid(num_routers, dst) ||
        !routers[src].active ||
        !routers[dst].active) {
        return;
    }

    if (src == dst) {
        result->path[0]    = src;
        result->hop_count  = 1;
        result->total_cost = 0;
        result->total_delay_ms = 0;
        result->reachable  = true;
        return;
    }

    bool visited[MAX_ROUTERS];
    int  parent[MAX_ROUTERS];
    int  queue[MAX_ROUTERS];   /* simple FIFO, at most V elements */
    int  head = 0, tail = 0;

    for (int i = 0; i < num_routers; i++) {
        visited[i] = false;
        parent[i]  = -1;
    }

    visited[src]      = true;
    queue[tail++]     = src;

    while (head < tail) {
        int u = queue[head++];

        if (u == dst) {
            break;
        }

        for (int v = 0; v < num_routers; v++) {
            if (!visited[v] &&
                net->cost[u][v] > 0 &&
                routers[v].active) {

                visited[v]   = true;
                parent[v]    = u;
                queue[tail++] = v;
            }
        }
    }

    if (parent[dst] == -1 && dst != src) {
        return;   /* unreachable */
    }

    result->reachable = true;

    /* Reconstruct path; total_cost = hop count (each hop costs 1) */
    reconstruct_path(parent, src, dst, result, net);
    result->total_cost = result->hop_count - 1;   /* hops, not link cost */
}

/* ── dispatch ────────────────────────────────────────────────────────── */

void routing_compute(RouteAlgorithm algo,
                     const Network *net,
                     const Router routers[], int num_routers,
                     int src, int dst,
                     PathResult *result)
{
    switch (algo) {
        case ALGO_BFS:
            routing_bfs(net, routers, num_routers, src, dst, result);
            break;

        case ALGO_DIJKSTRA:
        default:
            routing_dijkstra(net, routers, num_routers, src, dst, result);
            break;
    }
}

/* ── output helpers ──────────────────────────────────────────────────── */

void routing_print_path(const PathResult *result,
                        const Router routers[])
{
    if (!result->reachable) {
        printf("  (no route)\n");
        return;
    }

    for (int i = 0; i < result->hop_count; i++) {
        printf("%s", routers[result->path[i]].name);
        if (i + 1 < result->hop_count) {
            printf(" -> ");
        }
    }
    printf("\n");
    printf("  Cost: %d | Hops: %d | Propagation delay: %d ms\n",
           result->total_cost,
           result->hop_count - 1,
           result->total_delay_ms);
}

const char *routing_algo_name(RouteAlgorithm algo)
{
    switch (algo) {
        case ALGO_DIJKSTRA: return "Dijkstra (minimum cost)";
        case ALGO_BFS:      return "BFS (minimum hops)";
        default:            return "Unknown";
    }
}

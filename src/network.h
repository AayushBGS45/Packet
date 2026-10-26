/*
 * network.h — Network topology interface
 *
 * The network is modelled as a weighted, undirected graph stored as an
 * adjacency matrix.  A value of 0 (or INF_COST) means no link exists.
 * Link weights represent routing cost (a proxy for distance or latency).
 *
 * Each link also has an associated delay (ms) and bandwidth (Mbps) used
 * by the packet simulation to estimate end-to-end transmission time.
 *
 * NOTE: This is a simulation.  No actual network packets are transmitted.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "router.h"
#include <stdbool.h>

/* ── constants ───────────────────────────────────────────────────────── */

/** Sentinel value meaning "no path" or "no link". */
#define INF_COST 999999

/** Default link delay in milliseconds when not explicitly set. */
#define DEFAULT_LINK_DELAY_MS 10

/** Default link bandwidth in Mbps when not explicitly set. */
#define DEFAULT_LINK_BW_MBPS  100.0

/* ── data types ──────────────────────────────────────────────────────── */

/**
 * LinkAttr — per-link attributes beyond the routing cost.
 *
 * Fields
 * ------
 * delay_ms   : one-way propagation delay on this link (milliseconds).
 * bw_mbps    : link bandwidth (Mbps), used to compute transmission delay.
 * utilisation: packets forwarded over this link in the current session.
 */
typedef struct {
    int    delay_ms;
    double bw_mbps;
    int    utilisation;
} LinkAttr;

/**
 * Network — complete topology state.
 *
 * cost[i][j] holds the routing cost from router i to router j.
 *            0 means no direct link; INF_COST is never stored directly.
 * attr[i][j] holds per-link attributes for link (i, j).
 *            Only meaningful when cost[i][j] > 0.
 *
 * The matrix is kept symmetric (undirected graph).
 */
typedef struct {
    int      cost[MAX_ROUTERS][MAX_ROUTERS];
    LinkAttr attr[MAX_ROUTERS][MAX_ROUTERS];
} Network;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * network_init — initialise a Network to "no links".
 * Fills the cost matrix with 0 and attributes with defaults.
 */
void network_init(Network *net, int num_routers);

/**
 * network_init_defaults — set up the default 4-router topology from
 * the original project so that first-time users see a working example.
 *
 * Default cost matrix (0 = no link):
 *      A  B  C  D
 *   A  0  2  5  0
 *   B  2  0  1  4
 *   C  5  1  0  1
 *   D  0  4  1  0
 */
void network_init_defaults(Network *net);

/**
 * network_add_link — create or update a bidirectional link.
 *
 * @param net       Network to modify.
 * @param num_routers  Number of routers (for bounds check).
 * @param u, v      Router indices (0-based).
 * @param cost      Routing cost (must be > 0).
 * @param delay_ms  Propagation delay in milliseconds (>= 0).
 * @param bw_mbps   Bandwidth in Mbps (> 0).
 * @return          true on success, false if parameters are invalid.
 */
bool network_add_link(Network *net, int num_routers,
                      int u, int v, int cost,
                      int delay_ms, double bw_mbps);

/**
 * network_remove_link — remove the link between u and v.
 *
 * @return true on success, false if no link existed or IDs invalid.
 */
bool network_remove_link(Network *net, int num_routers, int u, int v);

/**
 * network_has_link — return true if a direct link exists between u and v.
 */
bool network_has_link(const Network *net, int u, int v);

/**
 * network_get_cost — return the routing cost of link (u, v),
 *                    or INF_COST if there is no link.
 */
int network_get_cost(const Network *net, int u, int v);

/**
 * network_print_topology — display all links and their attributes.
 */
void network_print_topology(const Network *net,
                             const Router routers[], int num_routers);

/**
 * network_print_matrix — display the raw adjacency/cost matrix.
 */
void network_print_matrix(const Network *net,
                           const Router routers[], int num_routers);

/**
 * network_link_utilisation — print per-link utilisation counters.
 */
void network_link_utilisation(const Network *net,
                               const Router routers[], int num_routers);

/**
 * network_reset_utilisation — zero all link utilisation counters.
 */
void network_reset_utilisation(Network *net, int num_routers);

#endif /* NETWORK_H */

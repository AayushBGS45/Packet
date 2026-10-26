/*
 * network.c — Network topology implementation
 */

#include "network.h"

#include <stdio.h>
#include <string.h>

/* ── internal helpers ────────────────────────────────────────────────── */

/** Zero-initialise the attribute for one directed edge. */
static void attr_clear(LinkAttr *a)
{
    a->delay_ms    = 0;
    a->bw_mbps     = 0.0;
    a->utilisation = 0;
}

/* ── public functions ────────────────────────────────────────────────── */

void network_init(Network *net, int num_routers)
{
    if (!net) {
        return;
    }
    for (int i = 0; i < num_routers; i++) {
        for (int j = 0; j < num_routers; j++) {
            net->cost[i][j] = 0;
            attr_clear(&net->attr[i][j]);
        }
    }
}

void network_init_defaults(Network *net)
{
    if (!net) {
        return;
    }
    /*
     * Default four-router topology preserved from the original project.
     *
     *   A—B : cost 2
     *   A—C : cost 5
     *   B—C : cost 1
     *   B—D : cost 4
     *   C—D : cost 1
     */
    network_init(net, MAX_ROUTERS);

    int links[][3] = {
        {0, 1, 2},
        {0, 2, 5},
        {1, 2, 1},
        {1, 3, 4},
        {2, 3, 1},
    };
    int n = (int)(sizeof links / sizeof links[0]);

    for (int k = 0; k < n; k++) {
        int u = links[k][0], v = links[k][1], c = links[k][2];
        network_add_link(net, MAX_ROUTERS, u, v, c,
                         DEFAULT_LINK_DELAY_MS, DEFAULT_LINK_BW_MBPS);
    }
}

bool network_add_link(Network *net, int num_routers,
                      int u, int v, int cost,
                      int delay_ms, double bw_mbps)
{
    if (u < 0 || u >= num_routers ||
        v < 0 || v >= num_routers ||
        u == v ||
        cost <= 0 ||
        delay_ms < 0 ||
        bw_mbps <= 0.0) {
        return false;
    }

    net->cost[u][v] = cost;
    net->cost[v][u] = cost;

    net->attr[u][v].delay_ms    = delay_ms;
    net->attr[u][v].bw_mbps     = bw_mbps;
    net->attr[u][v].utilisation = net->attr[u][v].utilisation; /* preserve */

    net->attr[v][u].delay_ms    = delay_ms;
    net->attr[v][u].bw_mbps     = bw_mbps;
    net->attr[v][u].utilisation = net->attr[v][u].utilisation;

    return true;
}

bool network_remove_link(Network *net, int num_routers, int u, int v)
{
    if (u < 0 || u >= num_routers ||
        v < 0 || v >= num_routers ||
        net->cost[u][v] == 0) {
        return false;
    }

    net->cost[u][v] = 0;
    net->cost[v][u] = 0;
    attr_clear(&net->attr[u][v]);
    attr_clear(&net->attr[v][u]);
    return true;
}

bool network_has_link(const Network *net, int u, int v)
{
    return (net->cost[u][v] > 0);
}

int network_get_cost(const Network *net, int u, int v)
{
    if (net->cost[u][v] > 0) {
        return net->cost[u][v];
    }
    return INF_COST;
}

void network_print_topology(const Network *net,
                             const Router routers[], int num_routers)
{
    printf("\n--- Network Topology (Links) ---\n");
    printf("%-20s <-> %-20s | Cost | Delay(ms) | BW(Mbps) | Utilisation\n",
           "Router A", "Router B");
    printf("%-20s     %-20s   %-4s   %-9s   %-8s   %s\n",
           "--------------------", "--------------------",
           "----", "---------", "--------", "-----------");

    bool any = false;
    for (int i = 0; i < num_routers; i++) {
        for (int j = i + 1; j < num_routers; j++) {
            if (net->cost[i][j] > 0) {
                printf("%-20s <-> %-20s | %-4d | %-9d | %-8.1f | %d\n",
                       routers[i].name, routers[j].name,
                       net->cost[i][j],
                       net->attr[i][j].delay_ms,
                       net->attr[i][j].bw_mbps,
                       net->attr[i][j].utilisation);
                any = true;
            }
        }
    }

    if (!any) {
        printf("  (no links defined)\n");
    }
}

void network_print_matrix(const Network *net,
                           const Router routers[], int num_routers)
{
    printf("\n--- Cost Matrix (0 = no link) ---\n");

    /* header row */
    printf("%-12s", "");
    for (int j = 0; j < num_routers; j++) {
        printf("%-12s", routers[j].name);
    }
    printf("\n");

    for (int i = 0; i < num_routers; i++) {
        printf("%-12s", routers[i].name);
        for (int j = 0; j < num_routers; j++) {
            if (net->cost[i][j] == 0) {
                printf("%-12s", "-");
            } else {
                printf("%-12d", net->cost[i][j]);
            }
        }
        printf("\n");
    }
}

void network_link_utilisation(const Network *net,
                               const Router routers[], int num_routers)
{
    printf("\n--- Link Utilisation ---\n");
    bool any = false;

    for (int i = 0; i < num_routers; i++) {
        for (int j = i + 1; j < num_routers; j++) {
            if (net->cost[i][j] > 0) {
                int util = net->attr[i][j].utilisation +
                           net->attr[j][i].utilisation;
                printf("  %s <-> %s : %d packets\n",
                       routers[i].name, routers[j].name, util);
                any = true;
            }
        }
    }

    if (!any) {
        printf("  (no links)\n");
    }
}

void network_reset_utilisation(Network *net, int num_routers)
{
    for (int i = 0; i < num_routers; i++) {
        for (int j = 0; j < num_routers; j++) {
            net->attr[i][j].utilisation = 0;
        }
    }
}

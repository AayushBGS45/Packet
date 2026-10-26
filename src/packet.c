/*
 * packet.c — Packet simulation implementation
 */

#include "packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── transmission-delay model ────────────────────────────────────────── */

double packet_compute_tx_delay(int size_bytes, double bw_mbps)
{
    if (bw_mbps <= 0.0) {
        return 0.0;
    }
    /* bits / (bits per second) = seconds; * 1000 → milliseconds */
    return ((double)size_bytes * 8.0) / (bw_mbps * 1.0e6) * 1000.0;
}

/* ── packet construction ─────────────────────────────────────────────── */

void packet_create(Packet *pkt, int id, int src, int dst, int size_bytes)
{
    memset(pkt, 0, sizeof *pkt);
    pkt->id         = id;
    pkt->src        = src;
    pkt->dst        = dst;
    pkt->size_bytes = size_bytes;
    pkt->status     = PKT_LOST_NO_ROUTE;   /* default until simulated */
}

/* ── simulation core ─────────────────────────────────────────────────── */

void packet_simulate_transmission(Packet *pkt,
                                  Network *net,
                                  Router routers[], int num_routers,
                                  RouteAlgorithm algo,
                                  int loss_pct)
{
    /* 1. Check source router */
    if (!router_is_valid(num_routers, pkt->src) ||
        !routers[pkt->src].active) {
        pkt->status = PKT_LOST_SRC_FAILED;
        return;
    }

    /* 2. Check destination router */
    if (!router_is_valid(num_routers, pkt->dst) ||
        !routers[pkt->dst].active) {
        pkt->status = PKT_LOST_DST_FAILED;
        return;
    }

    /* 3. Compute route */
    routing_compute(algo, net, routers, num_routers,
                    pkt->src, pkt->dst, &pkt->route);

    if (!pkt->route.reachable) {
        pkt->status = PKT_LOST_NO_ROUTE;
        return;
    }

    /*
     * 4. Find the bottleneck bandwidth along the computed path
     *    (minimum bandwidth of any link on the route).
     *    This determines transmission delay.
     */
    double bottleneck_bw = -1.0;
    for (int i = 0; i + 1 < pkt->route.hop_count; i++) {
        int u = pkt->route.path[i];
        int v = pkt->route.path[i + 1];
        double bw = net->attr[u][v].bw_mbps;
        if (bottleneck_bw < 0.0 || bw < bottleneck_bw) {
            bottleneck_bw = bw;
        }
    }
    if (bottleneck_bw <= 0.0) {
        bottleneck_bw = DEFAULT_LINK_BW_MBPS;
    }

    pkt->tx_delay_ms = packet_compute_tx_delay(pkt->size_bytes,
                                               bottleneck_bw);
    pkt->delay_ms    = (double)pkt->route.total_delay_ms + pkt->tx_delay_ms;

    /* 5. Apply probabilistic packet loss */
    if (loss_pct > 0 && (rand() % 100) < loss_pct) {
        pkt->status = PKT_LOST_RANDOM;
        return;
    }

    /* 6. Packet delivered — update utilisation counters */
    pkt->status = PKT_DELIVERED;

    for (int i = 0; i + 1 < pkt->route.hop_count; i++) {
        int u = pkt->route.path[i];
        int v = pkt->route.path[i + 1];
        net->attr[u][v].utilisation++;
        net->attr[v][u].utilisation++;
        routers[u].fwd_count++;
    }
    /* Destination router receives the packet */
    routers[pkt->dst].fwd_count++;
}

/* ── output ──────────────────────────────────────────────────────────── */

const char *packet_status_str(PacketStatus status)
{
    switch (status) {
        case PKT_DELIVERED:         return "DELIVERED";
        case PKT_LOST_RANDOM:       return "LOST (random link loss)";
        case PKT_LOST_NO_ROUTE:     return "LOST (no route)";
        case PKT_LOST_SRC_FAILED:   return "LOST (source router failed)";
        case PKT_LOST_DST_FAILED:   return "LOST (destination router failed)";
        case PKT_LOST_INTERMEDIATE: return "LOST (intermediate router failed)";
        default:                    return "UNKNOWN";
    }
}

void packet_print(const Packet *pkt,
                  const Router routers[],
                  bool verbose)
{
    printf("  Packet %-5d | %s -> %s | %d B | %s",
           pkt->id,
           routers[pkt->src].name,
           routers[pkt->dst].name,
           pkt->size_bytes,
           packet_status_str(pkt->status));

    if (pkt->status == PKT_DELIVERED) {
        printf(" | delay %.2f ms", pkt->delay_ms);
    }
    printf("\n");

    if (verbose && pkt->route.reachable) {
        printf("    Route: ");
        routing_print_path(&pkt->route, routers);
    }
}

/*
 * simulation.c — Batch simulation implementation
 */

#include "simulation.h"
#include "packet.h"
#include "input_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── defaults ────────────────────────────────────────────────────────── */

void sim_config_defaults(SimConfig *cfg)
{
    cfg->algo           = ALGO_DIJKSTRA;
    cfg->loss_pct       = DEFAULT_LOSS_PCT;
    cfg->default_pkt_sz = DEFAULT_PACKET_SIZE;
    cfg->use_fixed_seed = false;
    cfg->fixed_seed     = 42;
}

/* ── single-packet interactive send ─────────────────────────────────── */

bool sim_send_single(Network *net, Router routers[], int num_routers,
                     const SimConfig *cfg, SimStats *stats)
{
    router_print_all(routers, num_routers);

    int src, dst, sz;

    if (!input_read_router_id("Enter source router ID", num_routers, &src)) {
        return false;
    }
    if (!input_read_router_id("Enter destination router ID", num_routers, &dst)) {
        return false;
    }
    if (src == dst) {
        printf("  Source and destination must be different.\n");
        return false;
    }
    if (!input_read_int_range("Enter packet size in bytes",
                              MIN_PACKET_SIZE, MAX_PACKET_SIZE, &sz)) {
        sz = cfg->default_pkt_sz;
        printf("  Using default size: %d bytes\n", sz);
    }

    if (cfg->use_fixed_seed) {
        srand(cfg->fixed_seed);
    }

    Packet pkt;
    packet_create(&pkt, stats->packets_attempted + 1, src, dst, sz);
    packet_simulate_transmission(&pkt, net, routers, num_routers,
                                 cfg->algo, cfg->loss_pct);

    printf("\n--- Packet Result ---\n");
    packet_print(&pkt, routers, /*verbose=*/true);

    stats_record(stats, &pkt);
    return true;
}

/* ── batch simulation ────────────────────────────────────────────────── */

void sim_run_batch(Network *net, Router routers[], int num_routers,
                   const SimConfig *cfg, SimStats *stats,
                   int count, int src, int dst,
                   int pkt_size, bool verbose, FILE *log_fp)
{
    if (cfg->use_fixed_seed) {
        srand(cfg->fixed_seed);
    }

    /* Compute route once upfront to display it */
    PathResult route;
    routing_compute(cfg->algo, net, routers, num_routers, src, dst, &route);
    if (!route.reachable) {
        printf("  No route from %s to %s with current topology.\n",
               routers[src].name, routers[dst].name);
    } else {
        printf("  Route: ");
        routing_print_path(&route, routers);
    }

    printf("  Simulating %d packet(s)...\n\n", count);

    int base_id = stats->packets_attempted + 1;

    for (int i = 0; i < count; i++) {
        Packet pkt;
        packet_create(&pkt, base_id + i, src, dst, pkt_size);
        packet_simulate_transmission(&pkt, net, routers, num_routers,
                                     cfg->algo, cfg->loss_pct);
        stats_record(stats, &pkt);

        if (verbose) {
            packet_print(&pkt, routers, /*verbose=*/false);
        }

        if (log_fp) {
            stats_export_packet_log(log_fp, &pkt, routers);
        }
    }

    stats->run_count++;
}

/* ── interactive batch ───────────────────────────────────────────────── */

void sim_run_interactive_batch(Network *net, Router routers[],
                               int num_routers,
                               const SimConfig *cfg, SimStats *stats)
{
    router_print_all(routers, num_routers);

    int count, src, dst, sz;

    if (!input_read_int_range("Number of packets to simulate",
                              1, MAX_BATCH_PACKETS, &count)) {
        return;
    }
    if (!input_read_router_id("Enter source router ID", num_routers, &src)) {
        return;
    }
    if (!input_read_router_id("Enter destination router ID", num_routers, &dst)) {
        return;
    }
    if (src == dst) {
        printf("  Source and destination must be different.\n");
        return;
    }

    printf("Enter packet size in bytes [default %d]: ", cfg->default_pkt_sz);
    fflush(stdout);
    if (!input_read_int_range("", MIN_PACKET_SIZE, MAX_PACKET_SIZE, &sz)) {
        sz = cfg->default_pkt_sz;
        printf("  Using default size: %d bytes\n", sz);
    }

    bool verbose = (count <= 50);   /* print each packet if ≤ 50 */
    if (!verbose) {
        printf("  (sending %d packets; individual results suppressed)\n",
               count);
    }

    sim_run_batch(net, routers, num_routers, cfg, stats,
                  count, src, dst, sz, verbose, NULL);

    printf("\n--- Batch Summary ---\n");
    printf("  Packets sent : %d\n", count);
}

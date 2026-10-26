/*
 * simulation.h — Batch simulation and configuration interface
 *
 * SimConfig holds all tunable parameters for a simulation run.
 * A "run" may be:
 *   - a single packet (called from the menu)
 *   - a batch of N packets with the same source/destination
 *   - a stress test with random src/dst pairs
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include "routing.h"
#include "statistics.h"
#include "network.h"
#include "router.h"

/* ── constants ───────────────────────────────────────────────────────── */

#define MAX_BATCH_PACKETS 10000
#define DEFAULT_LOSS_PCT  30
#define DEFAULT_BATCH_SZ  10

/* ── configuration ───────────────────────────────────────────────────── */

/**
 * SimConfig — settings that control simulation behaviour.
 *
 * Fields
 * ------
 * algo            : routing algorithm (Dijkstra or BFS).
 * loss_pct        : packet-loss probability [0, 100].
 * default_pkt_sz  : default packet size in bytes.
 * use_fixed_seed  : if true, srand(fixed_seed) is called before each run.
 * fixed_seed      : seed value for reproducible experiments.
 */
typedef struct {
    RouteAlgorithm algo;
    int            loss_pct;
    int            default_pkt_sz;
    bool           use_fixed_seed;
    unsigned int   fixed_seed;
} SimConfig;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * sim_config_defaults — fill config with sensible defaults.
 */
void sim_config_defaults(SimConfig *cfg);

/**
 * sim_send_single — send one packet interactively.
 *
 * Prompts the user for source, destination, and size.
 * Routes and prints the result, then updates stats.
 *
 * @return true if the packet was submitted (even if lost).
 */
bool sim_send_single(Network *net, Router routers[], int num_routers,
                     const SimConfig *cfg, SimStats *stats);

/**
 * sim_run_batch — simulate count packets from src to dst.
 *
 * @param count      Number of packets to send.
 * @param src, dst   Router indices.
 * @param pkt_size   Size in bytes for each packet.
 * @param verbose    If true, print each packet result; else just summary.
 * @param log_fp     If non-NULL, write per-packet CSV rows here.
 */
void sim_run_batch(Network *net, Router routers[], int num_routers,
                   const SimConfig *cfg, SimStats *stats,
                   int count, int src, int dst,
                   int pkt_size, bool verbose, FILE *log_fp);

/**
 * sim_run_interactive_batch — prompt user for batch parameters, then call
 * sim_run_batch.
 */
void sim_run_interactive_batch(Network *net, Router routers[],
                               int num_routers,
                               const SimConfig *cfg, SimStats *stats);

#endif /* SIMULATION_H */

/*
 * statistics.h — Simulation metrics and result reporting
 *
 * Statistics are accumulated across all simulated packets in a session.
 * Calling stats_reset() clears all counters for a fresh run.
 *
 * Formulas
 * --------
 *   delivery_ratio   = packets_delivered / packets_attempted
 *   loss_pct         = packets_lost / packets_attempted * 100
 *   avg_cost         = total_routing_cost / packets_routed
 *   avg_delay_ms     = total_delay_ms / packets_delivered
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdio.h>
#include "packet.h"
#include <stdbool.h>

/* ── data types ──────────────────────────────────────────────────────── */

/**
 * SimStats — cumulative simulation metrics.
 */
typedef struct {
    /* packet counts */
    int packets_attempted;
    int packets_delivered;
    int packets_lost_random;
    int packets_lost_no_route;
    int packets_lost_src_failed;
    int packets_lost_dst_failed;
    int packets_routed;          /* attempted that had a valid route */

    /* cost / delay */
    long long total_routing_cost;   /* sum of route costs (delivered) */
    double    total_delay_ms;       /* sum of delays (delivered packets) */
    double    min_delay_ms;
    double    max_delay_ms;

    /* run bookkeeping */
    int run_count;               /* how many batch/single runs completed */
} SimStats;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * stats_reset — clear all counters.
 */
void stats_reset(SimStats *s);

/**
 * stats_record — update metrics from a single simulated packet.
 */
void stats_record(SimStats *s, const Packet *pkt);

/**
 * stats_print — display a formatted summary to stdout.
 */
void stats_print(const SimStats *s);

/**
 * stats_export_csv — write simulation results to a CSV file.
 *
 * @param s         Statistics to export.
 * @param filename  Output file path.
 * @return          true on success.
 */
bool stats_export_csv(const SimStats *s, const char *filename);

/**
 * stats_export_packet_log — append a packet record to an open CSV file.
 *
 * Call once per packet during a batch simulation; the caller manages
 * opening/closing the file.
 *
 * @param fp      Open file pointer (must be writable).
 * @param pkt     Packet to log.
 * @param routers For name lookup.
 */
void stats_export_packet_log(FILE *fp, const Packet *pkt,
                              const Router routers[]);

#endif /* STATISTICS_H */

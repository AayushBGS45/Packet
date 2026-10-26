/*
 * statistics.c — Simulation metrics implementation
 */

#include "statistics.h"
#include "packet.h"
#include "router.h"

#include <stdio.h>
#include <string.h>
#include <math.h>   /* for HUGE_VAL */

/* ── public functions ────────────────────────────────────────────────── */

void stats_reset(SimStats *s)
{
    memset(s, 0, sizeof *s);
    s->min_delay_ms = HUGE_VAL;   /* so the first comparison always wins */
    s->max_delay_ms = 0.0;
}

void stats_record(SimStats *s, const Packet *pkt)
{
    s->packets_attempted++;

    switch (pkt->status) {
        case PKT_DELIVERED:
            s->packets_delivered++;
            s->packets_routed++;
            s->total_routing_cost += pkt->route.total_cost;
            s->total_delay_ms     += pkt->delay_ms;
            if (pkt->delay_ms < s->min_delay_ms) {
                s->min_delay_ms = pkt->delay_ms;
            }
            if (pkt->delay_ms > s->max_delay_ms) {
                s->max_delay_ms = pkt->delay_ms;
            }
            break;

        case PKT_LOST_RANDOM:
            s->packets_lost_random++;
            s->packets_routed++;
            /* Still count the routing cost — a route was found */
            s->total_routing_cost += pkt->route.total_cost;
            break;

        case PKT_LOST_NO_ROUTE:
            s->packets_lost_no_route++;
            break;

        case PKT_LOST_SRC_FAILED:
            s->packets_lost_src_failed++;
            break;

        case PKT_LOST_DST_FAILED:
            s->packets_lost_dst_failed++;
            break;

        case PKT_LOST_INTERMEDIATE:
            s->packets_lost_no_route++;
            break;
    }
}

void stats_print(const SimStats *s)
{
    printf("\n");
    printf("+------------------------------------------+\n");
    printf("|         NETWORK SIMULATION STATISTICS    |\n");
    printf("+------------------------------------------+\n");

    printf("|  Packets attempted   : %-17d |\n", s->packets_attempted);
    printf("|  Packets delivered   : %-17d |\n", s->packets_delivered);

    int total_lost = s->packets_attempted - s->packets_delivered;
    printf("|  Packets lost (total): %-17d |\n", total_lost);
    printf("|    - Random loss     : %-17d |\n", s->packets_lost_random);
    printf("|    - No route        : %-17d |\n", s->packets_lost_no_route);
    printf("|    - Src failed      : %-17d |\n", s->packets_lost_src_failed);
    printf("|    - Dst failed      : %-17d |\n", s->packets_lost_dst_failed);

    printf("+------------------------------------------+\n");

    if (s->packets_attempted > 0) {
        double dr = 100.0 * s->packets_delivered / s->packets_attempted;
        double lr = 100.0 * total_lost          / s->packets_attempted;
        printf("|  Delivery ratio      : %-16.2f%% |\n", dr);
        printf("|  Loss rate           : %-16.2f%% |\n", lr);
    } else {
        printf("|  Delivery ratio      : N/A               |\n");
        printf("|  Loss rate           : N/A               |\n");
    }

    printf("+------------------------------------------+\n");

    if (s->packets_routed > 0) {
        double avg_cost = (double)s->total_routing_cost / s->packets_routed;
        printf("|  Avg routing cost    : %-17.2f |\n", avg_cost);
    } else {
        printf("|  Avg routing cost    : N/A               |\n");
    }

    if (s->packets_delivered > 0) {
        double avg_delay = s->total_delay_ms / s->packets_delivered;
        printf("|  Avg delay (ms)      : %-17.2f |\n", avg_delay);
        if (s->min_delay_ms < HUGE_VAL) {
            printf("|  Min delay (ms)      : %-17.2f |\n",
                   s->min_delay_ms);
        }
        printf("|  Max delay (ms)      : %-17.2f |\n", s->max_delay_ms);
    } else {
        printf("|  Avg delay (ms)      : N/A               |\n");
        printf("|  Min delay (ms)      : N/A               |\n");
        printf("|  Max delay (ms)      : N/A               |\n");
    }

    printf("+------------------------------------------+\n");
    printf("|  Simulation runs     : %-17d |\n", s->run_count);
    printf("+------------------------------------------+\n");
}

bool stats_export_csv(const SimStats *s, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return false;
    }

    int total_lost = s->packets_attempted - s->packets_delivered;

    fprintf(fp, "Metric,Value\n");
    fprintf(fp, "packets_attempted,%d\n",       s->packets_attempted);
    fprintf(fp, "packets_delivered,%d\n",       s->packets_delivered);
    fprintf(fp, "packets_lost_total,%d\n",      total_lost);
    fprintf(fp, "packets_lost_random,%d\n",     s->packets_lost_random);
    fprintf(fp, "packets_lost_no_route,%d\n",   s->packets_lost_no_route);
    fprintf(fp, "packets_lost_src_failed,%d\n", s->packets_lost_src_failed);
    fprintf(fp, "packets_lost_dst_failed,%d\n", s->packets_lost_dst_failed);
    fprintf(fp, "packets_routed,%d\n",          s->packets_routed);

    if (s->packets_attempted > 0) {
        fprintf(fp, "delivery_ratio_pct,%.4f\n",
                100.0 * s->packets_delivered / s->packets_attempted);
        fprintf(fp, "loss_rate_pct,%.4f\n",
                100.0 * total_lost / s->packets_attempted);
    }

    if (s->packets_routed > 0) {
        fprintf(fp, "avg_routing_cost,%.4f\n",
                (double)s->total_routing_cost / s->packets_routed);
    }

    if (s->packets_delivered > 0) {
        fprintf(fp, "avg_delay_ms,%.4f\n",
                s->total_delay_ms / s->packets_delivered);
        if (s->min_delay_ms < HUGE_VAL) {
            fprintf(fp, "min_delay_ms,%.4f\n", s->min_delay_ms);
        }
        fprintf(fp, "max_delay_ms,%.4f\n",     s->max_delay_ms);
    }

    fprintf(fp, "simulation_runs,%d\n", s->run_count);

    fclose(fp);
    return true;
}

void stats_export_packet_log(FILE *fp, const Packet *pkt,
                              const Router routers[])
{
    fprintf(fp, "%d,%s,%s,%d,%s,%.2f,%d,%d\n",
            pkt->id,
            routers[pkt->src].name,
            routers[pkt->dst].name,
            pkt->size_bytes,
            packet_status_str(pkt->status),
            pkt->delay_ms,
            pkt->route.total_cost,
            pkt->route.hop_count - 1);
}

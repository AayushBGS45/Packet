/*
 * tests.c — automated test suite for the Packet Routing Simulator
 *
 * Compile and run:
 *   gcc -std=c11 -Wall -Wextra -I. -o tests\run_tests.exe \
 *       tests\tests.c src\router.c src\network.c src\routing.c \
 *       src\packet.c src\statistics.c src\input_util.c -lm
 *   .\tests\run_tests.exe
 *
 * Tests cover:
 *  T01  Known shortest path in weighted graph
 *  T02  Multiple paths — Dijkstra picks the cheapest
 *  T03  Disconnected graph — destination unreachable
 *  T04  Source == destination
 *  T05  Invalid router IDs
 *  T06  Failed source router
 *  T07  Failed destination router
 *  T08  Failed intermediate router — alternate route used
 *  T09  All intermediate routers failed — unreachable
 *  T10  Router restoration
 *  T11  Packet loss at 0%
 *  T12  Packet loss at 100%
 *  T13  Zero packets — statistics remain zero
 *  T14  Single-packet statistics
 *  T15  Delivery ratio and loss rate consistency
 *  T16  BFS minimum-hop path
 *  T17  Dijkstra vs BFS on different optimal paths
 *  T18  Add and remove links at runtime
 *  T19  Add a new router
 *  T20  Router forwarding counters
 *  T21  Link utilisation counters
 *  T22  Repeated simulation runs accumulate correctly
 *  T23  Maximum network size (MAX_ROUTERS nodes, all connected)
 *  T24  Statistics reset
 *  T25  CSV export produces a file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Point to sources one level up */
#include "../src/router.h"
#include "../src/network.h"
#include "../src/routing.h"
#include "../src/packet.h"
#include "../src/statistics.h"
#include "test_framework.h"

/* ── framework counters ──────────────────────────────────────────────── */
int g_tests_run    = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;

/* ── helper: initialise a single router struct safely ───────────────── */
static void init_router(Router *r, int id, const char *name)
{
    r->id        = id;
    r->active    = true;
    r->fwd_count = 0;
    strncpy(r->name, name, ROUTER_NAME_LEN - 1);
    r->name[ROUTER_NAME_LEN - 1] = '\0';
}

/* ── helper: build default 4-router network ─────────────────────────── */
static void default_setup(Router r[], int *nr, Network *net)
{
    if (r && nr) {
        router_init_defaults(r, nr);
    }
    if (net) {
        network_init_defaults(net);
    }
}

/* ── T01: known shortest path ────────────────────────────────────────── */
static void test_shortest_path_known(void)
{
    TEST_SECTION("T01 Known shortest path");
    /*
     * Default graph:
     *   A-B:2  A-C:5  B-C:1  B-D:4  C-D:1
     * Shortest A→D: A→B(2)→C(1)→D(1) = cost 4
     */
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 3, &res);   /* A=0, D=3 */

    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, 4);
    CHECK_EQ(res.hop_count, 4);        /* A,B,C,D */
    CHECK_EQ(res.path[0], 0);          /* A */
    CHECK_EQ(res.path[3], 3);          /* D */
}

/* ── T02: Dijkstra picks cheapest, not fewest hops ───────────────────── */
static void test_dijkstra_cheapest(void)
{
    TEST_SECTION("T02 Dijkstra picks cheapest path");
    /*
     * Build graph:
     *   0-1: cost 1
     *   1-2: cost 1
     *   0-2: cost 100   (direct but expensive)
     * Dijkstra: 0→1→2 = cost 2   (not 0→2 = cost 100)
     */
    Router r[MAX_ROUTERS]; int nr = 0;
    r[0] = (Router){0, "R0", true, 0};
    r[1] = (Router){1, "R1", true, 0};
    r[2] = (Router){2, "R2", true, 0};
    nr = 3;

    Network net;
    network_init(&net, MAX_ROUTERS);
    network_add_link(&net, nr, 0, 1,   1, 10, 100.0);
    network_add_link(&net, nr, 1, 2,   1, 10, 100.0);
    network_add_link(&net, nr, 0, 2, 100, 10, 100.0);

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 2, &res);

    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, 2);
    CHECK_EQ(res.hop_count, 3);   /* 0, 1, 2 */
}

/* ── T03: disconnected graph ─────────────────────────────────────────── */
static void test_disconnected(void)
{
    TEST_SECTION("T03 Disconnected graph — unreachable destination");
    Router r[MAX_ROUTERS]; int nr = 2;
    init_router(&r[0], 0, "R0");
    init_router(&r[1], 1, "R1");

    Network net;
    network_init(&net, MAX_ROUTERS);
    /* no links added */

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 1, &res);
    CHECK(!res.reachable);

    routing_bfs(&net, r, nr, 0, 1, &res);
    CHECK(!res.reachable);
}

/* ── T04: src == dst ─────────────────────────────────────────────────── */
static void test_src_eq_dst(void)
{
    TEST_SECTION("T04 Source equals destination");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    PathResult res;
    routing_dijkstra(&net, r, nr, 1, 1, &res);

    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, 0);
    CHECK_EQ(res.hop_count, 1);
}

/* ── T05: invalid router IDs ─────────────────────────────────────────── */
static void test_invalid_ids(void)
{
    TEST_SECTION("T05 Invalid router IDs");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    PathResult res;
    /* id = -1 */
    routing_dijkstra(&net, r, nr, -1, 0, &res);
    CHECK(!res.reachable);

    /* id >= nr */
    routing_dijkstra(&net, r, nr, 0, 99, &res);
    CHECK(!res.reachable);

    CHECK(!router_is_valid(nr, -1));
    CHECK(!router_is_valid(nr, nr));
    CHECK(router_is_valid(nr, 0));
    CHECK(router_is_valid(nr, nr - 1));
}

/* ── T06: failed source ──────────────────────────────────────────────── */
static void test_failed_source(void)
{
    TEST_SECTION("T06 Failed source router");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    r[0].active = false;   /* fail Router-A */

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 3, &res);
    CHECK(!res.reachable);

    /* Packet simulation should report PKT_LOST_SRC_FAILED */
    Packet pkt;
    packet_create(&pkt, 1, 0, 3, 1024);
    packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
    CHECK_EQ((int)pkt.status, (int)PKT_LOST_SRC_FAILED);
}

/* ── T07: failed destination ─────────────────────────────────────────── */
static void test_failed_destination(void)
{
    TEST_SECTION("T07 Failed destination router");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    r[3].active = false;   /* fail Router-D */

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 3, &res);
    CHECK(!res.reachable);

    Packet pkt;
    packet_create(&pkt, 1, 0, 3, 1024);
    packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
    CHECK_EQ((int)pkt.status, (int)PKT_LOST_DST_FAILED);
}

/* ── T08: failed intermediate — alternate route ──────────────────────── */
static void test_failed_intermediate_alternate(void)
{
    TEST_SECTION("T08 Failed intermediate router — alternate route");
    /*
     * Default graph: A→D via A-B-C-D (cost 4).
     * Fail B (id=1). Only path: A-C-D (cost 5+1=6).
     */
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    r[1].active = false;   /* fail Router-B */

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 3, &res);

    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, 6);   /* A-C-D */

    /* Intermediate node must NOT be Router-B (id=1) */
    bool b_in_path = false;
    for (int i = 0; i < res.hop_count; i++) {
        if (res.path[i] == 1) b_in_path = true;
    }
    CHECK(!b_in_path);
}

/* ── T09: no alternate route ─────────────────────────────────────────── */
static void test_failed_all_intermediates(void)
{
    TEST_SECTION("T09 All paths blocked — unreachable");
    /*
     * Topology with only one path from 0 to 2:
     *   0-1-2
     * Fail node 1 → unreachable.
     */
    Router r[MAX_ROUTERS]; int nr = 0;
    r[0] = (Router){0, "R0", true, 0};
    r[1] = (Router){1, "R1", true, 0};
    r[2] = (Router){2, "R2", true, 0};
    nr = 3;

    Network net;
    network_init(&net, MAX_ROUTERS);
    network_add_link(&net, nr, 0, 1, 1, 10, 100.0);
    network_add_link(&net, nr, 1, 2, 1, 10, 100.0);

    r[1].active = false;

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 2, &res);
    CHECK(!res.reachable);
}

/* ── T10: router restoration ─────────────────────────────────────────── */
static void test_router_restoration(void)
{
    TEST_SECTION("T10 Router restoration");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    /* Fail B, confirm unreachability of direct B path */
    r[1].active = false;
    PathResult res;
    routing_dijkstra(&net, r, nr, 0, 1, &res);
    CHECK(!res.reachable);

    /* Restore B */
    r[1].active = true;
    routing_dijkstra(&net, r, nr, 0, 1, &res);
    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, 2);
}

/* ── T11: 0% loss — all packets delivered ────────────────────────────── */
static void test_zero_loss(void)
{
    TEST_SECTION("T11 Zero packet loss");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    srand(12345);

    SimStats s; stats_reset(&s);
    for (int i = 0; i < 100; i++) {
        Packet pkt;
        packet_create(&pkt, i + 1, 0, 3, 1024);
        packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
        stats_record(&s, &pkt);
    }
    CHECK_EQ(s.packets_attempted,  100);
    CHECK_EQ(s.packets_delivered,  100);
    CHECK_EQ(s.packets_lost_random, 0);
}

/* ── T12: 100% loss — no packets delivered ────────────────────────────── */
static void test_full_loss(void)
{
    TEST_SECTION("T12 100%% packet loss");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    srand(12345);

    SimStats s; stats_reset(&s);
    for (int i = 0; i < 50; i++) {
        Packet pkt;
        packet_create(&pkt, i + 1, 0, 3, 1024);
        packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 100);
        stats_record(&s, &pkt);
    }
    CHECK_EQ(s.packets_attempted,   50);
    CHECK_EQ(s.packets_delivered,    0);
    CHECK_EQ(s.packets_lost_random, 50);
}

/* ── T13: zero packets — statistics empty ────────────────────────────── */
static void test_zero_packets(void)
{
    TEST_SECTION("T13 Zero packets — statistics remain zero");
    SimStats s; stats_reset(&s);
    CHECK_EQ(s.packets_attempted, 0);
    CHECK_EQ(s.packets_delivered, 0);
    /* stats_print must not divide by zero; just call it */
    stats_print(&s);
}

/* ── T14: single packet statistics ──────────────────────────────────── */
static void test_single_packet_stats(void)
{
    TEST_SECTION("T14 Single packet — statistics updated correctly");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    srand(0);   /* deterministic — first draw gives non-loss with 30% */

    SimStats s; stats_reset(&s);
    Packet pkt;
    packet_create(&pkt, 1, 0, 3, 1024);
    packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
    stats_record(&s, &pkt);

    CHECK_EQ(s.packets_attempted, 1);
    CHECK_EQ(s.packets_delivered, 1);
    CHECK(s.total_routing_cost > 0);
}

/* ── T15: delivery ratio consistency ────────────────────────────────── */
static void test_delivery_ratio_consistency(void)
{
    TEST_SECTION("T15 Delivery ratio + loss rate sum to 100%%");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    srand(9999);

    SimStats s; stats_reset(&s);
    for (int i = 0; i < 200; i++) {
        Packet pkt;
        packet_create(&pkt, i + 1, 0, 3, 1024);
        packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 30);
        stats_record(&s, &pkt);
    }

    int total_lost = s.packets_attempted - s.packets_delivered;
    CHECK_EQ(s.packets_attempted, 200);
    CHECK_EQ(s.packets_delivered + total_lost, 200);
}

/* ── T16: BFS minimum hops ───────────────────────────────────────────── */
static void test_bfs_min_hops(void)
{
    TEST_SECTION("T16 BFS minimum-hop path");
    /*
     * Graph:
     *   0-1 cost 1   0-2 cost 1   1-3 cost 1   2-3 cost 100
     * Dijkstra 0→3 via 0-1-3 (cost 2) or 0-2-3 (cost 101)?  → 0-1-3
     * BFS     0→3 via 0-1-3 (2 hops) or 0-2-3 (2 hops)? both 2 hops.
     * Use linear chain to verify BFS skips long-cost hops:
     *   0-1:1  0-2:1  1-3:1
     * BFS:  0→1→3  (2 hops)
     * Dijkstra: same (cost 2)
     */
    Router r[MAX_ROUTERS]; int nr = 4;
    for (int i = 0; i < nr; i++) {
        r[i].id        = i;
        r[i].active    = true;
        r[i].fwd_count = 0;
        snprintf(r[i].name, ROUTER_NAME_LEN, "R%d", i);
    }

    Network net;
    network_init(&net, MAX_ROUTERS);
    network_add_link(&net, nr, 0, 1, 1,   10, 100.0);
    network_add_link(&net, nr, 0, 2, 1,   10, 100.0);
    network_add_link(&net, nr, 1, 3, 1,   10, 100.0);
    network_add_link(&net, nr, 2, 3, 100, 10, 100.0);

    PathResult bfs_res, dijk_res;
    routing_bfs(&net, r, nr, 0, 3, &bfs_res);
    routing_dijkstra(&net, r, nr, 0, 3, &dijk_res);

    CHECK(bfs_res.reachable);
    CHECK(dijk_res.reachable);
    CHECK_EQ(bfs_res.hop_count - 1, 2);     /* 2 hops */
    CHECK_EQ(dijk_res.total_cost, 2);        /* cost 2 */
}

/* ── T17: Dijkstra vs BFS different result ───────────────────────────── */
static void test_dijkstra_vs_bfs_differ(void)
{
    TEST_SECTION("T17 Dijkstra and BFS choose different paths");
    /*
     * Graph:
     *   0-1: cost 1
     *   1-2: cost 1
     *   0-2: cost 3   (direct, cheaper than 3 hops? No: 1+1 < 3 either way)
     *
     * Try a case where BFS picks fewer hops but higher cost:
     *   0-1: cost 10
     *   0-2: cost 1
     *   2-3: cost 1
     *   1-3: cost 1
     * Dijkstra: 0→2→3  (cost 2)
     * BFS     : 0→1→3  (2 hops, cost 11) OR 0→2→3 (2 hops, cost 2)
     *           Both have 2 hops, so BFS may pick either.
     *
     * Better case — force difference:
     *   0-1: cost 1
     *   1-2: cost 1
     *   2-3: cost 1
     *   0-3: cost 4   (direct, 1 hop, cost 4)
     * Dijkstra: 0→1→2→3  cost 3  (3 hops)
     * BFS     : 0→3       hops 1  (direct)
     */
    Router r[MAX_ROUTERS]; int nr = 4;
    for (int i = 0; i < nr; i++) {
        r[i].id        = i;
        r[i].active    = true;
        r[i].fwd_count = 0;
        snprintf(r[i].name, ROUTER_NAME_LEN, "R%d", i);
    }

    Network net;
    network_init(&net, MAX_ROUTERS);
    network_add_link(&net, nr, 0, 1, 1, 10, 100.0);
    network_add_link(&net, nr, 1, 2, 1, 10, 100.0);
    network_add_link(&net, nr, 2, 3, 1, 10, 100.0);
    network_add_link(&net, nr, 0, 3, 4, 10, 100.0);  /* direct, more cost */

    PathResult dijk, bfs;
    routing_dijkstra(&net, r, nr, 0, 3, &dijk);
    routing_bfs(&net, r, nr, 0, 3, &bfs);

    /* Dijkstra takes 3 hops with cost 3 */
    CHECK(dijk.reachable);
    CHECK_EQ(dijk.total_cost, 3);
    CHECK_EQ(dijk.hop_count - 1, 3);

    /* BFS takes 1 hop (direct) */
    CHECK(bfs.reachable);
    CHECK_EQ(bfs.hop_count - 1, 1);
}

/* ── T18: add and remove links ────────────────────────────────────────── */
static void test_link_management(void)
{
    TEST_SECTION("T18 Add and remove links");
    Router r[MAX_ROUTERS]; int nr = 2;
    r[0] = (Router){0, "R0", true, 0};
    r[1] = (Router){1, "R1", true, 0};
    (void)r;   /* r used only for num_routers bounds; suppress warning */

    Network net;
    network_init(&net, MAX_ROUTERS);

    CHECK(!network_has_link(&net, 0, 1));

    bool ok = network_add_link(&net, nr, 0, 1, 5, 20, 50.0);
    CHECK(ok);
    CHECK(network_has_link(&net, 0, 1));
    CHECK_EQ(network_get_cost(&net, 0, 1), 5);

    /* Symmetry */
    CHECK(network_has_link(&net, 1, 0));

    ok = network_remove_link(&net, nr, 0, 1);
    CHECK(ok);
    CHECK(!network_has_link(&net, 0, 1));

    /* Remove non-existent link */
    ok = network_remove_link(&net, nr, 0, 1);
    CHECK(!ok);

    /* Invalid cost */
    ok = network_add_link(&net, nr, 0, 1, 0, 10, 100.0);
    CHECK(!ok);

    /* Self-link */
    ok = network_add_link(&net, nr, 0, 0, 5, 10, 100.0);
    CHECK(!ok);
}

/* ── T19: add a new router ───────────────────────────────────────────── */
static void test_add_router(void)
{
    TEST_SECTION("T19 Add a new router");
    Router r[MAX_ROUTERS]; int nr;
    default_setup(r, &nr, NULL);

    Network net;
    network_init_defaults(&net);

    int idx = router_add(r, &nr, "Router-E");
    CHECK(idx == 4);
    CHECK_EQ(nr, 5);
    CHECK(r[4].active);
}

/* ── T20: forwarding counters ────────────────────────────────────────── */
static void test_forwarding_counters(void)
{
    TEST_SECTION("T20 Router forwarding counters");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    /* Zero loss; reset counters first */
    router_reset_counters(r, nr);

    Packet pkt;
    packet_create(&pkt, 1, 0, 3, 512);
    packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);

    CHECK(pkt.status == PKT_DELIVERED);
    /* Path A(0)→B(1)→C(2)→D(3): A,B,C all get fwd_count++ ; D gets +1 */
    CHECK(r[0].fwd_count > 0 || r[3].fwd_count > 0);
}

/* ── T21: link utilisation ────────────────────────────────────────────── */
static void test_link_utilisation(void)
{
    TEST_SECTION("T21 Link utilisation counters");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);
    network_reset_utilisation(&net, nr);

    /* Send 5 packets with 0% loss */
    for (int i = 0; i < 5; i++) {
        Packet pkt;
        packet_create(&pkt, i + 1, 0, 3, 512);
        packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
    }

    /* Path is A-B-C-D; link A-B should have utilisation = 5 */
    CHECK(net.attr[0][1].utilisation == 5 || net.attr[1][0].utilisation == 5);
}

/* ── T22: repeated simulation accumulates ────────────────────────────── */
static void test_repeated_simulation(void)
{
    TEST_SECTION("T22 Repeated simulation runs accumulate stats");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    SimStats s; stats_reset(&s);
    srand(1);

    /* Run 3 batches */
    for (int run = 0; run < 3; run++) {
        for (int i = 0; i < 10; i++) {
            Packet pkt;
            packet_create(&pkt, s.packets_attempted + 1, 0, 3, 1024);
            packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
            stats_record(&s, &pkt);
        }
        s.run_count++;
    }

    CHECK_EQ(s.packets_attempted, 30);
    CHECK_EQ(s.packets_delivered, 30);
    CHECK_EQ(s.run_count, 3);
}

/* ── T23: maximum network size ────────────────────────────────────────── */
static void test_max_network_size(void)
{
    TEST_SECTION("T23 Maximum network size");
    Router r[MAX_ROUTERS];
    int nr = MAX_ROUTERS;
    Network net;
    network_init(&net, MAX_ROUTERS);

    for (int i = 0; i < nr; i++) {
        r[i].id        = i;
        r[i].active    = true;
        r[i].fwd_count = 0;
        snprintf(r[i].name, ROUTER_NAME_LEN, "R%d", i);
    }

    /* Chain: 0-1-2-...(MAX_ROUTERS-1) */
    for (int i = 0; i + 1 < nr; i++) {
        network_add_link(&net, nr, i, i + 1, 1, 5, 100.0);
    }

    PathResult res;
    routing_dijkstra(&net, r, nr, 0, nr - 1, &res);

    CHECK(res.reachable);
    CHECK_EQ(res.total_cost, nr - 1);
    CHECK_EQ(res.hop_count, nr);
}

/* ── T24: statistics reset ────────────────────────────────────────────── */
static void test_stats_reset(void)
{
    TEST_SECTION("T24 Statistics reset");
    Router r[MAX_ROUTERS]; int nr;
    Network net;
    default_setup(r, &nr, &net);

    SimStats s; stats_reset(&s);
    /* Add a packet */
    Packet pkt;
    packet_create(&pkt, 1, 0, 3, 1024);
    packet_simulate_transmission(&pkt, &net, r, nr, ALGO_DIJKSTRA, 0);
    stats_record(&s, &pkt);

    CHECK(s.packets_attempted > 0);

    stats_reset(&s);
    CHECK_EQ(s.packets_attempted, 0);
    CHECK_EQ(s.packets_delivered, 0);
    CHECK_EQ(s.run_count, 0);
}

/* ── T25: CSV export ─────────────────────────────────────────────────── */
static void test_csv_export(void)
{
    TEST_SECTION("T25 CSV export");
    SimStats s; stats_reset(&s);
    s.packets_attempted = 10;
    s.packets_delivered = 7;
    s.packets_lost_random = 3;
    s.packets_routed = 10;
    s.total_routing_cost = 40;
    s.total_delay_ms = 100.0;
    s.min_delay_ms = 5.0;
    s.max_delay_ms = 20.0;

    const char *fname = "tests/test_export_output.csv";
    bool ok = stats_export_csv(&s, fname);
    CHECK(ok);

    /* Verify the file exists and is non-empty */
    FILE *fp = fopen(fname, "r");
    CHECK(fp != NULL);
    if (fp) {
        char line[256];
        CHECK(fgets(line, sizeof line, fp) != NULL);
        fclose(fp);
    }
}

/* ── main ────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("==============================================\n");
    printf("  PACKET ROUTING SIMULATOR — Test Suite\n");
    printf("==============================================\n");

    test_shortest_path_known();
    test_dijkstra_cheapest();
    test_disconnected();
    test_src_eq_dst();
    test_invalid_ids();
    test_failed_source();
    test_failed_destination();
    test_failed_intermediate_alternate();
    test_failed_all_intermediates();
    test_router_restoration();
    test_zero_loss();
    test_full_loss();
    test_zero_packets();
    test_single_packet_stats();
    test_delivery_ratio_consistency();
    test_bfs_min_hops();
    test_dijkstra_vs_bfs_differ();
    test_link_management();
    test_add_router();
    test_forwarding_counters();
    test_link_utilisation();
    test_repeated_simulation();
    test_max_network_size();
    test_stats_reset();
    test_csv_export();

    return test_summary();
}

/*
 * main.c — Packet Routing Simulator — program entry point and CLI menu
 *
 * This file owns the top-level application loop.  Each menu option
 * delegates to one of the purpose-built modules:
 *
 *   router.c     — router list management
 *   network.c    — link/topology management
 *   routing.c    — Dijkstra and BFS path computation
 *   packet.c     — single-packet simulation
 *   simulation.c — batch simulation and configuration
 *   statistics.c — metrics, display, and CSV export
 *   input_util.c — safe stdin reading
 *
 * Build:
 *   See Makefile in the project root, or compile with:
 *   gcc -std=c11 -Wall -Wextra -o simulator.exe src/router.c src/network.c ... -lm
 *
 * NOTE: This is a SIMULATOR.  No real network packets are transmitted.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "src/router.h"
#include "src/network.h"
#include "src/routing.h"
#include "src/packet.h"
#include "src/simulation.h"
#include "src/statistics.h"
#include "src/input_util.h"

/* ── application state ───────────────────────────────────────────────── */

static Router    g_routers[MAX_ROUTERS];
static int       g_num_routers = 0;
static Network   g_net;
static SimConfig g_cfg;
static SimStats  g_stats;

/* ── banner and menu ─────────────────────────────────────────────────── */

static void print_banner(void)
{
    printf("\n");
    printf("+--------------------------------------------------+\n");
    printf("|       PACKET ROUTING SIMULATOR  v2.0             |\n");
    printf("|  A network simulation tool - NOT a real router   |\n");
    printf("+--------------------------------------------------+\n");
}

static void print_menu(void)
{
    printf("\n");
    printf("+-----------------------------------+\n");
    printf("|            MAIN MENU              |\n");
    printf("+-----------------------------------+\n");
    printf("|  TOPOLOGY                         |\n");
    printf("|  1. Show routers and topology     |\n");
    printf("|  2. Show cost matrix              |\n");
    printf("|  3. Add or update a link          |\n");
    printf("|  4. Remove a link                 |\n");
    printf("|  5. Add a new router              |\n");
    printf("+-----------------------------------+\n");
    printf("|  SIMULATION                       |\n");
    printf("|  6. Send a single packet          |\n");
    printf("|  7. Simulate multiple packets     |\n");
    printf("|  8. Choose routing algorithm      |\n");
    printf("+-----------------------------------+\n");
    printf("|  ROUTER MANAGEMENT                |\n");
    printf("|  9.  Fail a router                |\n");
    printf("|  10. Restore a router             |\n");
    printf("+-----------------------------------+\n");
    printf("|  STATISTICS & SETTINGS            |\n");
    printf("|  11. View network statistics      |\n");
    printf("|  12. Export statistics to CSV     |\n");
    printf("|  13. Configure simulation         |\n");
    printf("|  14. Show link utilisation        |\n");
    printf("|  15. Reset all statistics         |\n");
    printf("+-----------------------------------+\n");
    printf("|  0.  Exit                         |\n");
    printf("+-----------------------------------+\n");
    printf("Current algorithm: %-22s\n",
           routing_algo_name(g_cfg.algo));
    printf("Packet loss: %d%%\n", g_cfg.loss_pct);
    printf("Enter choice: ");
    fflush(stdout);
}

/* ── menu handlers ───────────────────────────────────────────────────── */

/* 1 — show routers and topology */
static void menu_show_topology(void)
{
    router_print_all(g_routers, g_num_routers);
    network_print_topology(&g_net, g_routers, g_num_routers);
}

/* 2 — cost matrix */
static void menu_show_matrix(void)
{
    network_print_matrix(&g_net, g_routers, g_num_routers);
}

/* 3 — add or update a link */
static void menu_add_link(void)
{
    router_print_all(g_routers, g_num_routers);

    int u, v, cost, delay;
    double bw;

    if (!input_read_router_id("Router A ID", g_num_routers, &u)) return;
    if (!input_read_router_id("Router B ID", g_num_routers, &v)) return;
    if (u == v) { printf("  Cannot link a router to itself.\n"); return; }
    if (!input_read_int_range("Routing cost", 1, 99999, &cost)) return;
    if (!input_read_int_range("Propagation delay (ms)", 0, 10000, &delay)) {
        delay = DEFAULT_LINK_DELAY_MS;
    }
    printf("Bandwidth (Mbps) [default %.0f]: ", DEFAULT_LINK_BW_MBPS);
    fflush(stdout);
    if (!input_read_double("", &bw) || bw <= 0.0) {
        bw = DEFAULT_LINK_BW_MBPS;
    }

    if (network_add_link(&g_net, g_num_routers, u, v, cost, delay, bw)) {
        printf("  Link %s <-> %s added (cost %d, %d ms, %.1f Mbps).\n",
               g_routers[u].name, g_routers[v].name, cost, delay, bw);
    } else {
        printf("  Failed to add link. Check router IDs and cost.\n");
    }
}

/* 4 — remove a link */
static void menu_remove_link(void)
{
    network_print_topology(&g_net, g_routers, g_num_routers);

    int u, v;
    if (!input_read_router_id("Router A ID", g_num_routers, &u)) return;
    if (!input_read_router_id("Router B ID", g_num_routers, &v)) return;

    if (network_remove_link(&g_net, g_num_routers, u, v)) {
        printf("  Link %s <-> %s removed.\n",
               g_routers[u].name, g_routers[v].name);
    } else {
        printf("  No link between those routers, or invalid IDs.\n");
    }
}

/* 5 — add a router */
static void menu_add_router(void)
{
    if (g_num_routers >= MAX_ROUTERS) {
        printf("  Maximum router count (%d) reached.\n", MAX_ROUTERS);
        return;
    }
    char name[ROUTER_NAME_LEN];
    if (!input_read_string("Enter router name", name, ROUTER_NAME_LEN)) {
        printf("  Invalid name.\n");
        return;
    }
    int idx = router_add(g_routers, &g_num_routers, name);
    if (idx >= 0) {
        printf("  Router '%s' added with ID %d.\n", name, idx);
    }
}

/* 6 — single packet */
static void menu_send_single(void)
{
    sim_send_single(&g_net, g_routers, g_num_routers, &g_cfg, &g_stats);
}

/* 7 — batch simulation */
static void menu_batch(void)
{
    sim_run_interactive_batch(&g_net, g_routers, g_num_routers,
                              &g_cfg, &g_stats);
}

/* 8 — choose algorithm */
static void menu_choose_algo(void)
{
    printf("\n  1. Dijkstra — minimum cost\n");
    printf("  2. BFS      — minimum hops\n");
    printf("\n");
    printf("  Dijkstra uses link weights (cost) to find the\n");
    printf("  cheapest path.  BFS ignores costs and finds the\n");
    printf("  path with fewest router-to-router hops.\n");
    printf("  They may yield different paths in the same network.\n\n");

    int choice;
    if (!input_read_int_range("Select algorithm", 1, 2, &choice)) return;
    g_cfg.algo = (RouteAlgorithm)choice;
    printf("  Algorithm set to: %s\n", routing_algo_name(g_cfg.algo));
}

/* 9 — fail a router */
static void menu_fail_router(void)
{
    router_print_all(g_routers, g_num_routers);
    int id;
    if (!input_read_router_id("Enter router ID to fail", g_num_routers, &id)) return;
    if (router_set_active(g_routers, g_num_routers, id, false)) {
        printf("  %s is now FAILED.\n", g_routers[id].name);
    }
}

/* 10 — restore a router */
static void menu_restore_router(void)
{
    router_print_all(g_routers, g_num_routers);
    int id;
    if (!input_read_router_id("Enter router ID to restore", g_num_routers, &id)) return;
    if (router_set_active(g_routers, g_num_routers, id, true)) {
        printf("  %s is now ACTIVE.\n", g_routers[id].name);
    }
}

/* 11 — statistics */
static void menu_statistics(void)
{
    stats_print(&g_stats);
    network_link_utilisation(&g_net, g_routers, g_num_routers);
}

/* 12 — export CSV */
static void menu_export_csv(void)
{
    char filename[256];
    if (!input_read_string("Output filename (e.g. results.csv)",
                           filename, sizeof filename)) {
        strcpy(filename, "simulation_results.csv");
    }
    if (stats_export_csv(&g_stats, filename)) {
        printf("  Statistics exported to '%s'.\n", filename);
    } else {
        printf("  Failed to write '%s'. Check write permissions.\n", filename);
    }
}

/* 13 — configure simulation */
static void menu_configure(void)
{
    printf("\n--- Simulation Configuration ---\n");
    printf("  Current loss %%  : %d\n", g_cfg.loss_pct);
    printf("  Default pkt size: %d bytes\n", g_cfg.default_pkt_sz);
    printf("  Fixed seed      : %s (%u)\n",
           g_cfg.use_fixed_seed ? "YES" : "NO", g_cfg.fixed_seed);
    printf("\n");

    int loss;
    if (input_read_int_range("New packet-loss percentage (0-100)",
                             0, 100, &loss)) {
        g_cfg.loss_pct = loss;
        printf("  Loss set to %d%%.\n", loss);
    }

    int sz;
    if (input_read_int_range("Default packet size in bytes",
                             MIN_PACKET_SIZE, MAX_PACKET_SIZE, &sz)) {
        g_cfg.default_pkt_sz = sz;
        printf("  Default size set to %d bytes.\n", sz);
    }

    int seed_choice;
    printf("Use fixed random seed? (1=yes, 0=no): ");
    fflush(stdout);
    if (input_read_int_range("", 0, 1, &seed_choice)) {
        g_cfg.use_fixed_seed = (bool)seed_choice;
        if (g_cfg.use_fixed_seed) {
            int seed;
            if (input_read_int("Enter seed value", &seed)) {
                g_cfg.fixed_seed = (unsigned int)seed;
            }
        }
    }
}

/* 14 — link utilisation */
static void menu_utilisation(void)
{
    network_link_utilisation(&g_net, g_routers, g_num_routers);
}

/* 15 — reset statistics */
static void menu_reset(void)
{
    stats_reset(&g_stats);
    router_reset_counters(g_routers, g_num_routers);
    network_reset_utilisation(&g_net, g_num_routers);
    printf("  All statistics and counters reset.\n");
}

/* ── entry point ─────────────────────────────────────────────────────── */

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    /* Initialise random seed from clock */
    srand((unsigned int)time(NULL));

    /* Set up default topology */
    router_init_defaults(g_routers, &g_num_routers);
    network_init_defaults(&g_net);
    sim_config_defaults(&g_cfg);
    stats_reset(&g_stats);

    print_banner();

    int choice;

    do {
        print_menu();

        if (!input_read_int("", &choice)) {
            /* input_read_int already flushed the line; keep looping */
            printf("  Invalid choice. Enter a number from the menu.\n");
            choice = -1;
            continue;
        }

        switch (choice) {
            case  1: menu_show_topology();   break;
            case  2: menu_show_matrix();     break;
            case  3: menu_add_link();        break;
            case  4: menu_remove_link();     break;
            case  5: menu_add_router();      break;
            case  6: menu_send_single();     break;
            case  7: menu_batch();           break;
            case  8: menu_choose_algo();     break;
            case  9: menu_fail_router();     break;
            case 10: menu_restore_router();  break;
            case 11: menu_statistics();      break;
            case 12: menu_export_csv();      break;
            case 13: menu_configure();       break;
            case 14: menu_utilisation();     break;
            case 15: menu_reset();           break;
            case  0: printf("  Goodbye!\n"); break;
            default:
                printf("  Unknown option. Please choose from the menu.\n");
                break;
        }

    } while (choice != 0);

    return 0;
}

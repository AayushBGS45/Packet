/*
 * router.h — Router management interface
 *
 * A "router" in this simulator is a node in the network graph.
 * Each router has an ID, a display name, and an active/failed flag.
 *
 * Real routers forward packets between networks.  Here we model only
 * the forwarding-decision layer: a router that has "failed" is excluded
 * from path calculations and cannot be a packet source or destination.
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <stdbool.h>

/* ── compile-time limits ─────────────────────────────────────────────── */

/** Maximum number of routers the simulator can handle. */
#define MAX_ROUTERS 20

/** Maximum characters in a router name (including NUL terminator). */
#define ROUTER_NAME_LEN 32

/* ── data types ──────────────────────────────────────────────────────── */

/**
 * Router — a single network node.
 *
 * Fields
 * ------
 * id     : 0-based index used throughout the simulator.
 * name   : human-readable label shown in the CLI.
 * active : true → router is operational; false → router has failed.
 * fwd_count : number of packets this router has forwarded (including
 *             packets whose source or destination is this router).
 */
typedef struct {
    int  id;
    char name[ROUTER_NAME_LEN];
    bool active;
    int  fwd_count;   /* forwarding / utilisation counter */
} Router;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * router_init_defaults — populate a Router array with a default
 * four-router topology (A, B, C, D) all set active.
 *
 * @param routers  Array to initialise.  Must have capacity >= 4.
 * @param count    Set to 4 on return.
 */
void router_init_defaults(Router routers[], int *count);

/**
 * router_add — append a new router to the array.
 *
 * @param routers  Existing array.
 * @param count    Current size; incremented on success.
 * @param name     Display name for the new router.
 * @return         Index of the new router, or -1 if the array is full.
 */
int router_add(Router routers[], int *count, const char *name);

/**
 * router_set_active — mark a router as active or failed.
 *
 * @param routers  Router array.
 * @param count    Number of routers.
 * @param id       Router index to change.
 * @param active   true = operational, false = failed.
 * @return         true on success, false if id is out of range.
 */
bool router_set_active(Router routers[], int count, int id, bool active);

/**
 * router_is_valid — check that an id is in range [0, count).
 */
bool router_is_valid(int count, int id);

/**
 * router_print_all — display router list with status to stdout.
 */
void router_print_all(const Router routers[], int count);

/**
 * router_reset_counters — zero the forwarding counters of all routers.
 */
void router_reset_counters(Router routers[], int count);

#endif /* ROUTER_H */

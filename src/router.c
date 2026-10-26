/*
 * router.c — Router management implementation
 */

#include "router.h"

#include <stdio.h>
#include <string.h>

/* ── public functions ────────────────────────────────────────────────── */

void router_init_defaults(Router routers[], int *count)
{
    /* Four-router default topology that mirrors the original project. */
    static const char *default_names[] = {
        "Router-A", "Router-B", "Router-C", "Router-D"
    };
    int n = 4;

    for (int i = 0; i < n; i++) {
        routers[i].id        = i;
        routers[i].active    = true;
        routers[i].fwd_count = 0;
        strncpy(routers[i].name, default_names[i], ROUTER_NAME_LEN - 1);
        routers[i].name[ROUTER_NAME_LEN - 1] = '\0';
    }

    *count = n;
}

int router_add(Router routers[], int *count, const char *name)
{
    if (*count >= MAX_ROUTERS) {
        return -1;   /* array is full */
    }

    int idx = *count;
    routers[idx].id        = idx;
    routers[idx].active    = true;
    routers[idx].fwd_count = 0;
    strncpy(routers[idx].name, name, ROUTER_NAME_LEN - 1);
    routers[idx].name[ROUTER_NAME_LEN - 1] = '\0';

    (*count)++;
    return idx;
}

bool router_set_active(Router routers[], int count, int id, bool active)
{
    if (!router_is_valid(count, id)) {
        return false;
    }
    routers[id].active = active;
    return true;
}

bool router_is_valid(int count, int id)
{
    return (id >= 0 && id < count);
}

void router_print_all(const Router routers[], int count)
{
    printf("\n--- Network Routers ---\n");
    printf("%-4s %-20s %-10s %s\n",
           "ID", "Name", "Status", "Fwd Count");
    printf("%-4s %-20s %-10s %s\n",
           "--", "----", "------", "---------");

    for (int i = 0; i < count; i++) {
        printf("%-4d %-20s %-10s %d\n",
               routers[i].id,
               routers[i].name,
               routers[i].active ? "ACTIVE" : "FAILED",
               routers[i].fwd_count);
    }
}

void router_reset_counters(Router routers[], int count)
{
    for (int i = 0; i < count; i++) {
        routers[i].fwd_count = 0;
    }
}

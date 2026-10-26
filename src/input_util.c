/*
 * input_util.c — Safe user-input implementation
 */

#include "input_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── buffer flusher ──────────────────────────────────────────────────── */

void input_flush_line(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* discard */
    }
}

static const char *skip_bom_and_spaces(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        p += 3;
    }
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        p++;
    }
    return (const char *)p;
}

/* ── integer ─────────────────────────────────────────────────────────── */

bool input_read_int(const char *prompt, int *out)
{
    if (prompt && prompt[0] != '\0') {
        printf("%s: ", prompt);
        fflush(stdout);
    }

    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }

    const char *p = skip_bom_and_spaces(buf);
    if (*p == '\0') {
        return false;
    }

    char *endptr;
    long val = strtol(p, &endptr, 10);
    if (endptr == p) {
        return false;
    }

    *out = (int)val;
    return true;
}

bool input_read_int_range(const char *prompt, int lo, int hi, int *out)
{
    if (prompt && prompt[0] != '\0') {
        printf("%s (%d-%d): ", prompt, lo, hi);
        fflush(stdout);
    }

    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }

    const char *p = skip_bom_and_spaces(buf);
    if (*p == '\0') {
        printf("  Invalid input. Expected a number between %d and %d.\n", lo, hi);
        return false;
    }

    char *endptr;
    long val = strtol(p, &endptr, 10);
    if (endptr == p) {
        printf("  Invalid input. Expected a number between %d and %d.\n",
               lo, hi);
        return false;
    }

    if (val < lo || val > hi) {
        printf("  Value %ld is out of range [%d, %d].\n", val, lo, hi);
        return false;
    }

    *out = (int)val;
    return true;
}

/* ── router ID ───────────────────────────────────────────────────────── */

bool input_read_router_id(const char *prompt, int num_routers, int *out)
{
    printf("%s (0-%d): ", prompt, num_routers - 1);
    fflush(stdout);

    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }

    const char *p = skip_bom_and_spaces(buf);
    if (*p == '\0') {
        printf("  Invalid input. Expected a router ID.\n");
        return false;
    }

    char *endptr;
    long val = strtol(p, &endptr, 10);
    if (endptr == p) {
        printf("  Invalid input. Expected a router ID.\n");
        return false;
    }

    if (val < 0 || val >= num_routers) {
        printf("  Router ID %ld is out of range [0, %d].\n",
               val, num_routers - 1);
        return false;
    }

    *out = (int)val;
    return true;
}

/* ── double ──────────────────────────────────────────────────────────── */

bool input_read_double(const char *prompt, double *out)
{
    if (prompt && prompt[0] != '\0') {
        printf("%s: ", prompt);
        fflush(stdout);
    }

    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }

    const char *p = skip_bom_and_spaces(buf);
    if (*p == '\0') {
        printf("  Invalid input. Expected a number.\n");
        return false;
    }

    char *endptr;
    double val = strtod(p, &endptr);
    if (endptr == p) {
        printf("  Invalid input. Expected a number.\n");
        return false;
    }

    *out = val;
    return true;
}

/* ── string ──────────────────────────────────────────────────────────── */

bool input_read_string(const char *prompt, char *buf, int max_len)
{
    if (prompt && prompt[0] != '\0') {
        printf("%s: ", prompt);
        fflush(stdout);
    }

    char line[512];
    if (!fgets(line, sizeof(line), stdin)) {
        return false;
    }

    const char *p = skip_bom_and_spaces(line);
    strncpy(buf, p, max_len - 1);
    buf[max_len - 1] = '\0';

    /* Remove trailing newline or carriage return */
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }

    return (strlen(buf) > 0);
}

/*
 * packet.h — Packet data types and transmission simulation interface
 *
 * IMPORTANT: This is a SIMULATOR.
 * No real network packets are transmitted.  All "transmission" is a
 * mathematical model: we compute expected delay from link bandwidth and
 * packet size, and apply a configurable random packet-loss probability.
 *
 * Model:
 *   transmission_delay = (packet_size_bytes * 8) / (bw_mbps * 1e6) * 1000 ms
 *   end_to_end_delay   = sum(propagation_delay per hop) + transmission_delay
 *   packet_loss        = rand() / RAND_MAX < loss_prob
 */

#ifndef PACKET_H
#define PACKET_H

#include "routing.h"
#include "network.h"
#include "router.h"
#include <stdbool.h>

/* ── constants ───────────────────────────────────────────────────────── */

/** Default packet size in bytes if the user does not specify one. */
#define DEFAULT_PACKET_SIZE 1024

/** Maximum packet size allowed (1 MB). */
#define MAX_PACKET_SIZE (1024 * 1024)

/** Minimum packet size (1 byte). */
#define MIN_PACKET_SIZE 1

/* ── data types ──────────────────────────────────────────────────────── */

/**
 * PacketStatus — outcome of a simulated packet transmission.
 */
typedef enum {
    PKT_DELIVERED          = 0,
    PKT_LOST_RANDOM        = 1,   /* random link loss */
    PKT_LOST_NO_ROUTE      = 2,   /* no path between src and dst */
    PKT_LOST_SRC_FAILED    = 3,   /* source router is down */
    PKT_LOST_DST_FAILED    = 4,   /* destination router is down */
    PKT_LOST_INTERMEDIATE  = 5,   /* an intermediate router failed mid-route */
} PacketStatus;

/**
 * Packet — represents one simulated network packet.
 *
 * Fields
 * ------
 * id             : globally unique packet serial number.
 * src, dst       : router indices for source and destination.
 * size_bytes     : payload size in bytes.
 * route          : computed path result (algorithm-specific).
 * status         : delivery outcome.
 * delay_ms       : simulated end-to-end delay in milliseconds.
 * tx_delay_ms    : transmission delay component (from bandwidth model).
 */
typedef struct {
    int          id;
    int          src;
    int          dst;
    int          size_bytes;
    PathResult   route;
    PacketStatus status;
    double       delay_ms;
    double       tx_delay_ms;
} Packet;

/* ── function declarations ───────────────────────────────────────────── */

/**
 * packet_create — initialise a Packet with the given parameters.
 *
 * @param pkt        Packet to initialise.
 * @param id         Serial number (caller assigns).
 * @param src, dst   Router indices.
 * @param size_bytes Payload size in bytes.
 */
void packet_create(Packet *pkt, int id, int src, int dst, int size_bytes);

/**
 * packet_simulate_transmission — compute the transmission outcome.
 *
 * Runs the selected routing algorithm, checks for route existence, then
 * applies the probabilistic loss model.  Updates pkt->status, pkt->route,
 * pkt->delay_ms, and pkt->tx_delay_ms.
 *
 * Also increments router forwarding counters and link utilisation.
 *
 * @param pkt         Packet to simulate (in/out).
 * @param net         Network topology (link utilisation updated).
 * @param routers     Router state (forwarding counters updated).
 * @param num_routers Number of routers.
 * @param algo        Routing algorithm to use.
 * @param loss_pct    Packet loss probability in percent [0, 100].
 */
void packet_simulate_transmission(Packet *pkt,
                                  Network *net,
                                  Router routers[], int num_routers,
                                  RouteAlgorithm algo,
                                  int loss_pct);

/**
 * packet_print — display one packet's result to stdout.
 *
 * @param pkt      Packet to display.
 * @param routers  For name lookup.
 * @param verbose  If true, also print the full route.
 */
void packet_print(const Packet *pkt,
                  const Router routers[],
                  bool verbose);

/**
 * packet_status_str — return a short string describing the status.
 */
const char *packet_status_str(PacketStatus status);

/**
 * packet_compute_tx_delay — compute transmission delay in ms.
 *
 * tx_delay = (size_bytes * 8) / (bw_mbps * 1_000_000) * 1000
 *
 * @param size_bytes  Packet size in bytes.
 * @param bw_mbps     Bottleneck link bandwidth in Mbps.
 * @return            Transmission delay in milliseconds.
 */
double packet_compute_tx_delay(int size_bytes, double bw_mbps);

#endif /* PACKET_H */

# Packet Routing Simulator

A robust, modular, and professional-grade Computer Networking and Data Structures simulation engine written in C11. Developed for Windows with GCC (MSYS2 UCRT64) and VS Code.

---

## 📋 Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Data Structures & Algorithms](#data-structures--algorithms)
- [Networking & Simulation Models](#networking--simulation-models)
- [Module Breakdown](#module-breakdown)
- [Building & Running](#building--running)
- [VS Code Integration](#vs-code-integration)
- [Automated Test Suite](#automated-test-suite)
- [Interactive CLI Reference](#interactive-cli-reference)
- [Academic Documentation & Design Notes](#academic-documentation--design-notes)

---

## Overview

The **Packet Routing Simulator** is an academic-grade discrete event and graph-algorithmic networking simulator. It models a dynamic packet-switched communication network consisting of software-defined routers, bidirectional weighted transmission links with delay and bandwidth parameters, packet forwarding engines, router failure/restoration scenarios, and statistical performance monitoring.

### Key Capabilities
- **Dual Routing Paradigms**: Dijkstra's Shortest Path Algorithm (optimal metric cost) and Breadth-First Search (BFS, minimum hop count).
- **Realistic Network Constraints**: Edge costs (metrics), link transmission delays (ms), bandwidth capacities (Mbps), and dynamic link utilisation counters.
- **Resilience & Fault Modeling**: Router downtime/failure modeling, automatic fallback rerouting around offline nodes, and recovery/restoration handling.
- **Probabilistic Packet Loss**: Configurable stochastic drop rates (0–100%) and deterministic failure tracking (e.g., source offline, destination unreachable, network partitions).
- **Statistical Analytics**: End-to-end packet delivery ratio, latency tracking (min, avg, max), routing cost totals, per-router forwarding counts, and CSV export for post-run analysis.
- **100% Automated Testing**: Comprehensive unit and integration test suite with 81 assertions across 25 rigorous test cases, testing edge cases, boundary conditions, and graph topologies.

---

## System Architecture

The project adheres to strict separation of concerns, defensive programming, and modular C design. Global state is completely eliminated in favor of clean structure instances passed by pointer.

```
                      +-------------------+
                      |      main.c       |
                      | (Interactive CLI) |
                      +---------+---------+
                                |
        +-----------------------+-----------------------+
        |                       |                       |
        v                       v                       v
+---------------+       +---------------+       +---------------+
| simulation.c  |       |  statistics.c |       |  input_util.c |
| (Batch Engine)|       | (Metrics/CSV) |       | (Safe Stdin)  |
+-------+-------+       +---------------+       +---------------+
        |
        +-----------------------+
        |                       |
        v                       v
+---------------+       +---------------+
|   packet.c    |       |   routing.c   |
| (Packet Flow) |       | (Dijkstra/BFS)|
+-------+-------+       +-------+-------+
        |                       |
        +-----------+-----------+
                    |
                    v
            +---------------+
            |   network.c   |
            | (Graph / Net) |
            +-------+-------+
                    |
                    v
            +---------------+
            |   router.c    |
            | (Node State)  |
            +---------------+
```

---

## Data Structures & Algorithms

### 1. Graph Representation: Adjacency Matrix with Link Attributes
The network topology is modeled using an $N \times N$ matrix supporting up to `MAX_ROUTERS = 20` nodes:
```c
typedef struct {
    int   cost[MAX_ROUTERS][MAX_ROUTERS];       /* Link cost (0 = no direct link) */
    LinkAttr attr[MAX_ROUTERS][MAX_ROUTERS];   /* Delay, Bandwidth, Utilisation */
} Network;
```
- **Lookup Complexity**: $O(1)$ edge existence and metric queries.
- **Space Complexity**: $O(V^2)$ where $V \le 20$, easily fitting within L1 CPU cache.

### 2. Dijkstra's Shortest Path Algorithm ($O(V^2)$)
- Implemented in `src/routing.c`.
- Computes the minimum-cost route between source and destination routers based on link metric costs.
- Automatically bypasses failed (`active == false`) intermediate routers.
- Returns reachability, total path metric cost, accumulated end-to-end delay, and explicit node sequence.

### 3. Breadth-First Search (BFS) for Minimum-Hop Routing ($O(V + E)$)
- Implemented in `src/routing.c`.
- Utilises a FIFO queue to discover routes with the strictly fewest intermediate router hops regardless of link cost weights.
- Demonstrates algorithmic differences in routing decisions under asymmetric metric configurations.

### 4. Router Node State & Metrics
```c
typedef struct {
    int  id;
    char name[ROUTER_NAME_LEN];
    bool active;
    int  fwd_count;
} Router;
```

---

## Networking & Simulation Models

### Packet Lifecycle & States
Every packet is identified and tracked through its state lifecycle:
- `PKT_CREATED`: Packet instantiated with payload size, sequence ID, source, and destination.
- `PKT_DELIVERED`: Route discovered and successfully traversed without stochastic drops.
- `PKT_LOST_RANDOM`: Dropped in-flight by the simulated channel loss model.
- `PKT_LOST_NO_ROUTE`: No connected path exists between source and destination (network partition).
- `PKT_LOST_SRC_FAILED`: Source router is down/inactive.
- `PKT_LOST_DST_FAILED`: Destination router is down/inactive.

### Channel Loss Simulation
Loss is determined using uniform pseudorandom sampling:
$$\text{Dropped} \iff \text{rand}() < \left(\frac{\text{drop\_rate}}{100.0} \times (\text{RAND\_MAX} + 1.0)\right)$$

### Latency Calculation
The simulator tracks link propagation delays across traversed hops:
$$\text{Delay}_{\text{total}} = \sum_{i=0}^{k-1} \text{attr}[u_i][u_{i+1}].\text{delay\_ms}$$

---

## Module Breakdown

| Header File | Implementation | Primary Purpose |
| :--- | :--- | :--- |
| `src/router.h` | `src/router.c` | Node management: initialization, name assignment, status toggle, forwarding counters. |
| `src/network.h` | `src/network.c` | Graph topology: link addition, deletion, metric query, delay/bandwidth configuration. |
| `src/routing.h` | `src/routing.c` | Graph algorithms: Dijkstra's algorithm, Breadth-First Search (BFS), path reconstruction. |
| `src/packet.h` | `src/packet.c` | Packet lifecycle: creation, transmission simulation, status inspection. |
| `src/simulation.h` | `src/simulation.c` | Batch execution: multi-packet traffic generation, logging, failure injection. |
| `src/statistics.h` | `src/statistics.c` | Metrics: aggregation, ANSI terminal reports, CSV export. |
| `src/input_util.h` | `src/input_util.c` | Robust CLI I/O: input buffer flushing, bounded integer/string parsing, input sanitization. |
| `main.c` | — | Interactive terminal user interface and menu driver. |
| `tests/tests.c` | — | Standalone unit and integration test suite (81 checks). |

---

## Building & Running

### Prerequisites
- Windows 10/11
- GCC with C11 support (Recommended: MSYS2 UCRT64: `C:\msys64\ucrt64\bin\gcc.exe`)

### 1. Compile the Simulator
```powershell
C:\msys64\ucrt64\bin\gcc.exe -std=c11 -Wall -Wextra -g main.c src/router.c src/network.c src/routing.c src/packet.c src/simulation.c src/statistics.c src/input_util.c -o simulator.exe -lm
```

### 2. Run the Simulator
```powershell
.\simulator.exe
```

### 3. Compile and Run the Test Suite
```powershell
C:\msys64\ucrt64\bin\gcc.exe -std=c11 -Wall -Wextra -g -I. tests/tests.c src/router.c src/network.c src/routing.c src/packet.c src/statistics.c src/input_util.c -o tests/run_tests.exe -lm
.\tests\run_tests.exe
```

---

## VS Code Integration

The project includes pre-configured `.vscode` configurations:

- **Build Simulator**: Press `Ctrl+Shift+B` (Runs `Build Simulator (all sources)` defined in `.vscode/tasks.json`).
- **Run Tests**: `Terminal` -> `Run Task...` -> `Build and Run Tests`.
- **GDB Debugging**: Press `F5` to start debugging with GDB (Select `Debug Simulator (GDB)` or `Debug Tests (GDB)` in `.vscode/launch.json`).

---

## Automated Test Suite

The test suite in `tests/tests.c` executes 25 test categories with 81 explicit assertions:

| Test ID | Test Name | Scope |
| :---: | :--- | :--- |
| **T01** | Known Shortest Path | Verifies canonical 4-router path ($A \to B \to C \to D = \text{cost } 4$). |
| **T02** | Dijkstra Cheapest Path | Compares multi-hop cheap path vs high-cost direct path. |
| **T03** | Disconnected Graph | Detects unreachable destinations in partitioned graphs. |
| **T04** | Source == Destination | Verifies base case: zero cost, 1 hop, immediate delivery. |
| **T05** | Invalid Router IDs | Validates bounds checking on out-of-range IDs. |
| **T06** | Failed Source Router | Asserts `PKT_LOST_SRC_FAILED` status when source is offline. |
| **T07** | Failed Destination Router | Asserts `PKT_LOST_DST_FAILED` status when destination is offline. |
| **T08** | Intermediate Router Failure | Validates dynamic rerouting around failed transit nodes. |
| **T09** | All Paths Blocked | Validates unreachability when all candidate paths are cut. |
| **T10** | Router Restoration | Re-enables router and confirms path restoration. |
| **T11** | Zero Packet Loss | Validates 100% delivery under 0% loss condition. |
| **T12** | 100% Packet Loss | Validates 0% delivery under 100% loss condition. |
| **T13** | Zero Packets | Verifies safe division-by-zero protection in statistics. |
| **T14** | Single Packet Tracking | Verifies correct field updates for a single transmission. |
| **T15** | Delivery Ratio Consistency | Asserts Delivered + Lost == Total Attempted over 200 packets. |
| **T16** | BFS Minimum Hops | Verifies BFS identifies shortest hop count path. |
| **T17** | Dijkstra vs BFS Difference | Demonstrates case where Dijkstra cost != BFS hop path. |
| **T18** | Link Management | Tests dynamic link addition, removal, and symmetry. |
| **T19** | Dynamic Router Addition | Validates appending new nodes to active topology. |
| **T20** | Forwarding Counters | Verifies packet transit increment counters on routers. |
| **T21** | Link Utilisation | Verifies edge utilization counters across transmitted batches. |
| **T22** | Multi-Run Accumulation | Verifies aggregate statistics accumulation across runs. |
| **T23** | Max Network Scalability | Validates chain topology with maximum nodes (`MAX_ROUTERS = 20`). |
| **T24** | Statistics Reset | Confirms all counters reset cleanly to 0. |
| **T25** | CSV Export | Validates file generation and RFC-compliant CSV structure. |

---

## Interactive CLI Reference

```
======================================================
         PACKET ROUTING SIMULATOR (C11)
======================================================
  [1] Display Network Topology & Router Status
  [2] Send Single Packet (Step-by-Step Trace)
  [3] Run Batch Simulation (Multi-Packet)
  [4] Toggle Router Status (Fail / Restore)
  [5] Add / Remove Network Link
  [6] Add New Router
  [7] Select Routing Algorithm (Dijkstra / BFS)
  [8] View Simulation Statistics
  [9] Reset Statistics
  [10] Export Statistics to CSV
  [0] Exit Simulator
======================================================
```

---

## Academic Documentation & Design Notes

### Defensive Programming Highlights
- **No Buffer Overflows**: All user input is read via `fgets` and safely tokenized with `strtol`/`strtod` in [src/input_util.c](file:///c:/Users/aayus/Desktop/Packet/src/input_util.c).
- **Null-Pointer Resilient**: All public functions check input pointers (`net`, `routers`, `result`, `pkt`, `stats`).
- **Division-by-Zero Guarded**: All statistics computations check `packets_attempted > 0` before calculating ratios or averages.
- **Portability**: Uses standard C11 (`#include <stdbool.h>`, `<stdint.h>`) without compiler-specific extensions or OS-dependent system calls.

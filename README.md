# Network Optimization 3 (C++) — Customer Clustering & Territory Assignment MIP

A C++17 reimplementation of "Network Optimization 3," a mixed-integer
programming (MIP) model I designed and deployed for global supply chain
network design at Veolia. The original is a Python model solved with
COIN-OR/CBC, GLPK, and PuLP; this repo ports the same formulation to C++ to
show the algorithm itself — the model, the exact solver, and the tests —
independent of any particular solver binding.

It's a companion to
[`network-optimization-2`](https://github.com/cosmosanalytics/network-optimization-2)
(truck-load packing), and follows the same structure: a solver-agnostic
problem definition, a `Strategy` interface, a fast heuristic, a
dependency-free exact solver, and (behind a build flag) a production path
through COIN-OR CBC.

## The problem

Given a set of customers (demand points, each with a location and a
demand) and a set of candidate territory hubs (distribution centers, each
with a location, a capacity, and a fixed cost to open), decide:

1. **Which hubs to open.**
2. **Which open hub serves each customer** — i.e. how to draw territory
   boundaries.

...to minimize total cost: the fixed cost of every opened hub, plus a
distance- and demand-weighted service cost for every customer-hub
assignment, subject to every open hub's capacity. This is the classic
*capacitated facility location problem*, framed here the way the business
actually uses it — as customer clustering and territory design, trading
off service cost against how many distribution centers you commit to
running.

### MIP formulation

For customers `i = 1..n` and candidate hubs `j = 1..m`:

- `x_ij ∈ {0,1}` — customer `i` is served by hub `j`
- `y_j ∈ {0,1}` — hub `j` is opened
- `cost_ij = distance(i, j) * demand_i * ratePerUnitDistance`

```
minimize   Σ_ij cost_ij * x_ij   +   Σ_j fixedCost_j * y_j

subject to Σ_j x_ij = 1                                for every customer i
           Σ_i demand_i * x_ij  ≤  capacity_j * y_j     for every hub j
           x_ij, y_j ∈ {0, 1}
```

The capacity constraint does double duty: it caps how much demand a hub
can serve, and it forces `y_j = 1` (paying the fixed cost) before any
customer can be routed through hub `j`, since `x_ij` is otherwise
unconstrained.

## Design

- **`ClusteringProblem`** owns the customers, the candidate hubs, and the
  cost rate; it computes distances/assignment costs and validates any
  candidate `ClusteringSolution` (feasibility + total cost), independent
  of how that solution was produced.
- **`ClusteringSolver`** is a small `Strategy` interface (`solve`, `name`)
  so `main.cpp` and the tests can swap backends freely.
- **`GreedyNearestHubSolver`** is a fast largest-demand-first heuristic —
  the facility-location analogue of first-fit-decreasing bin packing:
  place the hardest-to-fit customers first, always to the nearest hub
  with remaining capacity, opening hubs on demand.
- **`BranchAndBoundSolver`** is a from-scratch exact solver with zero
  external dependencies, so the whole project builds anywhere with a
  C++17 compiler. It branches over which hub serves each customer,
  seeds its incumbent from the greedy solution, and prunes with an
  admissible lower bound (the cheapest possible per-hub distance cost for
  every not-yet-assigned customer). Exponential in the worst case, like
  any exact facility-location solver — intended for the small/medium
  instances a demo or a regression test uses.
- **`CbcMipSolver`** (behind `NETOPT3_USE_CBC`) documents the same model
  expressed directly against COIN-OR CBC's C++ API (`CbcModel`,
  `OsiClpSolverInterface`), i.e. the production-scale path.

## Build & run

```sh
cmake -B build
cmake --build build

./build/network_opt3_demo    # runs greedy + exact solvers on a sample instance
./build/network_opt3_tests   # unit tests
```

To build with the real CBC backend instead of just documenting it:

```sh
sudo apt-get install coinor-libcbc-dev coinor-libclp-dev \
                      coinor-libosi-dev coinor-libcoinutils-dev
cmake -DUSE_CBC=ON -B build
cmake --build build
```

## Layout

```
include/   Customer, Hub, ClusteringProblem, ClusteringSolver interface,
           GreedyNearestHubSolver, BranchAndBoundSolver, CbcMipSolver
src/       Implementations of the above (ClusteringProblem.cpp,
           GreedyNearestHubSolver.cpp, BranchAndBoundSolver.cpp) + main.cpp
tests/     Dependency-free unit test harness (TestFramework.h) and
           ClusteringTests.cpp — feasibility checks, a hand-verified
           optimum, greedy-vs-exact comparison, and edge cases
```

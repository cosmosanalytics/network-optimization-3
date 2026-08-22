# Network Optimization 3 (Python) — Customer Clustering & Territory Assignment MIP

A Python 3 port of "Network Optimization 3," a mixed-integer programming
(MIP) model for global supply chain network design. This mirrors the
original C++ port's structure — a solver-agnostic problem definition, a
`Solver` strategy interface, a fast heuristic, a dependency-free exact
solver, and a production-scale MIP path — using PuLP/CBC in place of raw
COIN-OR CBC bindings.

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
*capacitated facility location problem*, framed as customer clustering
and territory design, trading off service cost against how many
distribution centers you commit to running.

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

- **`netopt3.problem.ClusteringProblem`** owns the customers, the
  candidate hubs, and the cost rate; it computes distances/assignment
  costs and validates any candidate `ClusteringSolution` (feasibility +
  total cost) independently of how that solution was produced —
  recomputing capacity checks and activation-cost accounting from
  scratch, the same logic as the C++ `ClusteringProblem::validate`.
- **`netopt3.solver.ClusteringSolver`** is a small `ABC` (`solve`,
  `name`) so `main.py` and the tests can swap backends freely.
- **`netopt3.greedy_solver.GreedyNearestHubSolver`** is a fast
  largest-demand-first heuristic: place the hardest-to-fit customers
  first, always to the nearest hub with remaining capacity, opening hubs
  on demand.
- **`netopt3.exact_solver.BranchAndBoundSolver`** is a from-scratch exact
  solver with zero external dependencies — stdlib only. It branches over
  which hub serves each customer, seeds its incumbent from the greedy
  solution, and prunes with an admissible lower bound (the cheapest
  possible per-hub distance cost for every not-yet-assigned customer).
  Exponential in the worst case, intended for the small/medium instances
  the test suite uses. Because greedy and exact are dependency-free, the
  full test suite runs with no extra installs.
- **`netopt3.pulp_solver.PulpMipSolver`** expresses the same model
  against [PuLP](https://coin-or.github.io/pulp/) targeting CBC — the
  production-scale path, mirroring the C++ project's `CbcMipSolver.h`
  (there, "documentary," gated behind a CMake build flag needing the CBC
  dev libraries; here, gated behind `pip install pulp` instead). `pulp`
  is imported lazily inside `solve()`, so importing `netopt3` never
  requires it, and its test is skipped automatically when pulp isn't
  installed.

## Build & run

```sh
pip install -r requirements.txt      # optional: only needed for the pulp solver/tests
python3 -m unittest discover -s tests -v
python3 main.py
```

## Layout

```
netopt3/   customer.py, hub.py, problem.py (ClusteringProblem + validate),
           solver.py (Solver ABC), greedy_solver.py, exact_solver.py,
           pulp_solver.py
tests/     test_clustering.py — feasibility checks, a hand-verified
           optimum, greedy-vs-exact comparison, edge cases, and a
           pulp-vs-exact cost check (skipped if pulp isn't installed)
main.py    demo: greedy + exact (+ pulp, if available) on a sample instance
```

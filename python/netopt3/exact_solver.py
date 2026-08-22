"""Port of BranchAndBoundSolver.h / .cpp."""

import math
from typing import List

from .greedy_solver import GreedyNearestHubSolver
from .problem import ClusteringProblem, ClusteringSolution, Territory
from .solver import ClusteringSolver

_CAPACITY_EPS = 1e-9


class _SearchState:
    __slots__ = (
        "problem",
        "customers",
        "hubs",
        "cheapest_possible_cost",
        "suffix_lower_bound",
        "assignment",
        "remaining",
        "opened",
        "best_assignment",
        "best_cost",
    )


def _search(s: _SearchState, customer_idx: int, running_cost: float) -> None:
    n = len(s.customers)
    if customer_idx == n:
        if running_cost < s.best_cost:
            s.best_cost = running_cost
            s.best_assignment = list(s.assignment)
        return

    if running_cost + s.suffix_lower_bound[customer_idx] >= s.best_cost:
        return  # even the most optimistic completion can't beat the incumbent

    customer = s.customers[customer_idx]

    # Try nearer hubs first so a good incumbent is found early, which
    # makes the lower-bound prune above effective sooner.
    hub_order = sorted(range(len(s.hubs)), key=lambda j: s.problem.distance(customer, s.hubs[j]))

    for j in hub_order:
        hub = s.hubs[j]
        if s.remaining[j] + _CAPACITY_EPS < customer.demand:
            continue

        added_fixed = 0.0 if s.opened[j] else hub.fixed_cost
        added_assignment = s.problem.assignment_cost(customer, hub)
        was_opened = s.opened[j]

        s.assignment[customer_idx] = j
        s.remaining[j] -= customer.demand
        s.opened[j] = True

        _search(s, customer_idx + 1, running_cost + added_assignment + added_fixed)

        s.remaining[j] += customer.demand
        s.opened[j] = was_opened

    s.assignment[customer_idx] = -1


class BranchAndBoundSolver(ClusteringSolver):
    """Exact, dependency-free solver for the capacitated facility-location
    / territory-assignment problem.

    Branches over which hub each customer is assigned to, tracking
    per-hub remaining capacity and which hubs have already been "opened"
    (so a hub's fixed cost is only charged once). Prunes with an
    admissible lower bound: the cheapest possible per-hub distance cost
    for every not-yet-assigned customer, ignoring capacity and fixed
    cost (both of which can only make the true remaining cost higher).

    Exponential in the worst case, like any exact facility-location
    solver, so it is meant for the small/medium instances a portfolio
    demo or a regression test uses -- pulp_solver.py is the path for
    production scale.
    """

    def solve(self, problem: ClusteringProblem) -> ClusteringSolution:
        customers = problem.customers
        hubs = problem.hubs

        result = ClusteringSolution()
        if not customers:
            result.feasible = True
            result.total_cost = 0.0
            return result

        s = _SearchState()
        s.problem = problem
        s.customers = customers
        s.hubs = hubs
        s.assignment = [-1] * len(customers)
        s.remaining = [hub.capacity for hub in hubs]
        s.opened = [False] * len(hubs)

        # Admissible lower bound: cheapest single-hub distance cost per
        # customer, ignoring capacity and fixed cost.
        s.cheapest_possible_cost = []
        for customer in customers:
            best = math.inf
            for hub in hubs:
                best = min(best, problem.assignment_cost(customer, hub))
            s.cheapest_possible_cost.append(0.0 if not hubs else best)

        s.suffix_lower_bound = [0.0] * (len(customers) + 1)
        for i in range(len(customers) - 1, -1, -1):
            s.suffix_lower_bound[i] = s.suffix_lower_bound[i + 1] + s.cheapest_possible_cost[i]

        # Seed the incumbent with the greedy heuristic's result so pruning is
        # effective from the very first branch instead of only after the
        # search stumbles onto a decent solution by luck.
        greedy = GreedyNearestHubSolver()
        greedy_solution = greedy.solve(problem)
        s.best_cost = greedy_solution.total_cost if greedy_solution.feasible else math.inf
        s.best_assignment = []

        if hubs:
            _search(s, 0, 0.0)

        if not s.best_assignment and not greedy_solution.feasible:
            # No feasible assignment exists at all (e.g. total demand exceeds
            # total capacity) -- report the infeasible greedy attempt so the
            # caller can see how far off it was.
            return greedy_solution
        if not s.best_assignment:
            return greedy_solution  # greedy already found the (only) feasible answer

        territory_index_by_hub_idx: List[int] = [-1] * len(hubs)
        for i, customer in enumerate(customers):
            hub_idx = s.best_assignment[i]
            if territory_index_by_hub_idx[hub_idx] == -1:
                territory_index_by_hub_idx[hub_idx] = len(result.territories)
                result.territories.append(Territory(hubs[hub_idx].id, [], 0.0))
            territory = result.territories[territory_index_by_hub_idx[hub_idx]]
            territory.customer_ids.append(customer.id)
            territory.used_capacity += customer.demand

        problem.validate(result)
        return result

    def name(self) -> str:
        return "BranchAndBound-Exact"

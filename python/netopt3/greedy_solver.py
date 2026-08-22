"""Port of GreedyNearestHubSolver.h / .cpp."""

import math
from typing import Dict, List

from .problem import ClusteringProblem, ClusteringSolution, Territory
from .solver import ClusteringSolver

_CAPACITY_EPS = 1e-9


class GreedyNearestHubSolver(ClusteringSolver):
    """Fast heuristic: largest-demand-first, nearest hub with capacity.

    The facility-location analogue of first-fit-decreasing bin packing:
    place the hardest-to-fit customers first, always to the nearest hub
    with remaining capacity, opening hubs on demand. Falls back to the
    nearest hub regardless of capacity if nothing fits, so the caller
    always gets a solution to inspect -- ClusteringProblem.validate() is
    what actually flags infeasibility.
    """

    def solve(self, problem: ClusteringProblem) -> ClusteringSolution:
        customers = problem.customers
        hubs = problem.hubs

        # Largest-demand-first: place the customers that are hardest to fit
        # while capacity is still plentiful, same rationale as FFD bin packing.
        order = sorted(range(len(customers)), key=lambda i: customers[i].demand, reverse=True)

        remaining_capacity: Dict[int, float] = {hub.id: hub.capacity for hub in hubs}
        territory_index_by_hub_id: Dict[int, int] = {}
        solution = ClusteringSolution()

        for idx in order:
            customer = customers[idx]

            # Prefer the nearest hub with enough remaining capacity.
            chosen = None
            best_dist = math.inf
            for hub in hubs:
                if remaining_capacity[hub.id] + _CAPACITY_EPS < customer.demand:
                    continue
                d = problem.distance(customer, hub)
                if d < best_dist:
                    best_dist = d
                    chosen = hub

            # Nothing has room: fall back to the nearest hub overall so the
            # solution stays complete; validate() will mark it infeasible.
            if chosen is None:
                for hub in hubs:
                    d = problem.distance(customer, hub)
                    if d < best_dist:
                        best_dist = d
                        chosen = hub

            if chosen is None:
                continue  # no hubs at all -- nothing we can do

            territory_idx = territory_index_by_hub_id.get(chosen.id)
            if territory_idx is None:
                territory_idx = len(solution.territories)
                solution.territories.append(Territory(chosen.id, [], 0.0))
                territory_index_by_hub_id[chosen.id] = territory_idx

            territory = solution.territories[territory_idx]
            territory.customer_ids.append(customer.id)
            territory.used_capacity += customer.demand
            remaining_capacity[chosen.id] -= customer.demand

        problem.validate(solution)
        return solution

    def name(self) -> str:
        return "Greedy-NearestHub"

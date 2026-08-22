"""Port of ClusteringProblem.h / ClusteringProblem.cpp."""

import math
from dataclasses import dataclass, field
from typing import List, Optional

from .customer import Customer
from .hub import Hub

_CAPACITY_EPS = 1e-9


@dataclass
class Territory:
    """One opened hub and the customers assigned to it."""

    hub_id: int = -1
    customer_ids: List[int] = field(default_factory=list)
    used_capacity: float = 0.0


@dataclass
class ClusteringSolution:
    """A candidate clustering/assignment produced by a solver.

    Solvers build one of these, then hand it to
    ClusteringProblem.validate() to check feasibility and price it.
    """

    feasible: bool = True
    total_cost: float = 0.0
    territories: List[Territory] = field(default_factory=list)

    @property
    def hub_count(self) -> int:
        return len(self.territories)


class ClusteringProblem:
    """Capacitated facility-location / territory-assignment problem.

    Decide which candidate hubs to open and assign every customer to
    exactly one open hub, minimizing fixed hub-opening cost plus
    distance-weighted service cost, subject to per-hub capacity.
    """

    def __init__(self, customers: List[Customer], hubs: List[Hub], cost_per_unit_distance: float):
        self._customers = list(customers)
        self._hubs = list(hubs)
        self._cost_per_unit_distance = cost_per_unit_distance

    @property
    def customers(self) -> List[Customer]:
        return self._customers

    @property
    def hubs(self) -> List[Hub]:
        return self._hubs

    @property
    def cost_per_unit_distance(self) -> float:
        return self._cost_per_unit_distance

    def distance(self, customer: Customer, hub: Hub) -> float:
        """Straight-line distance between a customer and a hub."""
        dx = customer.x - hub.x
        dy = customer.y - hub.y
        return math.sqrt(dx * dx + dy * dy)

    def assignment_cost(self, customer: Customer, hub: Hub) -> float:
        """Cost of serving one customer from one hub: distance*demand*rate."""
        return self.distance(customer, hub) * customer.demand * self._cost_per_unit_distance

    def find_hub(self, hub_id: int) -> Optional[Hub]:
        for h in self._hubs:
            if h.id == hub_id:
                return h
        return None

    def find_customer(self, customer_id: int) -> Optional[Customer]:
        for c in self._customers:
            if c.id == customer_id:
                return c
        return None

    def validate(self, solution: ClusteringSolution) -> None:
        """Recompute feasibility and total_cost for a candidate solution.

        Every customer must appear exactly once, every territory's used
        capacity must fit its hub's capacity, and cost is fixed cost (once
        per opened hub) plus per-customer assignment cost. Mutates
        `solution` in place, mirroring the C++ signature.
        """
        times_seen = [0] * len(self._customers)

        def index_of(customer_id: int) -> int:
            for i, c in enumerate(self._customers):
                if c.id == customer_id:
                    return i
            return -1

        cost = 0.0
        feasible = True

        for territory in solution.territories:
            hub = self.find_hub(territory.hub_id)
            if hub is None:
                feasible = False
                continue
            used = 0.0
            for customer_id in territory.customer_ids:
                idx = index_of(customer_id)
                if idx < 0:
                    feasible = False
                    continue
                times_seen[idx] += 1
                used += self._customers[idx].demand
                cost += self.assignment_cost(self._customers[idx], hub)
            if used > hub.capacity + _CAPACITY_EPS:
                feasible = False
            cost += hub.fixed_cost

        for count in times_seen:
            if count != 1:
                feasible = False  # every customer exactly once

        solution.feasible = feasible
        solution.total_cost = cost if feasible else 0.0

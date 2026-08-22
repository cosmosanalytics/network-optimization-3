"""netopt3 -- capacitated facility-location / territory-assignment solver.

Assign customers to territory hubs to minimize total cost (fixed hub
activation cost plus distance- and demand-weighted service cost), subject
to per-hub capacity. Python port of the C++ `network_optimization_3`
project.
"""

from .customer import Customer
from .hub import Hub
from .problem import ClusteringProblem, ClusteringSolution, Territory
from .solver import ClusteringSolver

__all__ = [
    "Customer",
    "Hub",
    "ClusteringProblem",
    "ClusteringSolution",
    "Territory",
    "ClusteringSolver",
]

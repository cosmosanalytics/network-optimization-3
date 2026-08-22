"""Port of Hub.h."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Hub:
    """A candidate territory hub (distribution center).

    Opening a hub incurs fixed_cost once; every customer assigned to it
    must fit within capacity.
    """

    id: int
    name: str
    x: float
    y: float
    capacity: float
    fixed_cost: float = 0.0

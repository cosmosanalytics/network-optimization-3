"""Port of Customer.h."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Customer:
    """A customer (demand point) to be assigned to a territory hub.

    Coordinates are treated as a flat-plane approximation (e.g. projected
    miles/km) -- good enough for a distribution-network design model.
    """

    id: int
    x: float
    y: float
    demand: float

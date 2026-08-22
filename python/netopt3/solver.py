"""Port of ClusteringSolver.h: the Strategy interface for solvers."""

from abc import ABC, abstractmethod

from .problem import ClusteringProblem, ClusteringSolution


class ClusteringSolver(ABC):
    """Strategy interface so callers can swap solver backends freely."""

    @abstractmethod
    def solve(self, problem: ClusteringProblem) -> ClusteringSolution:
        raise NotImplementedError

    @abstractmethod
    def name(self) -> str:
        raise NotImplementedError

"""Port of tests/ClusteringTests.cpp -- every case, same hand-verified numbers."""

import unittest

from netopt3.customer import Customer
from netopt3.exact_solver import BranchAndBoundSolver
from netopt3.greedy_solver import GreedyNearestHubSolver
from netopt3.hub import Hub
from netopt3.problem import ClusteringProblem, ClusteringSolution, Territory

try:
    import pulp  # noqa: F401

    PULP_AVAILABLE = True
except ImportError:
    PULP_AVAILABLE = False


def make_hub(hub_id: int, x: float, y: float, capacity: float, fixed_cost: float = 0.0) -> Hub:
    return Hub(hub_id, f"Hub{hub_id}", x, y, capacity, fixed_cost)


class ClusteringTests(unittest.TestCase):
    def test_feasibility_respects_capacity(self):
        customers = [Customer(1, 0.0, 0.0, 4.0), Customer(2, 1.0, 0.0, 5.0)]
        hubs = [make_hub(1, 0.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solution = ClusteringSolution()
        solution.territories.append(Territory(1, [1, 2], 0.0))
        problem.validate(solution)

        self.assertTrue(solution.feasible)  # 4 + 5 = 9 <= 10

    def test_feasibility_detects_capacity_violation(self):
        customers = [Customer(1, 0.0, 0.0, 6.0), Customer(2, 1.0, 0.0, 7.0)]
        hubs = [make_hub(1, 0.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solution = ClusteringSolution()
        solution.territories.append(Territory(1, [1, 2], 0.0))
        problem.validate(solution)

        self.assertFalse(solution.feasible)  # 6 + 7 = 13 > 10

    def test_feasibility_detects_missing_customer(self):
        customers = [Customer(1, 0.0, 0.0, 1.0), Customer(2, 1.0, 0.0, 1.0)]
        hubs = [make_hub(1, 0.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solution = ClusteringSolution()
        solution.territories.append(Territory(1, [1], 0.0))  # customer 2 never assigned
        problem.validate(solution)

        self.assertFalse(solution.feasible)

    def test_greedy_produces_feasible_solution_when_capacity_allows(self):
        customers = [
            Customer(1, 0.0, 0.0, 3.0), Customer(2, 1.0, 0.0, 4.0),
            Customer(3, 50.0, 0.0, 3.0), Customer(4, 51.0, 0.0, 4.0),
        ]
        hubs = [make_hub(1, 0.0, 0.0, 10.0, 100.0), make_hub(2, 50.0, 0.0, 10.0, 100.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solver = GreedyNearestHubSolver()
        solution = solver.solve(problem)

        self.assertTrue(solution.feasible)
        self.assertEqual(solution.hub_count, 2)  # each cluster is closer to its own hub

    def test_branch_and_bound_achieves_known_optimum(self):
        # Two well-separated pairs, one hub per pair, capacity == demand.
        # Hand-verified optimum: 5+10=15 per side -> 30 total.
        customers = [
            Customer(1, 1.0, 0.0, 5.0), Customer(2, 2.0, 0.0, 5.0),
            Customer(3, 99.0, 0.0, 5.0), Customer(4, 98.0, 0.0, 5.0),
        ]
        hubs = [make_hub(1, 0.0, 0.0, 10.0), make_hub(2, 100.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solver = BranchAndBoundSolver()
        solution = solver.solve(problem)

        self.assertTrue(solution.feasible)
        self.assertEqual(solution.hub_count, 2)
        self.assertTrue(29.999 < solution.total_cost < 30.001)

    def test_branch_and_bound_never_worse_than_greedy(self):
        # Demand (15) exceeds the near hub's capacity (8), forcing some
        # customers onto a distant hub -- greedy's myopic choice may not
        # be optimal; exact must do at least as well.
        customers = [
            Customer(1, 1.0, 0.0, 5.0), Customer(2, 2.0, 0.0, 5.0), Customer(3, 3.0, 0.0, 5.0),
        ]
        hubs = [make_hub(1, 0.0, 0.0, 8.0, 0.0), make_hub(2, 50.0, 0.0, 100.0, 1000.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        greedy = GreedyNearestHubSolver()
        exact = BranchAndBoundSolver()
        greedy_solution = greedy.solve(problem)
        exact_solution = exact.solve(problem)

        self.assertTrue(greedy_solution.feasible)
        self.assertTrue(exact_solution.feasible)
        self.assertLessEqual(exact_solution.total_cost, greedy_solution.total_cost + 1e-9)

    def test_edge_case_zero_customers(self):
        customers = []
        hubs = [make_hub(1, 0.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        greedy = GreedyNearestHubSolver()
        solution = greedy.solve(problem)

        self.assertTrue(solution.feasible)
        self.assertEqual(solution.hub_count, 0)
        self.assertEqual(solution.total_cost, 0.0)

    def test_edge_case_exact_capacity_fit(self):
        customers = [Customer(1, 0.0, 0.0, 10.0)]
        hubs = [make_hub(1, 0.0, 0.0, 10.0, 250.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        solution = ClusteringSolution()
        solution.territories.append(Territory(1, [1], 0.0))
        problem.validate(solution)

        self.assertTrue(solution.feasible)  # exactly at capacity should still fit
        self.assertTrue(249.999 < solution.total_cost < 250.001)  # 0 distance + fixed cost

    def test_edge_case_infeasible_when_demand_exceeds_all_capacity(self):
        customers = [Customer(1, 0.0, 0.0, 20.0)]
        hubs = [make_hub(1, 0.0, 0.0, 10.0)]  # only 10 units of capacity exist anywhere
        problem = ClusteringProblem(customers, hubs, 1.0)

        greedy = GreedyNearestHubSolver()
        exact = BranchAndBoundSolver()

        self.assertFalse(greedy.solve(problem).feasible)
        self.assertFalse(exact.solve(problem).feasible)

    @unittest.skipUnless(PULP_AVAILABLE, "pulp not installed")
    def test_pulp_solver_matches_exact_solver_cost(self):
        from netopt3.pulp_solver import PulpMipSolver

        customers = [
            Customer(1, 1.0, 0.0, 5.0), Customer(2, 2.0, 0.0, 5.0),
            Customer(3, 99.0, 0.0, 5.0), Customer(4, 98.0, 0.0, 5.0),
        ]
        hubs = [make_hub(1, 0.0, 0.0, 10.0), make_hub(2, 100.0, 0.0, 10.0)]
        problem = ClusteringProblem(customers, hubs, 1.0)

        exact_solution = BranchAndBoundSolver().solve(problem)
        pulp_solution = PulpMipSolver().solve(problem)

        self.assertTrue(pulp_solution.feasible)
        self.assertAlmostEqual(pulp_solution.total_cost, exact_solution.total_cost, places=6)


if __name__ == "__main__":
    unittest.main()

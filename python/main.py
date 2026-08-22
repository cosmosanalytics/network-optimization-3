"""Demo: build a sample instance, run greedy + exact (+ pulp, if available) solvers."""

from netopt3.customer import Customer
from netopt3.exact_solver import BranchAndBoundSolver
from netopt3.greedy_solver import GreedyNearestHubSolver
from netopt3.hub import Hub
from netopt3.problem import ClusteringProblem, ClusteringSolution


def print_solution(solver_name: str, problem: ClusteringProblem, solution: ClusteringSolution) -> None:
    print(f"\n--- {solver_name} ---")
    print(f"Feasible: {'yes' if solution.feasible else 'no'}")
    print(f"Hubs opened: {solution.hub_count}")
    print(f"Total cost: ${solution.total_cost:,.2f}")
    for territory in solution.territories:
        hub = problem.find_hub(territory.hub_id)
        hub_name = hub.name if hub else "?"
        customer_list = ", ".join(str(cid) for cid in territory.customer_ids)
        print(f"  {hub_name} (load {territory.used_capacity}): customers {{{customer_list}}}")


def main() -> None:
    # A representative regional customer base and candidate hub network:
    # demand in shipments/week, coordinates in projected miles.
    customers = [
        Customer(1, 5.0, 5.0, 12.0),
        Customer(2, 6.0, 4.0, 8.0),
        Customer(3, 40.0, 42.0, 15.0),
        Customer(4, 41.0, 40.0, 9.0),
        Customer(5, 8.0, 60.0, 11.0),
        Customer(6, 10.0, 58.0, 7.0),
        Customer(7, 45.0, 8.0, 14.0),
        Customer(8, 47.0, 10.0, 6.0),
        Customer(9, 25.0, 25.0, 10.0),
        Customer(10, 4.0, 45.0, 13.0),
    ]
    hubs = [
        Hub(1, "North DC", 5.0, 55.0, 40.0, 5000.0),
        Hub(2, "West DC", 6.0, 5.0, 40.0, 5000.0),
        Hub(3, "East DC", 44.0, 41.0, 40.0, 5500.0),
        Hub(4, "South DC", 46.0, 9.0, 40.0, 4800.0),
    ]

    problem = ClusteringProblem(customers, hubs, cost_per_unit_distance=12.0)

    print("Network Optimization 3 (Python) -- Customer Clustering / Territory Assignment MIP")
    print(f"{len(customers)} customers, {len(hubs)} candidate hubs, $12.00/unit-distance-demand")

    greedy = GreedyNearestHubSolver()
    print_solution(greedy.name(), problem, greedy.solve(problem))

    exact = BranchAndBoundSolver()
    print_solution(exact.name() + " (exact)", problem, exact.solve(problem))

    try:
        import pulp  # noqa: F401
        from netopt3.pulp_solver import PulpMipSolver

        pulp_solver = PulpMipSolver()
        print_solution(pulp_solver.name(), problem, pulp_solver.solve(problem))
    except ImportError:
        print("\n--- Pulp-CBC-MIP ---\nSkipped: pulp is not installed (pip install -r requirements.txt).")


if __name__ == "__main__":
    main()

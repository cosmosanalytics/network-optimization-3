"""PuLP + CBC MIP solver mirroring CbcMipSolver.h's formulation.

Production-scale path (the CBC-backed counterpart of BranchAndBoundSolver).
pulp is imported lazily inside solve() so importing the netopt3 package
never requires it to be installed -- run `pip install -r requirements.txt`
to enable this solver.
"""

from .problem import ClusteringProblem, ClusteringSolution, Territory
from .solver import ClusteringSolver


class PulpMipSolver(ClusteringSolver):
    """Solves ClusteringProblem as the capacitated facility-location MIP
    described in the README, via PuLP's CBC backend.

    Variables: x_ij in {0,1} (customer i served by hub j), y_j in {0,1}
    (hub j opened).
    """

    def solve(self, problem: ClusteringProblem) -> ClusteringSolution:
        try:
            import pulp
        except ImportError as exc:
            raise RuntimeError(
                "pulp is required for PulpMipSolver -- install it with "
                "`pip install -r requirements.txt` (pulp>=2.7)."
            ) from exc

        customers = problem.customers
        hubs = problem.hubs

        model = pulp.LpProblem("netopt3_clustering", pulp.LpMinimize)

        # x[i][j] = 1 if customer i is served by hub j.
        x = {
            (i, j): pulp.LpVariable(f"x_{i}_{j}", cat="Binary")
            for i in range(len(customers))
            for j in range(len(hubs))
        }
        # y[j] = 1 if hub j is opened.
        y = {j: pulp.LpVariable(f"y_{j}", cat="Binary") for j in range(len(hubs))}

        # Objective: sum_ij cost_ij * x_ij + sum_j fixedCost_j * y_j.
        model += pulp.lpSum(
            problem.assignment_cost(customers[i], hubs[j]) * x[(i, j)]
            for i in range(len(customers))
            for j in range(len(hubs))
        ) + pulp.lpSum(hubs[j].fixed_cost * y[j] for j in range(len(hubs)))

        # sum_j x_ij = 1 for every customer i.
        for i in range(len(customers)):
            model += pulp.lpSum(x[(i, j)] for j in range(len(hubs))) == 1

        # sum_i demand_i * x_ij <= capacity_j * y_j for every hub j.
        for j in range(len(hubs)):
            model += (
                pulp.lpSum(customers[i].demand * x[(i, j)] for i in range(len(customers)))
                <= hubs[j].capacity * y[j]
            )

        model.solve(pulp.PULP_CBC_CMD(msg=False))

        result = ClusteringSolution()
        territory_index_by_hub: dict = {}
        for i, customer in enumerate(customers):
            for j, hub in enumerate(hubs):
                val = pulp.value(x[(i, j)])
                if val is not None and val > 0.5:
                    idx = territory_index_by_hub.get(j)
                    if idx is None:
                        idx = len(result.territories)
                        result.territories.append(Territory(hub.id, [], 0.0))
                        territory_index_by_hub[j] = idx
                    territory = result.territories[idx]
                    territory.customer_ids.append(customer.id)
                    territory.used_capacity += customer.demand

        problem.validate(result)
        return result

    def name(self) -> str:
        return "Pulp-CBC-MIP"

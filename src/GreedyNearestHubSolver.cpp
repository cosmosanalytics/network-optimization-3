#include "GreedyNearestHubSolver.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace netopt3 {

ClusteringSolution GreedyNearestHubSolver::solve(const ClusteringProblem& problem) {
    const auto& customers = problem.customers();
    const auto& hubs = problem.hubs();

    // Largest-demand-first: place the customers that are hardest to fit
    // while capacity is still plentiful, same rationale as FFD bin packing.
    std::vector<int> order(customers.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return customers[a].demand() > customers[b].demand();
    });

    std::unordered_map<int, double> remainingCapacity;
    std::unordered_map<int, int> territoryIndexByHubId;
    ClusteringSolution solution;
    for (const auto& hub : hubs) remainingCapacity[hub.id()] = hub.capacity();

    for (int idx : order) {
        const Customer& customer = customers[idx];

        // Prefer the nearest hub with enough remaining capacity.
        const Hub* chosen = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (const auto& hub : hubs) {
            if (remainingCapacity[hub.id()] + 1e-9 < customer.demand()) continue;
            const double d = problem.distance(customer, hub);
            if (d < bestDist) {
                bestDist = d;
                chosen = &hub;
            }
        }
        // Nothing has room: fall back to the nearest hub overall so the
        // solution stays complete; validate() will mark it infeasible.
        if (!chosen) {
            for (const auto& hub : hubs) {
                const double d = problem.distance(customer, hub);
                if (d < bestDist) {
                    bestDist = d;
                    chosen = &hub;
                }
            }
        }
        if (!chosen) continue; // no hubs at all -- nothing we can do

        auto it = territoryIndexByHubId.find(chosen->id());
        int territoryIdx;
        if (it == territoryIndexByHubId.end()) {
            territoryIdx = static_cast<int>(solution.territories.size());
            solution.territories.push_back(Territory{chosen->id(), {}, 0.0});
            territoryIndexByHubId[chosen->id()] = territoryIdx;
        } else {
            territoryIdx = it->second;
        }
        Territory& territory = solution.territories[territoryIdx];
        territory.customerIds.push_back(customer.id());
        territory.usedCapacity += customer.demand();
        remainingCapacity[chosen->id()] -= customer.demand();
    }

    problem.validate(solution);
    return solution;
}

} // namespace netopt3

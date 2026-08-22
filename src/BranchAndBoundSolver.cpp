#include "BranchAndBoundSolver.h"

#include <algorithm>
#include <limits>
#include <vector>

#include "GreedyNearestHubSolver.h"

namespace netopt3 {

namespace {

struct SearchState {
    const ClusteringProblem* problem;
    const std::vector<Customer>* customers;
    const std::vector<Hub>* hubs;
    std::vector<double> cheapestPossibleCost; // per customer index, min over all hubs
    std::vector<double> suffixLowerBound;     // suffixLowerBound[i] = sum of cheapestPossibleCost[i..n-1]

    std::vector<int> assignment;     // assignment[i] = hub index chosen for customer i
    std::vector<double> remaining;   // remaining[j] = capacity left at hub j
    std::vector<bool> opened;        // opened[j] = has hub j been used yet

    std::vector<int> bestAssignment;
    double bestCost = std::numeric_limits<double>::max();
};

void search(SearchState& s, int customerIdx, double runningCost) {
    const std::size_t n = s.customers->size();
    if (static_cast<std::size_t>(customerIdx) == n) {
        if (runningCost < s.bestCost) {
            s.bestCost = runningCost;
            s.bestAssignment = s.assignment;
        }
        return;
    }

    if (runningCost + s.suffixLowerBound[customerIdx] >= s.bestCost) {
        return; // even the most optimistic completion can't beat the incumbent
    }

    const Customer& customer = (*s.customers)[customerIdx];

    // Try nearer hubs first so a good incumbent is found early, which
    // makes the lower-bound prune above effective sooner.
    std::vector<int> hubOrder(s.hubs->size());
    for (std::size_t j = 0; j < hubOrder.size(); ++j) hubOrder[j] = static_cast<int>(j);
    std::sort(hubOrder.begin(), hubOrder.end(), [&](int a, int b) {
        return s.problem->distance(customer, (*s.hubs)[a]) <
               s.problem->distance(customer, (*s.hubs)[b]);
    });

    for (int j : hubOrder) {
        const Hub& hub = (*s.hubs)[j];
        if (s.remaining[j] + 1e-9 < customer.demand()) continue;

        const double addedFixed = s.opened[j] ? 0.0 : hub.fixedCost();
        const double addedAssignment = s.problem->assignmentCost(customer, hub);
        const bool wasOpened = s.opened[j];

        s.assignment[customerIdx] = j;
        s.remaining[j] -= customer.demand();
        s.opened[j] = true;

        search(s, customerIdx + 1, runningCost + addedAssignment + addedFixed);

        s.remaining[j] += customer.demand();
        s.opened[j] = wasOpened;
    }
    s.assignment[customerIdx] = -1;
}

} // namespace

ClusteringSolution BranchAndBoundSolver::solve(const ClusteringProblem& problem) {
    const auto& customers = problem.customers();
    const auto& hubs = problem.hubs();

    ClusteringSolution result;
    if (customers.empty()) {
        result.feasible = true;
        result.totalCost = 0.0;
        return result;
    }

    SearchState s;
    s.problem = &problem;
    s.customers = &customers;
    s.hubs = &hubs;
    s.assignment.assign(customers.size(), -1);
    s.remaining.resize(hubs.size());
    for (std::size_t j = 0; j < hubs.size(); ++j) s.remaining[j] = hubs[j].capacity();
    s.opened.assign(hubs.size(), false);

    // Admissible lower bound: cheapest single-hub distance cost per
    // customer, ignoring capacity and fixed cost.
    s.cheapestPossibleCost.resize(customers.size());
    for (std::size_t i = 0; i < customers.size(); ++i) {
        double best = std::numeric_limits<double>::max();
        for (const auto& hub : hubs) {
            best = std::min(best, problem.assignmentCost(customers[i], hub));
        }
        s.cheapestPossibleCost[i] = hubs.empty() ? 0.0 : best;
    }
    s.suffixLowerBound.assign(customers.size() + 1, 0.0);
    for (std::size_t i = customers.size(); i-- > 0;) {
        s.suffixLowerBound[i] = s.suffixLowerBound[i + 1] + s.cheapestPossibleCost[i];
    }

    // Seed the incumbent with the greedy heuristic's result so pruning is
    // effective from the very first branch instead of only after the
    // search stumbles onto a decent solution by luck.
    GreedyNearestHubSolver greedy;
    ClusteringSolution greedySolution = greedy.solve(problem);
    if (greedySolution.feasible) s.bestCost = greedySolution.totalCost;

    if (!hubs.empty()) {
        search(s, 0, 0.0);
    }

    if (s.bestAssignment.empty() && !greedySolution.feasible) {
        // No feasible assignment exists at all (e.g. total demand exceeds
        // total capacity) -- report the infeasible greedy attempt so the
        // caller can see how far off it was, same as Network Optimization 2.
        return greedySolution;
    }
    if (s.bestAssignment.empty()) {
        return greedySolution; // greedy already found the (only) feasible answer
    }

    std::vector<int> territoryIndexByHubIdx(hubs.size(), -1);
    for (std::size_t i = 0; i < customers.size(); ++i) {
        const int hubIdx = s.bestAssignment[i];
        if (territoryIndexByHubIdx[hubIdx] == -1) {
            territoryIndexByHubIdx[hubIdx] = static_cast<int>(result.territories.size());
            result.territories.push_back(Territory{hubs[hubIdx].id(), {}, 0.0});
        }
        Territory& territory = result.territories[territoryIndexByHubIdx[hubIdx]];
        territory.customerIds.push_back(customers[i].id());
        territory.usedCapacity += customers[i].demand();
    }

    problem.validate(result);
    return result;
}

} // namespace netopt3

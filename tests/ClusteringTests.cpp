#include <vector>

#include "BranchAndBoundSolver.h"
#include "ClusteringProblem.h"
#include "GreedyNearestHubSolver.h"
#include "TestFramework.h"

using namespace netopt3;

namespace {

Hub makeHub(int id, double x, double y, double capacity, double fixedCost = 0.0) {
    return Hub(id, "Hub" + std::to_string(id), x, y, capacity, fixedCost);
}

} // namespace

TEST(Feasibility_RespectsCapacity) {
    std::vector<Customer> customers = {Customer(1, 0.0, 0.0, 4.0), Customer(2, 1.0, 0.0, 5.0)};
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    ClusteringSolution solution;
    solution.territories.push_back(Territory{1, {1, 2}, 0.0});
    problem.validate(solution);

    CHECK(solution.feasible); // 4 + 5 = 9 <= 10
}

TEST(Feasibility_DetectsCapacityViolation) {
    std::vector<Customer> customers = {Customer(1, 0.0, 0.0, 6.0), Customer(2, 1.0, 0.0, 7.0)};
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    ClusteringSolution solution;
    solution.territories.push_back(Territory{1, {1, 2}, 0.0});
    problem.validate(solution);

    CHECK(!solution.feasible); // 6 + 7 = 13 > 10
}

TEST(Feasibility_DetectsMissingCustomer) {
    std::vector<Customer> customers = {Customer(1, 0.0, 0.0, 1.0), Customer(2, 1.0, 0.0, 1.0)};
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    ClusteringSolution solution;
    solution.territories.push_back(Territory{1, {1}, 0.0}); // customer 2 never assigned
    problem.validate(solution);

    CHECK(!solution.feasible);
}

TEST(Greedy_ProducesFeasibleSolutionWhenCapacityAllows) {
    std::vector<Customer> customers = {
        Customer(1, 0.0, 0.0, 3.0),  Customer(2, 1.0, 0.0, 4.0), Customer(3, 50.0, 0.0, 3.0),
        Customer(4, 51.0, 0.0, 4.0),
    };
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0, 100.0), makeHub(2, 50.0, 0.0, 10.0, 100.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    GreedyNearestHubSolver solver;
    ClusteringSolution solution = solver.solve(problem);

    CHECK(solution.feasible);
    CHECK(solution.hubCount() == 2); // each cluster is closer to its own hub
}

TEST(BranchAndBound_AchievesKnownOptimum) {
    // Two well-separated pairs of customers, one hub next to each pair,
    // capacity exactly matching demand -- the optimal assignment (each
    // pair to its own hub) is hand-verifiable: 5+10 = 15 per side = 30.
    std::vector<Customer> customers = {
        Customer(1, 1.0, 0.0, 5.0), Customer(2, 2.0, 0.0, 5.0),
        Customer(3, 99.0, 0.0, 5.0), Customer(4, 98.0, 0.0, 5.0),
    };
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0), makeHub(2, 100.0, 0.0, 10.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    BranchAndBoundSolver solver;
    ClusteringSolution solution = solver.solve(problem);

    CHECK(solution.feasible);
    CHECK(solution.hubCount() == 2);
    CHECK(solution.totalCost > 29.999 && solution.totalCost < 30.001);
}

TEST(BranchAndBound_NeverWorseThanGreedy) {
    // Total demand (15) exceeds the near hub's capacity (8), forcing some
    // customers onto a distant, expensive-to-open hub. Greedy's myopic
    // nearest-hub choice has no reason to be optimal here; the exact
    // solver must do at least as well.
    std::vector<Customer> customers = {
        Customer(1, 1.0, 0.0, 5.0), Customer(2, 2.0, 0.0, 5.0), Customer(3, 3.0, 0.0, 5.0),
    };
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 8.0, 0.0), makeHub(2, 50.0, 0.0, 100.0, 1000.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    GreedyNearestHubSolver greedy;
    BranchAndBoundSolver exact;
    ClusteringSolution greedySolution = greedy.solve(problem);
    ClusteringSolution exactSolution = exact.solve(problem);

    CHECK(greedySolution.feasible);
    CHECK(exactSolution.feasible);
    CHECK(exactSolution.totalCost <= greedySolution.totalCost + 1e-9);
}

TEST(EdgeCase_ZeroCustomers) {
    std::vector<Customer> customers; // empty
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    GreedyNearestHubSolver greedy;
    ClusteringSolution solution = greedy.solve(problem);

    CHECK(solution.feasible);
    CHECK(solution.hubCount() == 0);
    CHECK(solution.totalCost == 0.0);
}

TEST(EdgeCase_ExactCapacityFit) {
    std::vector<Customer> customers = {Customer(1, 0.0, 0.0, 10.0)};
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0, 250.0)};
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    ClusteringSolution solution;
    solution.territories.push_back(Territory{1, {1}, 0.0});
    problem.validate(solution);

    CHECK(solution.feasible); // exactly at capacity should still fit
    CHECK(solution.totalCost > 249.999 && solution.totalCost < 250.001); // 0 distance + fixed cost
}

TEST(EdgeCase_InfeasibleWhenDemandExceedsAllCapacity) {
    std::vector<Customer> customers = {Customer(1, 0.0, 0.0, 20.0)};
    std::vector<Hub> hubs = {makeHub(1, 0.0, 0.0, 10.0)}; // only 10 units of capacity exist anywhere
    ClusteringProblem problem(customers, hubs, /*rate=*/1.0);

    GreedyNearestHubSolver greedy;
    BranchAndBoundSolver exact;

    CHECK(!greedy.solve(problem).feasible);
    CHECK(!exact.solve(problem).feasible);
}

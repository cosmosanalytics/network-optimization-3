#include <iomanip>
#include <iostream>
#include <vector>

#include "BranchAndBoundSolver.h"
#include "ClusteringProblem.h"
#include "GreedyNearestHubSolver.h"

using namespace netopt3;

namespace {

void printSolution(const std::string& solverName, const ClusteringProblem& problem,
                    const ClusteringSolution& solution) {
    std::cout << "\n--- " << solverName << " ---\n";
    std::cout << "Feasible: " << (solution.feasible ? "yes" : "no") << "\n";
    std::cout << "Hubs opened: " << solution.hubCount() << "\n";
    std::cout << "Total cost: $" << std::fixed << std::setprecision(2) << solution.totalCost
              << "\n";
    for (const Territory& territory : solution.territories) {
        const Hub* hub = problem.findHub(territory.hubId);
        std::cout << "  " << (hub ? hub->name() : "?") << " (load " << territory.usedCapacity
                  << "): customers {";
        for (std::size_t i = 0; i < territory.customerIds.size(); ++i) {
            std::cout << territory.customerIds[i]
                       << (i + 1 < territory.customerIds.size() ? ", " : "");
        }
        std::cout << "}\n";
    }
}

} // namespace

int main() {
    // A representative regional customer base and candidate hub network:
    // demand in shipments/week, coordinates in projected miles.
    std::vector<Customer> customers = {
        Customer(1, 5.0, 5.0, 12.0),   Customer(2, 6.0, 4.0, 8.0),
        Customer(3, 40.0, 42.0, 15.0), Customer(4, 41.0, 40.0, 9.0),
        Customer(5, 8.0, 60.0, 11.0),  Customer(6, 10.0, 58.0, 7.0),
        Customer(7, 45.0, 8.0, 14.0),  Customer(8, 47.0, 10.0, 6.0),
        Customer(9, 25.0, 25.0, 10.0), Customer(10, 4.0, 45.0, 13.0),
    };
    std::vector<Hub> hubs = {
        Hub(1, "North DC", 5.0, 55.0, 40.0, 5000.0),
        Hub(2, "West DC", 6.0, 5.0, 40.0, 5000.0),
        Hub(3, "East DC", 44.0, 41.0, 40.0, 5500.0),
        Hub(4, "South DC", 46.0, 9.0, 40.0, 4800.0),
    };

    ClusteringProblem problem(customers, hubs, /*costPerUnitDistance=*/12.0);

    std::cout << "Network Optimization 3 (C++) — Customer Clustering / Territory Assignment MIP\n";
    std::cout << customers.size() << " customers, " << hubs.size()
              << " candidate hubs, $12.00/unit-distance-demand\n";

    GreedyNearestHubSolver greedy;
    printSolution(greedy.name(), problem, greedy.solve(problem));

    BranchAndBoundSolver exact;
    printSolution(exact.name() + " (exact)", problem, exact.solve(problem));

    return 0;
}

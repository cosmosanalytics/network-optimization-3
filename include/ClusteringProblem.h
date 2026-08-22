#pragma once

#include <vector>

#include "Customer.h"
#include "Hub.h"

namespace netopt3 {

// One opened hub and the customers assigned to it (a "territory").
struct Territory {
    int hubId = -1;
    std::vector<int> customerIds;
    double usedCapacity = 0.0;
};

// A candidate clustering/assignment produced by a solver. Mirrors the
// Bin/PackingSolution shape from Network Optimization 2: solvers build one
// of these, then hand it to ClusteringProblem::validate() to check
// feasibility and price it.
struct ClusteringSolution {
    bool feasible = true;
    double totalCost = 0.0;
    std::vector<Territory> territories; // one entry per opened hub (non-empty)

    int hubCount() const { return static_cast<int>(territories.size()); }
};

// A capacitated facility-location / territory-assignment problem: decide
// which candidate hubs to open and assign every customer to exactly one
// open hub, minimizing fixed hub-opening cost plus distance-weighted
// service cost, subject to per-hub capacity.
class ClusteringProblem {
public:
    ClusteringProblem(std::vector<Customer> customers, std::vector<Hub> hubs,
                       double costPerUnitDistance);

    const std::vector<Customer>& customers() const { return customers_; }
    const std::vector<Hub>& hubs() const { return hubs_; }
    double costPerUnitDistance() const { return costPerUnitDistance_; }

    // Straight-line distance between a customer and a hub.
    double distance(const Customer& customer, const Hub& hub) const;

    // Cost of serving one customer from one hub: distance * demand * rate.
    // Demand-weighting means heavier customers further away cost more to
    // serve, which is what actually drives territory design in practice.
    double assignmentCost(const Customer& customer, const Hub& hub) const;

    const Hub* findHub(int hubId) const;
    const Customer* findCustomer(int customerId) const;

    // Recomputes feasibility and totalCost for a solution built by a
    // solver: every customer must appear exactly once, every territory's
    // used capacity must fit its hub's capacity, and cost is fixed cost
    // (once per opened hub) plus per-customer assignment cost.
    void validate(ClusteringSolution& solution) const;

private:
    std::vector<Customer> customers_;
    std::vector<Hub> hubs_;
    double costPerUnitDistance_;
};

} // namespace netopt3

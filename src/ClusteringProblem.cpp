#include "ClusteringProblem.h"

#include <cmath>

namespace netopt3 {

ClusteringProblem::ClusteringProblem(std::vector<Customer> customers, std::vector<Hub> hubs,
                                      double costPerUnitDistance)
    : customers_(std::move(customers)), hubs_(std::move(hubs)),
      costPerUnitDistance_(costPerUnitDistance) {}

double ClusteringProblem::distance(const Customer& customer, const Hub& hub) const {
    const double dx = customer.x() - hub.x();
    const double dy = customer.y() - hub.y();
    return std::sqrt(dx * dx + dy * dy);
}

double ClusteringProblem::assignmentCost(const Customer& customer, const Hub& hub) const {
    return distance(customer, hub) * customer.demand() * costPerUnitDistance_;
}

const Hub* ClusteringProblem::findHub(int hubId) const {
    for (const auto& h : hubs_) {
        if (h.id() == hubId) return &h;
    }
    return nullptr;
}

const Customer* ClusteringProblem::findCustomer(int customerId) const {
    for (const auto& c : customers_) {
        if (c.id() == customerId) return &c;
    }
    return nullptr;
}

void ClusteringProblem::validate(ClusteringSolution& solution) const {
    std::vector<int> timesSeen(customers_.size(), 0);
    auto indexOf = [this](int customerId) -> int {
        for (std::size_t i = 0; i < customers_.size(); ++i) {
            if (customers_[i].id() == customerId) return static_cast<int>(i);
        }
        return -1;
    };

    double cost = 0.0;
    bool feasible = true;

    for (const Territory& territory : solution.territories) {
        const Hub* hub = findHub(territory.hubId);
        if (!hub) {
            feasible = false;
            continue;
        }
        double used = 0.0;
        for (int customerId : territory.customerIds) {
            const int idx = indexOf(customerId);
            if (idx < 0) {
                feasible = false;
                continue;
            }
            ++timesSeen[idx];
            used += customers_[idx].demand();
            cost += assignmentCost(customers_[idx], *hub);
        }
        if (used > hub->capacity() + 1e-9) feasible = false;
        cost += hub->fixedCost();
    }

    for (int count : timesSeen) {
        if (count != 1) feasible = false; // every customer exactly once
    }

    solution.feasible = feasible;
    solution.totalCost = feasible ? cost : 0.0;
}

} // namespace netopt3

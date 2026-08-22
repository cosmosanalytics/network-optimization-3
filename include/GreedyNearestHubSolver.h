#pragma once

#include "ClusteringSolver.h"

namespace netopt3 {

// Fast heuristic: process customers largest-demand-first (the facility-
// location analogue of Network Optimization 2's first-fit-decreasing bin
// packer), assigning each to its nearest hub that still has capacity,
// opening hubs on demand. Falls back to the nearest hub regardless of
// capacity if nothing fits, so the caller always gets a solution to
// inspect -- ClusteringProblem::validate() is what actually flags
// infeasibility.
class GreedyNearestHubSolver : public ClusteringSolver {
public:
    ClusteringSolution solve(const ClusteringProblem& problem) override;
    std::string name() const override { return "Greedy-NearestHub"; }
};

} // namespace netopt3

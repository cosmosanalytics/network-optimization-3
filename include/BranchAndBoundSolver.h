#pragma once

#include "ClusteringSolver.h"

namespace netopt3 {

// Exact, dependency-free solver for the capacitated facility-location /
// territory-assignment problem. Branches over which hub each customer is
// assigned to, tracking per-hub remaining capacity and which hubs have
// already been "opened" (so a hub's fixed cost is only charged once).
// Prunes with an admissible lower bound: the cheapest possible per-hub
// distance cost for every not-yet-assigned customer, ignoring capacity and
// fixed cost (both of which can only make the true remaining cost higher).
//
// Exponential in the worst case, like any exact facility-location solver,
// so it is meant for the small/medium instances a portfolio demo or a
// regression test uses -- CbcMipSolver.h is the path for production scale.
class BranchAndBoundSolver : public ClusteringSolver {
public:
    ClusteringSolution solve(const ClusteringProblem& problem) override;
    std::string name() const override { return "BranchAndBound-Exact"; }
};

} // namespace netopt3

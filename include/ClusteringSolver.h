#pragma once

#include <string>

#include "ClusteringProblem.h"

namespace netopt3 {

// Strategy interface for territory-assignment solvers, so main.cpp and the
// tests can swap heuristic/exact/production backends without caring which
// one they're driving.
class ClusteringSolver {
public:
    virtual ~ClusteringSolver() = default;
    virtual ClusteringSolution solve(const ClusteringProblem& problem) = 0;
    virtual std::string name() const = 0;
};

} // namespace netopt3

#pragma once

// CbcMipSolver -- production-scale path using COIN-OR CBC's C++ API.
// BranchAndBoundSolver.h is the zero-dependency exact solver used by
// default; this documents how the same model maps onto CBC (the solver
// the Python version of this project actually uses via COIN-OR/CBC).
//
// Compiled only when NETOPT3_USE_CBC is defined (CMakeLists.txt's
// USE_CBC option), since it needs the COIN-OR CBC dev libraries:
//   sudo apt-get install coinor-libcbc-dev coinor-libclp-dev \
//                         coinor-libosi-dev coinor-libcoinutils-dev
//   cmake -DUSE_CBC=ON -B build && cmake --build build

#ifdef NETOPT3_USE_CBC

#include <algorithm>
#include <limits>

#include <CbcModel.hpp>
#include <CoinPackedMatrix.hpp>
#include <OsiClpSolverInterface.hpp>

#include "ClusteringSolver.h"

namespace netopt3 {

// Solves ClusteringProblem as the capacitated facility-location MIP
// described in the README, via CBC's Open Solver Interface (OSI).
// Variables: x_ij in {0,1} (customer i served by hub j), y_j in {0,1}
// (hub j opened).
class CbcMipSolver : public ClusteringSolver {
public:
    ClusteringSolution solve(const ClusteringProblem& problem) override {
        const auto& customers = problem.customers();
        const auto& hubs = problem.hubs();
        const int n = static_cast<int>(customers.size());
        const int m = static_cast<int>(hubs.size());

        const int numXVars = n * m;
        const int numVars = numXVars + m;
        auto xIndex = [m](int i, int j) { return i * m + j; };

        OsiClpSolverInterface solver;

        std::vector<double> objective(numVars, 0.0);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                objective[xIndex(i, j)] = problem.assignmentCost(customers[i], hubs[j]);
            }
        }
        for (int j = 0; j < m; ++j) objective[numXVars + j] = hubs[j].fixedCost();

        std::vector<double> colLower(numVars, 0.0);
        std::vector<double> colUpper(numVars, 1.0);

        CoinPackedMatrix matrix(false, 0, 0);
        matrix.setDimensions(0, numVars);

        std::vector<double> rowLower;
        std::vector<double> rowUpper;

        // sum_j x_ij = 1 for every customer i.
        for (int i = 0; i < n; ++i) {
            CoinPackedVector row;
            for (int j = 0; j < m; ++j) row.insert(xIndex(i, j), 1.0);
            matrix.appendRow(row);
            rowLower.push_back(1.0);
            rowUpper.push_back(1.0);
        }

        // sum_i demand_i*x_ij - capacity_j*y_j <= 0 for every hub j.
        for (int j = 0; j < m; ++j) {
            CoinPackedVector row;
            for (int i = 0; i < n; ++i) row.insert(xIndex(i, j), customers[i].demand());
            row.insert(numXVars + j, -hubs[j].capacity());
            matrix.appendRow(row);
            rowLower.push_back(-COIN_DBL_MAX);
            rowUpper.push_back(0.0);
        }

        solver.loadProblem(matrix, colLower.data(), colUpper.data(),
                            objective.data(), rowLower.data(), rowUpper.data());
        for (int v = 0; v < numVars; ++v) solver.setInteger(v);

        CbcModel model(solver);
        model.setLogLevel(0);
        model.branchAndBound();

        ClusteringSolution result;
        const double* sol = model.solver()->getColSolution();
        std::vector<int> territoryIndexByHub(m, -1);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (sol[xIndex(i, j)] > 0.5) {
                    if (territoryIndexByHub[j] == -1) {
                        territoryIndexByHub[j] = static_cast<int>(result.territories.size());
                        result.territories.push_back(Territory{hubs[j].id(), {}, 0.0});
                    }
                    Territory& territory = result.territories[territoryIndexByHub[j]];
                    territory.customerIds.push_back(customers[i].id());
                    territory.usedCapacity += customers[i].demand();
                }
            }
        }
        problem.validate(result);
        return result;
    }

    std::string name() const override { return "Cbc-MIP"; }
};

} // namespace netopt3

#endif // NETOPT3_USE_CBC

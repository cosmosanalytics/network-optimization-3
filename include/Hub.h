#pragma once

#include <string>

namespace netopt3 {

// A candidate territory hub (distribution center). Opening a hub incurs
// fixedCost once; every customer assigned to it must fit within capacity.
class Hub {
public:
    Hub(int id, std::string name, double x, double y, double capacity, double fixedCost)
        : id_(id), name_(std::move(name)), x_(x), y_(y), capacity_(capacity),
          fixedCost_(fixedCost) {}

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    double x() const { return x_; }
    double y() const { return y_; }
    double capacity() const { return capacity_; }
    double fixedCost() const { return fixedCost_; }

private:
    int id_;
    std::string name_;
    double x_;
    double y_;
    double capacity_;
    double fixedCost_;
};

} // namespace netopt3

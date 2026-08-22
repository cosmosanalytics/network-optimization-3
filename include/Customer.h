#pragma once

namespace netopt3 {

// A customer (demand point) to be assigned to a territory hub. Coordinates
// are treated as a flat-plane approximation (e.g. projected miles/km) --
// good enough for a distribution-network design model.
class Customer {
public:
    Customer(int id, double x, double y, double demand)
        : id_(id), x_(x), y_(y), demand_(demand) {}

    int id() const { return id_; }
    double x() const { return x_; }
    double y() const { return y_; }
    double demand() const { return demand_; }

private:
    int id_;
    double x_;
    double y_;
    double demand_;
};

} // namespace netopt3

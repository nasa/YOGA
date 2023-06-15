#pragma once

#include <parfait/Point.h>

namespace YOGA {

template<typename T>
class InverseReceptor {
  public:
    InverseReceptor(int donor_cell,int node_id,Parfait::Point<T> xyz)
    :donor_cell_id(donor_cell),local_node_id(node_id),p(xyz){}

    InverseReceptor() = delete;

    int donor_cell_id;
    int local_node_id;
    Parfait::Point<T> p;
};

}
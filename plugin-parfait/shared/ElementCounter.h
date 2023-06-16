#pragma once
#include <map>
#include <cgnslib.h>

class ElementCounter {
  public:
    long currentCount(CGNS_ENUMT(ElementType_t) element_type);

    void setCounter(CGNS_ENUMT(ElementType_t) element_type, long current_count);

  private:
    std::map<CGNS_ENUMT(ElementType_t), long> count;
};
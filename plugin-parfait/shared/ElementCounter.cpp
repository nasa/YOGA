#include "ElementCounter.h"

long ElementCounter::currentCount(CGNS_ENUMT(ElementType_t) element_type) {
    if (count.count(element_type) == 0)
        return 0;
    else
        return count.at(element_type);
}

void ElementCounter::setCounter(CGNS_ENUMT(ElementType_t) element_type, long current_count) {
    count[element_type] = current_count;
}

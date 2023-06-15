#pragma once
#include <vector>
#include <string>
#include "Extent.h"

namespace Parfait {

namespace ExtentWriter {
    void write(std::string filename, const std::vector<Parfait::Extent<double>>& extents);
    void write(std::string filename,
               const std::vector<Parfait::Extent<double>>& extents,
               const std::vector<double>& field);

}
}

#include "ExtentWriter.hpp"

#pragma once
#include "EnsightReader.h"
#include "GmshReader.h"
#include "Reader.h"
#include "UgridReader.h"
#include "STLReader.h"
#include "TetGenReader.h"
#include "TriReader.h"
#include "UgridAsciiReader.h"
#ifdef PAR_HAVE_CGNS
#include "CGNSReader.h"
#endif

#include <memory>
#include <string>
#include "TecplotPointCloudReader.h"
#include "SU2IO.h"

namespace ReaderFactory {
inline bool contains(const std::vector<std::string>& vec, const std::string& s) {
    for (const auto& v : vec) {
        if (v == s) return true;
    }
    return false;
}

inline std::shared_ptr<Reader> getReaderByFilename(std::string filename) {
    auto split_filename = Parfait::StringTools::split(filename, ".");
    if (contains(split_filename, "lb8") or contains(split_filename, "b8")) {
        return std::make_shared<UgridReader>(filename);
    } else if (Parfait::StringTools::getExtension(filename) == "ugrid") {
        return std::make_shared<UgridAsciiReader>(filename);
    } else if (Parfait::StringTools::getExtension(filename) == "dat" or
               Parfait::StringTools::getExtension(filename) == "3D") {
        return std::make_shared<TecplotPointCloudReader>(filename);
    } else if (contains(split_filename, "msh")) {
        return std::make_shared<GmshReader>(filename);
    } else if (contains(split_filename, "ens")) {
        return std::make_shared<EnsightReader>(filename);
    } else if (contains(split_filename, "su2")) {
        return std::make_shared<SU2Reader>(filename);
    } else if (Parfait::StringTools::getExtension(filename) == "stl" or
               Parfait::StringTools::getExtension(filename) == "obj") {
        return std::make_shared<STLReader>(filename);
    } else if (Parfait::StringTools::getExtension(filename) == "tri") {
        return std::make_shared<TriReader>(filename);
    } else if (Parfait::StringTools::getExtension(filename) == "node" or
               Parfait::StringTools::getExtension(filename) == "ele") {
        return std::make_shared<TetGenReader>(filename);
    } else if (contains(split_filename, "cgns")) {
#ifdef PAR_HAVE_CGNS
        return std::make_shared<CGNSReader>(filename);
#else
        throw std::logic_error("Cannot read cgns; it must be enabled.");
#endif
    }
    throw std::logic_error("This preprocessor doesn't know what to do with input filename: " + filename);
}
}

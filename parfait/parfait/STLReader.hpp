
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once
#include <fstream>
#include <stdexcept>
#include "Facet.h"
#include "STLReader.h"
#include "Throw.h"
#include "ToString.h"
#include "StringTools.h"

namespace Parfait {
namespace STL {

    inline BinaryReader::BinaryReader(std::string fileName_in) : fileName(fileName_in) { throwIfFileDoesNotExist(); }

    inline size_t BinaryReader::readFacetCount() {
        FILE* fp = fopen(fileName.c_str(), "r'");
        chompHeader(fp);
        int num_facets = 0;
        fread(&num_facets, sizeof(int), 1, fp);
        fclose(fp);
        return static_cast<int>(num_facets);
    }
    inline void BinaryReader::chompHeader(FILE* fp) { fseek(fp, 80 * sizeof(char), SEEK_SET); }

    inline void BinaryReader::throwIfFileDoesNotExist() {
        FILE* fp = fopen(fileName.c_str(), "r");
        if (fp == NULL) throw std::logic_error("Could not open file " + fileName);
        fclose(fp);
    }

    inline std::vector<Parfait::Facet> BinaryReader::readFacets() {
        int facet_count = readFacetCount();
        if (not isFacetCountCorrect(facet_count)) {
            PARFAIT_WARNING("binary stl file facet count is bogus: ");
        }
        printf("Reading %s facets\n", Parfait::bigNumberToStringWithCommas(facet_count).c_str());
        return readFacets(0, facet_count);
    }

    inline std::vector<Parfait::Facet> BinaryReader::readFacets(int start, int end) {
        int readCount = end - start;
        std::vector<Facet> facets(readCount);
        FILE* fp = fopen(fileName.c_str(), "r");
        putPointerAtFacet(fp, start);

        for (int i = 0; i < readCount; i++) {
            readFacet(fp, facets[i]);
        }

        fclose(fp);
        return facets;
    }

    inline void* BinaryReader::putPointerAtFacet(FILE* fp, int start) {
        int headerOffset = 80 * sizeof(char);
        int facetCountOffset = 1 * sizeof(int);
        int sizeofFacet = 12 * sizeof(float) + 2 * sizeof(char);

        int offset = headerOffset + facetCountOffset + start * sizeofFacet;
        fseek(fp, offset, SEEK_SET);
        return fp;
    }

    inline void BinaryReader::readFacet(FILE* fp, Facet& facet) {
        float ddum[3] = {-999, -999, -999};
        fread(ddum, sizeof(float), 3, fp);  // Normal xyz
        facet.normal[0] = ddum[0];
        facet.normal[1] = ddum[1];
        facet.normal[2] = ddum[2];
        fread(ddum, sizeof(float), 3, fp);  // Point 1
        facet[0][0] = ddum[0];
        facet[0][1] = ddum[1];
        facet[0][2] = ddum[2];
        fread(ddum, sizeof(float), 3, fp);  // Point 2
        facet[1][0] = ddum[0];
        facet[1][1] = ddum[1];
        facet[1][2] = ddum[2];
        fread(ddum, sizeof(float), 3, fp);  // Point 3
        facet[2][0] = ddum[0];
        facet[2][1] = ddum[1];
        facet[2][2] = ddum[2];
        fread(ddum, sizeof(char), 2, fp);
    }

    inline bool BinaryReader::isFacetCountCorrect(size_t count) {
        FILE* fp = fopen(fileName.c_str(), "r");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for reading.");
        fseek(fp, 0L, SEEK_END);
        size_t length = ftell(fp);
        size_t header_size = 80 * sizeof(char) + sizeof(int);
        size_t facet_size = 12 * sizeof(float) + 2 * sizeof(char);
        length -= header_size;
        length /= facet_size;
        bool consistent = true;
        if (count != length) {
            consistent = false;
        }

        fclose(fp);
        return consistent;
    }
    inline bool BinaryReader::isConsistent() { return isFacetCountCorrect(readFacetCount()); }

    inline AsciiReader::AsciiReader(std::string filename) : filename(filename) {}

    inline Parfait::Facet readFacet(std::ifstream& file, std::string line) {
        using namespace Parfait::StringTools;
        auto facet_normal_segments = split(line, " ");
        Parfait::Facet facet;
        facet.normal[0] = toDouble(facet_normal_segments[2]);
        facet.normal[1] = toDouble(facet_normal_segments[3]);
        facet.normal[2] = toDouble(facet_normal_segments[4]);

        std::getline(file, line);  // chomp outer loop

        for (int i = 0; i < 3; i++) {
            std::getline(file, line);
            auto vertex_segments = split(line, " ");
            facet[i][0] = toDouble(vertex_segments[1]);
            facet[i][1] = toDouble(vertex_segments[2]);
            facet[i][2] = toDouble(vertex_segments[3]);
        }
        std::getline(file, line);  // chomp endloop
        std::getline(file, line);  // chomp outfacet
        return facet;
    }

    inline double percent(size_t current, size_t total) {
        return 100 * static_cast<double>(current) / static_cast<double>(total);
    }

    inline std::vector<Parfait::Facet> AsciiReader::readFacets() {
        std::ifstream f(filename);
        std::string line;
        std::vector<Parfait::Facet> facets;
        using namespace Parfait::StringTools;
        for (; std::getline(f, line);) {
            if (contains(line, "solid") or contains(line, "color")) {
                continue;
            } else if (contains(line, "facet")) {
                facets.push_back(readFacet(f, line));
            }
        }
        printf(
            "Read %s facets from %s\n", Parfait::bigNumberToStringWithCommas(facets.size()).c_str(), filename.c_str());
        return facets;
    }
}
}


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
#include "ByteSwap.h"
#include "CellWindingConverters.h"
#include "UgridReader.h"

#include <stdexcept>

inline void Parfait::UgridReader::readHeader() {
    fread(&nnodes, sizeof(int), 1, f);
    fread(&ntri, sizeof(int), 1, f);
    fread(&nquad, sizeof(int), 1, f);
    fread(&ntet, sizeof(int), 1, f);
    fread(&npyramid, sizeof(int), 1, f);
    fread(&nprism, sizeof(int), 1, f);
    fread(&nhex, sizeof(int), 1, f);
    if (swap_bytes) {
        bswap_32(&nnodes);
        bswap_32(&ntri);
        bswap_32(&nquad);
        bswap_32(&ntet);
        bswap_32(&npyramid);
        bswap_32(&nprism);
        bswap_32(&nhex);
    }
}

inline long Parfait::UgridReader::calcOffsetForSection(Section section) {
    long offset = 7 * sizeof(int);
    if (Section::Nodes == section) return offset;
    offset += 3 * long(nnodes) * sizeof(double);
    if (Section::Triangles == section) return offset;
    offset += 3 * long(ntri) * sizeof(int);
    if (Section::Quads == section) return offset;
    offset += 4 * long(nquad) * sizeof(int);
    if (Section::TriTags == section) return offset;
    offset += long(ntri) * sizeof(int);
    if (Section::QuadTags == section) return offset;
    offset += long(nquad) * sizeof(int);
    if (Section::Tets == section) return offset;
    offset += 4 * long(ntet) * sizeof(int);
    if (Section::Pyramids == section) return offset;
    offset += 5 * long(npyramid) * sizeof(int);
    if (Section::Prisms == section) return offset;
    offset += 6 * long(nprism) * sizeof(int);
    return offset;
}

inline void checkForInvalidNodeIds(const std::vector<int>& v, const std::string& section) {
    long n_neg = 0;
    for (auto id : v) {
        if (id < 0) n_neg++;
    }
    if (n_neg > 0) {
        std::string s = "Parfait::UgridReader: read negative node ids (" + std::to_string(n_neg) + " / " +
                        std::to_string(v.size()) + ")" + " in " + section;
        throw std::logic_error(s);
    }
}

inline std::vector<double> Parfait::UgridReader::readNodes() { return readNodes(0, nnodes); }

inline std::vector<double> Parfait::UgridReader::readNodes(int begin, int end) {
    int nrequested = end - begin;
    std::vector<double> nodes(3 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Nodes);
    byteOffset += 3 * begin * sizeof(double);
    fseek(f, byteOffset, SEEK_SET);
    fread(&nodes[0], sizeof(double), 3 * nrequested, f);
    if (swap_bytes) {
        for (double& p : nodes) {
            bswap_64(&p);
        }
    }

    return nodes;
}

inline std::vector<int> Parfait::UgridReader::readTriangles() { return readTriangles(0, ntri); }

inline std::vector<int> Parfait::UgridReader::readTriangles(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> triangles(3 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Triangles);
    byteOffset += 3 * begin * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&triangles[0], sizeof(int), 3 * nrequested, f);
    if (swap_bytes) swapBytes(triangles);

    convertToCIndexing(triangles);
    checkForInvalidNodeIds(triangles, "triangles");
    return triangles;
}

inline std::vector<int> Parfait::UgridReader::readQuads() { return readQuads(0, nquad); }

inline std::vector<int> Parfait::UgridReader::readQuads(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> quads(4 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Quads);
    byteOffset += 4 * begin * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&quads[0], sizeof(int), 4 * nrequested, f);
    if (swap_bytes) swapBytes(quads);

    convertToCIndexing(quads);
    checkForInvalidNodeIds(quads, "quads");
    return quads;
}

inline std::vector<int> Parfait::UgridReader::readTets() { return readTets(0, ntet); }

inline std::vector<int> Parfait::UgridReader::readTets(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> tets(4 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Tets);
    byteOffset += 4 * long(begin) * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&tets[0], sizeof(int), 4 * nrequested, f);

    if (swap_bytes) swapBytes(tets);
    convertToCIndexing(tets);
    for (int i = 0; i < nrequested; i++) Parfait::AflrToCGNS::convertTet(&tets[4 * i]);
    checkForInvalidNodeIds(tets, "tets");
    return tets;
}

inline std::vector<int> Parfait::UgridReader::readPyramids() { return readPyramids(0, npyramid); }

inline std::vector<int> Parfait::UgridReader::readPyramids(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> pyrs(5 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Pyramids);
    byteOffset += 5 * long(begin) * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&pyrs[0], sizeof(int), 5 * nrequested, f);
    if (swap_bytes) swapBytes(pyrs);

    convertToCIndexing(pyrs);
    for (int i = 0; i < nrequested; i++) Parfait::AflrToCGNS::convertPyramid(&pyrs[5 * i]);
    checkForInvalidNodeIds(pyrs, "pyramids");
    return pyrs;
}

inline std::vector<int> Parfait::UgridReader::readPrisms() { return readPrisms(0, nprism); }

inline std::vector<int> Parfait::UgridReader::readPrisms(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> prisms(6 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Prisms);
    byteOffset += 6 * long(begin) * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&prisms[0], sizeof(int), 6 * nrequested, f);
    if (swap_bytes) swapBytes(prisms);

    convertToCIndexing(prisms);
    for (int i = 0; i < nrequested; i++) Parfait::AflrToCGNS::convertPrism(&prisms[6 * i]);
    checkForInvalidNodeIds(prisms, "prisms");
    return prisms;
}

inline std::vector<int> Parfait::UgridReader::readHexs() { return readHexs(0, nhex); }

inline std::vector<int> Parfait::UgridReader::readHexs(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> hexs(8 * nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::Hexs);
    byteOffset += 8 * long(begin) * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&hexs[0], sizeof(int), 8 * nrequested, f);
    if (swap_bytes) swapBytes(hexs);

    convertToCIndexing(hexs);
    for (int i = 0; i < nrequested; i++) Parfait::AflrToCGNS::convertHex(&hexs[8 * i]);
    checkForInvalidNodeIds(hexs, "hexs");
    return hexs;
}

inline std::vector<int> Parfait::UgridReader::readTriangleBoundaryTags() { return readBoundaryTags(0, ntri); }

inline std::vector<int> Parfait::UgridReader::readTriangleBoundaryTags(int begin, int end) {
    return readBoundaryTags(begin, end);
}

inline std::vector<int> Parfait::UgridReader::readQuadBoundaryTags() { return readBoundaryTags(ntri, ntri + nquad); }

inline std::vector<int> Parfait::UgridReader::readQuadBoundaryTags(int begin, int end) {
    return readBoundaryTags(ntri + begin, ntri + end);
}

inline std::vector<int> Parfait::UgridReader::readBoundaryTags(int begin, int end) {
    int nrequested = end - begin;
    std::vector<int> tags(nrequested, 0);

    long byteOffset = calcOffsetForSection(Section::TriTags);
    byteOffset += begin * sizeof(int);
    fseek(f, byteOffset, SEEK_SET);
    fread(&tags[0], sizeof(int), nrequested, f);
    if (swap_bytes) swapBytes(tags);
    return tags;
}

inline FILE* Parfait::UgridReader::openGrid(const std::string& fname) {
    FILE* f_ptr = fopen(fname.c_str(), "rb");
    if (f_ptr == NULL) {
        throw std::domain_error("Could not open .ugrid file: " + fname);
    }
    return f_ptr;
}

inline void Parfait::UgridReader::swapBytes(std::vector<int>& v) {
    for (int& id : v) {
        bswap_32(&id);
    }
}

inline void Parfait::UgridReader::convertToCIndexing(std::vector<int>& v) {
    for (int& n : v) --n;
}
inline void Parfait::UgridReader::readHeader(
    int& n_nodes, int& n_tri, int& n_quad, int& n_tet, int& n_pyr, int& n_prism, int& n_hex) {
    n_nodes = nnodes;
    n_tri = ntri;
    n_quad = nquad;
    n_tet = ntet;
    n_pyr = npyramid;
    n_prism = nprism;
    n_hex = nhex;
}

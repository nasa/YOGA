
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
#include <assert.h>
#include <vector>
#include <stdlib.h>
#include <string>
namespace Parfait {
class UgridReader {
  public:
    explicit UgridReader(std::string filename, bool swap_bytes = false)
        : filename(filename), swap_bytes(swap_bytes), f(openGrid(filename)) {
        readHeader();
    }
    void readHeader(int& nnodes, int& ntri, int& nquad, int& ntet, int& npyr, int& prism, int& hex);
    // read all elements of a type
    std::vector<double> readNodes();
    std::vector<int> readTriangles();
    std::vector<int> readQuads();
    std::vector<int> readTets();
    std::vector<int> readPyramids();
    std::vector<int> readPrisms();
    std::vector<int> readHexs();
    std::vector<int> readTriangleBoundaryTags();
    std::vector<int> readQuadBoundaryTags();
    // read a slice of the elements of a type
    std::vector<double> readNodes(int begin, int end);
    std::vector<int> readTriangles(int begin, int end);
    std::vector<int> readQuads(int begin, int end);
    std::vector<int> readTets(int begin, int end);
    std::vector<int> readPyramids(int begin, int end);
    std::vector<int> readPrisms(int begin, int end);
    std::vector<int> readHexs(int begin, int end);
    std::vector<int> readBoundaryTags(int begin, int end);
    std::vector<int> readTriangleBoundaryTags(int begin, int end);
    std::vector<int> readQuadBoundaryTags(int begin, int end);

    ~UgridReader() { fclose(f); }

  private:
    enum Section { Nodes, Triangles, TriTags, Quads, QuadTags, Tets, Pyramids, Prisms, Hexs };
    std::string filename;
    bool swap_bytes;
    FILE* f;
    int nnodes;
    int ntri;
    int nquad;
    int ntet;
    int npyramid;
    int nprism;
    int nhex;

    long calcOffsetForSection(Section section);
    FILE* openGrid(const std::string& fname);
    void swapBytes(std::vector<int>& v);
    void convertToCIndexing(std::vector<int>& v);
    void readHeader();
};
}

#include "UgridReader.hpp"
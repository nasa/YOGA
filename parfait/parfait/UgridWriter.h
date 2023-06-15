
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
#include <stdlib.h>
#include <string>
#include <vector>
#include "ImportedUgrid.h"
namespace Parfait {

class UgridWriter {
  public:
    inline UgridWriter() : doByteSwap(false) {}

    void setName(std::string filename);

    void setTets(int* tets, int numberOfTets);
    void setHexes(int* hexes, int numberOfHexes);
    void setPyramids(int* pyramids, int numberOfPyramids);
    void setPrisms(int* prisms, int numberOfPrisms);
    void setTriangles(int* triangles, int numberOfTriangles);
    void setQuads(int* quads, int numberOfQuads);
    void setTriangleTags(int* triangleTags);
    void setQuadTags(int* quadTags);
    void writeFile();
    void setNodes(double* points, int numberOfNodes);

    static void writeHeader(
        std::string filename, int nnodes, int ntri, int nquad, int ntet, int npyr, int prism, int hex, bool swapBytes);

    static void writeNodes(std::string filename, int nnodes, double* nodes, bool swapBytes);
    static void writeTriangles(std::string filename, int ntriangles, int* triangles, bool swapBytes);
    static void writeQuads(std::string filename, int nquads, int* quads, bool swapBytes);
    static void writeTets(std::string filename, int ntets, int* tets, bool swapBytes);
    static void writePyramids(std::string filename, int npyramyds, int* pyramids, bool swapBytes);
    static void writePrisms(std::string filename, int nprisms, int* prisms, bool swapBytes);
    static void writeHexs(std::string filename, int nhexs, int* hexs, bool swapBytes);
    static void writeTriangleBoundaryTags(std::string filename, int ntriangles, int* triangleTags, bool swapBytes);
    static void writeQuadBoundaryTags(std::string filename, int nquads, int* quadTags, bool swapBytes);

    static void writeIntegerField(std::string filename, int n, int* fieldData, bool swapBytes);

  private:
    bool doByteSwap;
    std::string fileName;
    int numberOfTets = 0;
    int* tets = nullptr;

    int numberOfHexes = 0;
    int* hexes = nullptr;

    int* pyramids = nullptr;
    int numberOfPyramids = 0;

    int* prisms = nullptr;
    int numberOfPrisms = 0;

    int* triangles = nullptr;
    int numberOfTriangles = 0;

    int numberOfQuads = 0;
    int* quads = nullptr;

    int* triangleTags = 0;
    int* quadTags = nullptr;

    int numberOfNodes = 0;
    double* nodes = nullptr;
};
}

#include "UgridWriter.hpp"
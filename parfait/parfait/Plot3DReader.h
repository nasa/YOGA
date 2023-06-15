
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
#include <utility>
#include "Point.h"
#include "StringTools.h"
#include "FortranUnformatted.h"

namespace Parfait {

class Plot3DReader {
  public:
    explicit Plot3DReader(const std::string& filename) { readFile(filename, isGridFile(filename), false); }
    explicit Plot3DReader(const std::string& filename, bool is_single_precision) {
        readFile(filename, isGridFile(filename), is_single_precision);
    }
    explicit Plot3DReader(const std::string& filename, bool is_grid_file, bool is_single_precision) {
        readFile(filename, is_grid_file, is_single_precision);
    }

    void readFile(const std::string& filename, const bool is_grid_file, const bool is_single_precision) {
        f = fopen(filename.c_str(), "rb");
        if (f == nullptr) throw std::domain_error("Could not open plot3d file: " + filename);
        if (is_grid_file) {
            readGridHeader();
        } else {
            readSolutionHeader();
        }
        readData(is_single_precision);
    }

    static bool isGridFile(const std::string& filename) {
        auto extension = StringTools::getExtension(filename);
        return (extension == "g" || extension == "x");
    }

    int blockCount() { return number_of_blocks; }

    double getVariable(int block_id, int i, int j, int k, int variable) {
        return internal_data.at(calcOffset(block_id, i, j, k, variable));
    }

    struct BlockDimensions {
        int i;
        int j;
        int k;
        int n_variables;
        int getSize() const { return i * j * k * n_variables; };
    };

    BlockDimensions blockDimensions(int block_id) { return block_dimensions.at(block_id); }

    virtual ~Plot3DReader() { fclose(f); }

  private:
    FILE* f;

    int number_of_blocks;
    std::vector<BlockDimensions> block_dimensions;
    std::vector<double> internal_data;

    void readGridHeader() {
        FortranUnformatted::skipMarker(f);
        fread(&number_of_blocks, sizeof(int), 1, f);
        FortranUnformatted::skipMarker(f);
        block_dimensions.resize(number_of_blocks);
        FortranUnformatted::skipMarker(f);
        for (int block_id = 0; block_id < number_of_blocks; ++block_id) {
            fread(&block_dimensions[block_id].i, sizeof(int), 1, f);
            fread(&block_dimensions[block_id].j, sizeof(int), 1, f);
            fread(&block_dimensions[block_id].k, sizeof(int), 1, f);
            block_dimensions[block_id].n_variables = 3;
        }
        FortranUnformatted::skipMarker(f);
    }

    void readSolutionHeader() {
        FortranUnformatted::skipMarker(f);
        fread(&number_of_blocks, sizeof(int), 1, f);
        FortranUnformatted::skipMarker(f);
        block_dimensions.resize(number_of_blocks);
        FortranUnformatted::skipMarker(f);
        for (int block_id = 0; block_id < number_of_blocks; ++block_id) {
            fread(&block_dimensions[block_id].i, sizeof(int), 1, f);
            fread(&block_dimensions[block_id].j, sizeof(int), 1, f);
            fread(&block_dimensions[block_id].k, sizeof(int), 1, f);
            fread(&block_dimensions[block_id].n_variables, sizeof(int), 1, f);
        }
        FortranUnformatted::skipMarker(f);
    }

    void readData(bool is_single_precision) {
        long total_size = 0;
        for (const auto& dim : block_dimensions) total_size += dim.getSize();

        internal_data.resize(total_size);
        for (int block_id = 0; block_id < number_of_blocks; ++block_id) {
            const auto& dim = block_dimensions.at(block_id);
            FortranUnformatted::skipMarker(f);
            for (int var = 0; var < dim.n_variables; ++var) {
                for (int k = 0; k < dim.k; ++k) {
                    for (int j = 0; j < dim.j; ++j) {
                        for (int i = 0; i < dim.i; ++i) {
                            long offset = calcOffset(block_id, i, j, k, var);
                            if (is_single_precision) {
                                float temp;
                                fread(&temp, sizeof(float), 1, f);
                                internal_data[offset] = temp;
                            } else {
                                fread(&internal_data[offset], sizeof(double), 1, f);
                            }
                        }
                    }
                }
            }
            FortranUnformatted::skipMarker(f);
        }
    }

    long calcOffset(int block_id, int i, int j, int k, int var) const {
        long offset = 0;
        for (int b = 0; b < block_id; ++b) offset += block_dimensions.at(b).getSize();
        const auto& dim = block_dimensions.at(block_id);
        offset += var + i * dim.n_variables + j * dim.i * dim.n_variables + k * dim.i * dim.j * dim.n_variables;
        return offset;
    }
};

class Plot3DGridReader : public Plot3DReader {
  public:
    explicit Plot3DGridReader(const std::string& filename) : Plot3DReader(filename) {}

    Parfait::Point<double> getXYZ(int block_id, int i, int j, int k) {
        auto x = getVariable(block_id, i, j, k, 0);
        auto y = getVariable(block_id, i, j, k, 1);
        auto z = getVariable(block_id, i, j, k, 2);
        return {x, y, z};
    }
};
}

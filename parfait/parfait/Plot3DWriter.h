
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

#include <map>
#include "Point.h"
#include "FortranUnformatted.h"

namespace Parfait {
class Plot3DWriter {
  public:
    virtual ~Plot3DWriter() = default;
    void insert(int block_id, int i, int j, int k, int variable, double v) {
        internal_data[{block_id, i, j, k, variable}] = v;
        if (long(block_dimensions.size()) <= block_id) {
            block_dimensions.resize(block_id + 1);
            block_dimensions.at(block_id).i = 0;
            block_dimensions.at(block_id).j = 0;
            block_dimensions.at(block_id).k = 0;
            block_dimensions.at(block_id).n_variables = 0;
        }
        auto& d = block_dimensions.at(block_id);
        d.i = std::max(d.i, i + 1);
        d.j = std::max(d.j, j + 1);
        d.k = std::max(d.k, k + 1);
        d.n_variables = std::max(d.n_variables, variable + 1);
    }
    int blockCount() const { return int(block_dimensions.size()); }

    struct BlockDimensions {
        int i;
        int j;
        int k;
        int n_variables;
        int getSize() const { return i * j * k * n_variables; };
    };

    BlockDimensions blockDimensions(int block_id) { return block_dimensions.at(block_id); }
    void write(const std::string& filename) {
        auto f = fopen(filename.c_str(), "wb");
        writeHeader(f);

        for (int block_id = 0; block_id < blockCount(); ++block_id) {
            const auto& dim = block_dimensions.at(block_id);
            FortranUnformatted::writeMarker<double>(dim.getSize(), f);
            for (int variable = 0; variable < dim.n_variables; ++variable) {
                for (int k = 0; k < dim.k; ++k) {
                    for (int j = 0; j < dim.j; ++j) {
                        for (int i = 0; i < dim.i; ++i) {
                            std::tuple<int, int, int, int, int> v = {block_id, i, j, k, variable};
                            if (internal_data.count(v) == 0) {
                                throw std::runtime_error(
                                    "Error:non-contiguous data.  Verify that insert calls span the full space the "
                                    "structured grid.");
                            }
                            writeToFile(internal_data.at({block_id, i, j, k, variable}), f);
                        }
                    }
                }
            }
            FortranUnformatted::writeMarker<double>(dim.getSize(), f);
        }

        fclose(f);
    }

  protected:
    std::vector<BlockDimensions> block_dimensions;
    std::map<std::tuple<int, int, int, int, int>, double> internal_data;

    virtual void writeHeader(FILE* f) const = 0;

    template <typename T>
    void writeToFile(const T& v, FILE* f) const {
        fwrite(&v, sizeof(T), 1, f);
    }
};

class Plot3DGridWriter : public Plot3DWriter {
  public:
    void setPoint(int block_id, int i, int j, int k, const Parfait::Point<double>& p) {
        for (int direction = 0; direction < 3; ++direction) {
            insert(block_id, i, j, k, direction, p[direction]);
        }
    }
    Point<double> getPoint(int block_id, int i, int j, int k) const {
        auto x = internal_data.at({block_id, i, j, k, 0});
        auto y = internal_data.at({block_id, i, j, k, 1});
        auto z = internal_data.at({block_id, i, j, k, 2});
        return {x, y, z};
    }

  private:
    void writeHeader(FILE* f) const override {
        FortranUnformatted::writeMarker<int>(1, f);
        writeToFile(blockCount(), f);
        FortranUnformatted::writeMarker<int>(1, f);

        auto total_size = blockCount() * 3;

        FortranUnformatted::writeMarker<int>(total_size, f);
        for (const auto& dimension : block_dimensions) {
            writeToFile(dimension.i, f);
            writeToFile(dimension.j, f);
            writeToFile(dimension.k, f);
        }
        FortranUnformatted::writeMarker<int>(total_size, f);
    }
};

class Plot3DSolutionWriter : public Plot3DWriter {
  private:
    void writeHeader(FILE* f) const override {
        FortranUnformatted::writeMarker<int>(1, f);
        writeToFile(blockCount(), f);
        FortranUnformatted::writeMarker<int>(1, f);

        auto total_size = blockCount() * 4;

        FortranUnformatted::writeMarker<int>(total_size, f);
        for (const auto& dimension : block_dimensions) {
            writeToFile(dimension.i, f);
            writeToFile(dimension.j, f);
            writeToFile(dimension.k, f);
            writeToFile(dimension.n_variables, f);
        }
        FortranUnformatted::writeMarker<int>(total_size, f);
    }
};
}

#pragma once
#include <vector>
#include <array>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include "MDVector.h"
#include "StructuredMesh.h"

namespace inf {

class StructuredPlotter {
  public:
    StructuredPlotter(MessagePasser mp,
                      std::string filename_in,
                      std::shared_ptr<StructuredMesh> mesh);

    void addField(const std::shared_ptr<inf::StructuredField>& field);

    void writeSolutionFile();

    void writeGridFile();

    void visualize();

  private:
    using Fields = MDVector<double, 4, LayoutLeft>;
    using Coordinates = MDVector<double, 4, LayoutLeft>;
    MessagePasser mp;
    std::string filename;
    std::shared_ptr<StructuredMesh> mesh;
    std::vector<std::shared_ptr<StructuredField>> fields;
    std::vector<std::array<int, 3>> all_block_dimensions;
    std::vector<int> all_block_owners;
    int total_block_count;

    void writeSolutionFileHeader(FILE* fp);

    void writeGridFileHeader(FILE* fp);

    void writeBlockFields(FILE* fp);

    void writeBlockXYZs(FILE* fp);

    void rootWrite(FILE* fp, const std::vector<double>& values) const;

    void writeFieldNameFile();

    template <typename T>
    void writeToFile(const T& v, FILE* f) const {
        fwrite(&v, sizeof(T), 1, f);
    }

    Fields extractFields(int block_id) const;

    Coordinates extractXYZs(const StructuredBlock& block);
};

}

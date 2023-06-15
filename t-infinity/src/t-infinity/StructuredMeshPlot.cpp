#include "StructuredMeshPlot.h"
#include <parfait/StringTools.h>
#include <parfait/FortranUnformatted.h>
#include "FieldInterface.h"

using namespace Parfait;

inf::StructuredPlotter::StructuredPlotter(MessagePasser mp,
                                          std::string filename_in,
                                          std::shared_ptr<StructuredMesh> mesh)
    : mp(mp), filename(std::move(filename_in)), mesh(mesh) {
    total_block_count = mp.ParallelSum(mesh->blockIds().size());

    std::map<int, std::array<int, 3>> my_block_dimensions;
    std::map<int, int> block_owner;
    for (int block_id : mesh->blockIds()) {
        my_block_dimensions[block_id] = mesh->getBlock(block_id).nodeDimensions();
        block_owner[block_id] = mp.Rank();
    }

    //----- WARNING!  THIS ASSUMES BLOCK IDS ARE AN ORDINAL RANGE!!!!!
    all_block_dimensions = mp.GatherByIds(my_block_dimensions, 0);
    all_block_owners = mp.GatherByIds(block_owner, 0);
    //----- WARNING!  THIS ASSUMES BLOCK IDS ARE AN ORDINAL RANGE!!!!!
}
void inf::StructuredPlotter::addField(const std::shared_ptr<inf::StructuredField>& field) {
    PARFAIT_ASSERT(
        field->association() == FieldAttributes::Node(),
        "PLOT3D format requires a node association, but got cell association for field: " +
            field->name());
    fields.push_back(field);
}
void inf::StructuredPlotter::visualize() {
    writeGridFile();
    writeSolutionFile();
}
void inf::StructuredPlotter::writeSolutionFileHeader(FILE* fp) {
    if (mp.Rank() == 0) {
        FortranUnformatted::writeMarker<int>(1, fp);
        writeToFile(total_block_count, fp);
        FortranUnformatted::writeMarker<int>(1, fp);

        auto total_size = total_block_count * 4;
        int num_fields = fields.size();

        FortranUnformatted::writeMarker<int>(total_size, fp);
        for (const auto& d : all_block_dimensions) {
            writeToFile(d[0], fp);
            writeToFile(d[1], fp);
            writeToFile(d[2], fp);
            writeToFile(num_fields, fp);
        }
        FortranUnformatted::writeMarker<int>(total_size, fp);
    }
}
void inf::StructuredPlotter::writeGridFileHeader(FILE* fp) {
    if (mp.Rank() == 0) {
        FortranUnformatted::writeMarker<int>(1, fp);
        writeToFile(total_block_count, fp);
        FortranUnformatted::writeMarker<int>(1, fp);
        auto total_size = total_block_count * 3;
        FortranUnformatted::writeMarker<int>(total_size, fp);
        for (const auto& dimension : all_block_dimensions) {
            writeToFile(dimension[0], fp);
            writeToFile(dimension[1], fp);
            writeToFile(dimension[2], fp);
        }
        FortranUnformatted::writeMarker<int>(total_size, fp);
    }
}
void inf::StructuredPlotter::writeBlockFields(FILE* fp) {
    std::map<int, const StructuredBlock*> my_blocks;
    for (int block_id : mesh->blockIds()) {
        my_blocks[block_id] = &mesh->getBlock(block_id);
    }
    for (int block_id = 0; block_id < total_block_count; block_id++) {
        bool i_own_block = (my_blocks.count(block_id) == 1);

        if (mp.Rank() == 0) {
            int num_fields = fields.size();
            auto block_dims = all_block_dimensions[block_id];
            std::array<int, 4> block_field_dims{
                block_dims[0], block_dims[1], block_dims[2], num_fields};
            Fields block_field(block_field_dims);
            if (i_own_block) {
                block_field = extractFields(block_id);
            } else {
                mp.Recv(block_field.vec, all_block_owners.at(block_id));
            }

            rootWrite(fp, block_field.vec);
        }

        else if (i_own_block) {
            auto block_field = extractFields(block_id);
            mp.Send(block_field.vec, 0);
        }
    }
}
void inf::StructuredPlotter::writeBlockXYZs(FILE* fp) {
    std::set<int> my_blocks;
    for (int block_id : mesh->blockIds()) {
        my_blocks.insert(block_id);
    }
    for (int block_id = 0; block_id < total_block_count; block_id++) {
        bool i_own_block = (my_blocks.count(block_id) == 1);

        if (mp.Rank() == 0) {
            const auto& d = all_block_dimensions.at(block_id);
            Coordinates xyz({d[0], d[1], d[2], 3});
            if (i_own_block) {
                xyz = extractXYZs(mesh->getBlock(block_id));
            } else {
                mp.Recv(xyz.vec, all_block_owners.at(block_id));
            }

            rootWrite(fp, xyz.vec);
        } else if (i_own_block) {
            auto xyz = extractXYZs(mesh->getBlock(block_id));
            mp.Send(xyz.vec, 0);
        }
    }
}
void inf::StructuredPlotter::rootWrite(FILE* fp, const std::vector<double>& values) const {
    FortranUnformatted::writeMarker<double>(values.size(), fp);
    fwrite(values.data(), sizeof(double), values.size(), fp);
    FortranUnformatted::writeMarker<double>(values.size(), fp);
}
void inf::StructuredPlotter::writeFieldNameFile() {
    if (mp.Rank() == 0) {
        auto name_filename = filename;
        auto extension = StringTools::getExtension(name_filename);
        if (extension != "nam") {
            name_filename += ".nam";
        }
        FILE* fp = fopen(name_filename.c_str(), "w");
        for (const auto& f : fields) {
            fprintf(fp, "%s\n", f->name().c_str());
        }
        fclose(fp);
    }
}
void inf::StructuredPlotter::writeSolutionFile() {
    auto sol_filename = filename;
    auto extension = StringTools::getExtension(sol_filename);
    if (extension != "q") {
        sol_filename += ".q";
    }
    FILE* fp = nullptr;
    if (mp.Rank() == 0) {
        fp = fopen(sol_filename.c_str(), "w");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for reading " + sol_filename);
    }

    writeSolutionFileHeader(fp);
    writeBlockFields(fp);
    writeFieldNameFile();

    if (mp.Rank() == 0) fclose(fp);
}
void inf::StructuredPlotter::writeGridFile() {
    auto grid_filename = filename;
    auto extension = StringTools::getExtension(grid_filename);
    if (extension != "g") {
        grid_filename += ".g";
    }
    FILE* fp = nullptr;
    if (mp.Rank() == 0) {
        fp = fopen(grid_filename.c_str(), "w");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for reading " + grid_filename);
    }

    writeGridFileHeader(fp);
    writeBlockXYZs(fp);

    if (mp.Rank() == 0) fclose(fp);
}
typename inf::StructuredPlotter::Fields inf::StructuredPlotter::extractFields(int block_id) const {
    auto block_dims = mesh->getBlock(block_id).nodeDimensions();
    int num_fields = int(fields.size());
    Fields globbed_fields({block_dims[0], block_dims[1], block_dims[2], num_fields});

    for (int f = 0; f < num_fields; ++f) {
        const auto& block_field = fields.at(f)->getBlockField(block_id);
        inf::loopMDRange({0, 0, 0}, block_dims, [&](int i, int j, int k) {
            globbed_fields(i, j, k, f) = block_field.value(i, j, k);
        });
    }

    return globbed_fields;
}
typename inf::StructuredPlotter::Coordinates inf::StructuredPlotter::extractXYZs(
    const inf::StructuredBlock& block) {
    auto dims = block.nodeDimensions();
    Coordinates xyz({dims[0], dims[1], dims[2], 3});
    inf::loopMDRange({0, 0, 0}, dims, [&](int i, int j, int k) {
        auto p = block.point(i, j, k);
        xyz(i, j, k, 0) = p[0];
        xyz(i, j, k, 1) = p[1];
        xyz(i, j, k, 2) = p[2];
    });

    return xyz;
}

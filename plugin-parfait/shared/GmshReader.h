#pragma once
#include <parfait/Point.h>
#include <parfait/Throw.h>
#include <stdio.h>
#include <t-infinity/MeshInterface.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include "Reader.h"

class GmshReader : public Reader {
  public:
    enum Type { NODE = 15, BAR_2 = 1, TRI_3 = 2, QUAD_4 = 3, TETRA_4 = 4, HEXA_8 = 5, PRISM_6 = 6, PYRA_5 = 7 };
    GmshReader(const std::string& filename) : grid_file(filename) {
        auto f = fopen(grid_file.c_str(), "r");
        if (f == nullptr) {
            PARFAIT_THROW("Could not open file for reading " + grid_file);
        }
        auto section = seekFirstSection(f);
        if ("$PhysicalNames\n" == section) {
            setUpPhysicalNamesAndMoveFilePointerToEndOfSection(f);
            char line[256];
            fgets(line, N, f);
            section = line;
        }
        if ("$Nodes\n" == section) {
            setUpNodeCountAndMoveFilePointerToEndOfNodeSection(f);
            char line[256];
            fgets(line, N, f);
            section = line;
        } else {
            throw std::logic_error("Expected $Nodes section, but found " + section);
        }

        if ("$Elements\n" != section) {
            printf("Expected $Elements section, but found %s\n", section.c_str());
            throw;
        }
        char line[256];
        fgets(line, N, f);
        long n_elements = atol(line);
        long elements_so_far = 0;
        while (elements_so_far < n_elements) {
            int header[3];
            readElementHeader(f, header);
            // printf("Element header(%li): %i %i %i\n",elements_so_far, header[0], header[1], header[2]);
            int element_type = header[0];
            int element_count = header[1];
            skipElements(f, header);
            if (0 == cell_type_counts.count(element_type)) {
                cell_type_counts[element_type] = element_count;
            } else {
                cell_type_counts.at(element_type) += element_count;
            }
            elements_so_far += header[1];
        }
    }
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const {
        long read_count = end - start;
        auto f = fopen(grid_file.c_str(), "r");
        char line[256];
        auto section = seekFirstSection(f);
        while (section != "$Nodes\n") {
            skipSection(f, section);
            fgets(line, N, f);
            section = line;
        }
        fgets(line, N, f);
        fastForward(f, start * (4 + 3 * sizeof(double)));
        std::vector<Parfait::Point<double>> points(read_count);
        for (int i = 0; i < read_count; i++) {
            fastForward(f, sizeof(int));
            fread(points[i].data(), sizeof(double), 3, f);
        }
        return points;
    }
    std::vector<Parfait::Point<double>> readCoords() const { return readCoords(0, node_count); }
    std::vector<inf::MeshInterface::CellType> cellTypes() const {
        std::vector<inf::MeshInterface::CellType> types;
        for (auto& item : cell_type_counts) {
            types.push_back(GmshToTinfType(item.first));
        }
        return types;
    }
    std::vector<int> readCellTags(inf::MeshInterface::CellType infinity_type) const {
        auto gmsh_type = convertTinfType(infinity_type);
        return readCellTags(infinity_type, 0, cell_type_counts.at(gmsh_type));
    }
    std::vector<int> readCellTags(inf::MeshInterface::CellType infinity_type, long start, long end) const {
        auto gmsh_type = convertTinfType(infinity_type);
        long read_count = end - start;

        std::vector<int> first_tags(read_count);

        auto f = fopen(grid_file.c_str(), "r");
        int element_header[3];

        char line[256];
        auto section = seekFirstSection(f);
        while ("$Elements\n" != section) {
            skipSection(f, section);
            fgets(line, N, f);
            section = line;
        }
        fgets(line, N, f);  // chomp n elements

        long read_so_far = 0;
        long skipped_so_far = 0;
        while (read_so_far < read_count) {
            readElementHeader(f, element_header);
            int type = element_header[0];
            if (type != gmsh_type) {
                skipElements(f, element_header);
            } else {
                int n_elem_in_section = element_header[1];
                int ntags = element_header[2];
                int nvert = nodesInType(type);
                int n_ints_per_element = 1 + ntags + nvert;
                std::vector<int> element(n_ints_per_element);
                for (int i = 0; i < n_elem_in_section; i++) {
                    readElement(f, nvert, ntags, element.data());
                    if (skipped_so_far < start) {
                        skipped_so_far++;
                    } else {
                        int tag = element[1];
                        first_tags[read_so_far] = tag;
                        read_so_far++;
                    }
                }
            }
        }
        return first_tags;
    }
    std::vector<long> readCells(inf::MeshInterface::CellType infinity_type) const {
        auto gmsh_type = convertTinfType(infinity_type);
        long total = cell_type_counts.at(gmsh_type);
        return readCells(infinity_type, 0, total);
    }
    void printHeader(int h[3]) const { printf("Element type(%i) count(%i) tags(%i)\n", h[0], h[1], h[2]); }
    std::vector<long> readCells(inf::MeshInterface::CellType infinity_type, long start, long end) const {
        auto gmsh_type = convertTinfType(infinity_type);
        long read_count = end - start;
        auto f = fopen(grid_file.c_str(), "r");
        int element_header[3];
        std::vector<long> cells(nodesInType(gmsh_type) * read_count);

        char line[256];
        auto section = seekFirstSection(f);
        while ("$Elements\n" != section) {
            skipSection(f, section);
            fgets(line, N, f);
            section = line;
        }
        fgets(line, N, f);  // chomp n elements
        long read_so_far = 0;
        long skipped_so_far = 0;
        while (read_so_far < read_count) {
            readElementHeader(f, element_header);
            // printHeader(element_header);
            int type = element_header[0];
            if (type != gmsh_type) {
                skipElements(f, element_header);
            } else {
                int n_elem_in_section = element_header[1];
                int ntags = element_header[2];
                int nvert = nodesInType(type);
                int n_ints_per_element = 1 + ntags + nvert;
                std::vector<int> element(n_ints_per_element);
                for (int i = 0; i < n_elem_in_section; i++) {
                    readElement(f, nvert, ntags, element.data());
                    if (skipped_so_far < start) {
                        skipped_so_far++;
                    } else {
                        std::copy(element.begin() + 1 + ntags,
                                  element.begin() + n_ints_per_element,
                                  cells.begin() + read_so_far * nvert);
                        read_so_far++;
                    }
                }
            }
        }
        std::transform(cells.begin(), cells.end(), cells.begin(), [](long x) { return --x; });
        return cells;
    }
    long nodeCount() const { return node_count; }
    long cellCount(inf::MeshInterface::CellType tinf_cell_type) const {
        Type gmsh_type = convertTinfType(tinf_cell_type);
        if (cell_type_counts.count(gmsh_type) == 0) return 0;
        return cell_type_counts.at(gmsh_type);
    }

    static Type convertTinfType(inf::MeshInterface::CellType type) {
        switch (type) {
            case inf::MeshInterface::NODE:
                return NODE;
            case inf::MeshInterface::BAR_2:
                return BAR_2;
            case inf::MeshInterface::TRI_3:
                return TRI_3;
            case inf::MeshInterface::TETRA_4:
                return TETRA_4;
            case inf::MeshInterface::QUAD_4:
                return QUAD_4;
            case inf::MeshInterface::PYRA_5:
                return PYRA_5;
            case inf::MeshInterface::PENTA_6:
                return PRISM_6;
            case inf::MeshInterface::HEXA_8:
                return HEXA_8;
            default:
                throw std::logic_error("Unsupported t-infinity type for Gmsh: " + std::to_string(type));
        }
    }
    static inf::MeshInterface::CellType GmshToTinfType(int gmsh_type) {
        switch (gmsh_type) {
            case NODE:
                return inf::MeshInterface::NODE;
            case BAR_2:
                return inf::MeshInterface::BAR_2;
            case TRI_3:
                return inf::MeshInterface::TRI_3;
            case TETRA_4:
                return inf::MeshInterface::TETRA_4;
            case QUAD_4:
                return inf::MeshInterface::QUAD_4;
            case PYRA_5:
                return inf::MeshInterface::PYRA_5;
            case PRISM_6:
                return inf::MeshInterface::PENTA_6;
            case HEXA_8:
                return inf::MeshInterface::HEXA_8;
            default:
                throw std::logic_error("Could not convert Gmsh type to Tinf: " + std::to_string(gmsh_type));
        }
    }

  private:
    const std::string grid_file;
    const int N = 256;
    long node_count = 0;
    int n_physical_names = 0;
    std::map<int, long> cell_type_counts;
    struct physical_name {
        int id;
        int tag;
        std::string name;
    };

    int nodesInType(int t) const {
        switch (t) {
            case NODE:
                return 1;
            case BAR_2:
                return 2;
            case TRI_3:
                return 3;
            case TETRA_4:
                return 4;
            case QUAD_4:
                return 4;
            case PYRA_5:
                return 5;
            case PRISM_6:
                return 6;
            case HEXA_8:
                return 8;
            default:
                throw std::logic_error("Unsupported Gmsh type: " + std::to_string(t));
        }
    }

    std::string seekFirstSection(FILE* f) const {
        fseek(f, 0, SEEK_SET);
        skipHeader(f);
        char line[256];
        fgets(line, N, f);
        std::string s = line;
        return s;
    }

    void skipSection(FILE* f, const std::string section) const {
        char line[256];
        if ("$Nodes\n" == section) {
            fgets(line, N, f);  // chomp line with node count
            skipNodes(f, node_count);
            fgets(line, N, f);  // chomp newline
            fgets(line, N, f);  // chomp $EndNodes
            std::string s(line);
            if ("$EndNodes\n" != s) {
                throw std::logic_error("Expected $EndNodes, but found " + s);
            }
        } else if ("$PhysicalNames\n" == section) {
            for (int i = 0; i < n_physical_names + 2; i++) {
                fgets(line, N, f);
            }
            std::string s(line);
            if (s != "$EndPhysicalNames\n") {
                throw std::logic_error("Expected $EndPhysicalNames, but found " + s);
            }
        } else {
            printf("Trying to skip %s\n", section.c_str());
            throw;
        }
    }

    void setUpNodeCountAndMoveFilePointerToEndOfNodeSection(FILE* f) {
        node_count = readNodeCount(f);
        skipNodes(f, node_count);
        char line[256];
        fgets(line, N, f);  // chomp newline
        fgets(line, N, f);  // chomp $EndNodes
    }

    void setUpPhysicalNamesAndMoveFilePointerToEndOfSection(FILE* f) {
        char line[256];
        fgets(line, N, f);
        int n = std::atoi(line);
        for (int i = 0; i < n; i++) {
            fgets(line, N, f);
            physical_name item;
            char word[80];
            sscanf(line, "%d %d %s", &item.id, &item.tag, word);
            item.name = word;
            n_physical_names++;
        }
        fgets(line, N, f);
        std::string s(line);
        if (s != "$EndPhysicalNames\n") {
            throw std::logic_error("Expected $EndPhysicalNames, but found " + s);
        }
    }

    void fastForward(FILE* f, long bytes) const { fseek(f, bytes, SEEK_CUR); }

    void skipHeader(FILE* f) const {
        char line[256];
        fgets(line, N, f);
        if ("$MeshFormat\n" != std::string(line)) {
            throw std::logic_error("Expected file to start with $MeshFormat, but found: " + std::string(line));
        }
        fgets(line, N, f);
        int one = 0;
        fread(&one, sizeof(int), 1, f);
        fread(line, 1, 1, f);
        fgets(line, N, f);
    }

    void skipNodes(FILE* f, long n) const {
        for (int i = 0; i < n; i++) {
            int id;
            double x, y, z;
            fread(&id, sizeof(int), 1, f);
            fread(&x, sizeof(double), 1, f);
            fread(&y, sizeof(double), 1, f);
            fread(&z, sizeof(double), 1, f);
            // printf("node id: %i xyz(%lf, %lf, %lf)\n",id,x,y,z);
        }
    }

    long readNodeCount(FILE* f) const {
        char line[256];
        fgets(line, N, f);
        return atol(line);
    }

    void readElementHeader(FILE* f, int header[3]) const { fread(header, sizeof(int), 3, f); }

    void skipElements(FILE* f, int header[3]) const {
        int type = header[0];
        int n_elements = header[1];
        int n_tags = header[2];

        int nnodes_in_type = nodesInType(type);
        std::vector<int> element(1 + n_tags + nnodes_in_type);
        for (int i = 0; i < n_elements; i++) {
            readElement(f, nnodes_in_type, n_tags, element.data());
        }
    }

    void readElement(FILE* f, int nvertices, int ntags, int* element) const {
        fread(element, sizeof(int), 1 + ntags + nvertices, f);
    }
};

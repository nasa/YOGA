#pragma once
#include <parfait/StringTools.h>
#include <stdio.h>
#include "Reader.h"

class EnsightReader : public Reader {
  public:
    void readHeader() {
        char dum[80];
        fread(dum, sizeof(char), 80, fp);  // C Binary
        fread(dum, sizeof(char), 80, fp);  // Comment
        fread(dum, sizeof(char), 80, fp);  // Comment
        fread(dum, sizeof(char), 80, fp);  // node id off
        fread(dum, sizeof(char), 80, fp);  // element id off

        // for each part... but we're just assuming 1 part
        fread(dum, sizeof(char), 80, fp);  // part

        int part;
        fread(&part, sizeof(int), 1, fp);        // Part number (we're assuming only 1 part)
        fread(dum, sizeof(char), 80, fp);        // Part name
        fread(dum, sizeof(char), 80, fp);        // coordinates
        fread(&node_count, sizeof(int), 1, fp);  // num nodes
        x_offset = ftell(fp);
        y_offset = x_offset + sizeof(float) * node_count;
        z_offset = y_offset + sizeof(float) * node_count;
        fseek(fp, z_offset + sizeof(float) * node_count, SEEK_SET);
        fread(dum, sizeof(char), 80, fp);  // element type
        std::string element_type = Parfait::StringTools::split(dum, " ")[0];
        if (element_type != "hexa8") throw std::logic_error("ensight reader can only read hexes.  Call Matt.");

        int num_elements;
        fread(&num_elements, sizeof(int), 1, fp);
        cell_type_count[inf::MeshInterface::HEXA_8] = num_elements;
        hex_nodes_offset = ftell(fp);
    }
    EnsightReader(std::string filename) {
        fp = fopen(filename.c_str(), "r");
        if (fp == nullptr) throw std::logic_error("Could not open ensight file: " + filename);
        readHeader();
    }
    ~EnsightReader() {
        if (fp != nullptr) fclose(fp);
    }
    long nodeCount() const override { return node_count; }
    long cellCount(inf::MeshInterface::CellType etype) const override { return cell_type_count.at(etype); }
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const override {
        long num_read = end - start;
        std::vector<Parfait::Point<double>> points(num_read);
        std::vector<float> x(num_read);
        std::vector<float> y(num_read);
        std::vector<float> z(num_read);

        fseek(fp, x_offset + sizeof(float) * start, SEEK_SET);
        fread(x.data(), sizeof(float), num_read, fp);

        fseek(fp, y_offset + sizeof(float) * start, SEEK_SET);
        fread(y.data(), sizeof(float), num_read, fp);

        fseek(fp, z_offset + sizeof(float) * start, SEEK_SET);
        fread(z.data(), sizeof(float), num_read, fp);
        for (int i = 0; i < num_read; i++) points[i] = {x[i], y[i], z[i]};
        return points;
    }

    std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override {
        long num_read = end - start;
        std::vector<int> cell_nodes_int(num_read * 8);
        fseek(fp, hex_nodes_offset + sizeof(int) * 8 * start, SEEK_SET);
        fread(cell_nodes_int.data(), sizeof(int), num_read * 8, fp);

        std::vector<long> cell_nodes_long(num_read * 8);
        for (size_t i = 0; i < cell_nodes_long.size(); i++) {
            cell_nodes_long[i] = cell_nodes_int[i] - 1;
        }
        return cell_nodes_long;
    }
    std::vector<int> readCellTags(inf::MeshInterface::CellType element_type, long start, long end) const override {
        std::vector<int> tags(end - start, 0);

        return tags;
    }
    std::vector<inf::MeshInterface::CellType> cellTypes() const override { return {inf::MeshInterface::HEXA_8}; }

  private:
    FILE* fp;
    int node_count;
    long x_offset;
    long y_offset;
    long z_offset;
    long hex_nodes_offset;

    std::map<inf::MeshInterface::CellType, long> cell_type_count;
};

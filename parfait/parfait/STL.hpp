
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
#include <math.h>
#include <exception>
#include <stdexcept>
#include <queue>
#include "Extent.h"
#include "Point.h"
#include "STLReader.h"
#include "ObjReader.h"
#include "StringTools.h"
#include "VectorTools.h"

namespace Parfait {
namespace STL {

    inline void rescale(double scale, std::vector<Facet>& facets) {
        for (auto& facet : facets) {
            for (unsigned int j = 0; j < 3; j++) {
                facet[0][j] *= scale;
            }
            for (unsigned int j = 0; j < 3; j++) {
                facet[1][j] *= scale;
            }
            for (unsigned int j = 0; j < 3; j++) {
                facet[2][j] *= scale;
            }
        }
    }

    inline void scaleToUnitLength(std::vector<Facet>& facets) {
        auto d = findDomain(facets);
        scaleToUnitLength(d, facets);
    }

    inline void scaleToUnitLength(const Extent<double>& d, std::vector<Facet>& facets) {
        translateCenterToOrigin(d, facets);
        double length = getLongestCartesianLength(d);
        rescale(1.0 / length, facets);
    }

    inline double getLongestCartesianLength(const Extent<double>& d) {
        double dx = d.hi[0] - d.lo[0];
        double dy = d.hi[1] - d.lo[1];
        double dz = d.hi[2] - d.lo[2];
        double max = -20e20;
        max = (max > dx) ? (max) : (dx);
        max = (max > dy) ? (max) : (dy);
        max = (max > dz) ? (max) : (dz);
        return max;
    }

    inline double getLongestCartesianLength(const std::vector<Facet>& facets) {
        Extent<double> domain = findDomain(facets);
        return getLongestCartesianLength(domain);
    }

    inline void translateCenterToOrigin(const Extent<double>& d, std::vector<Facet>& facets) {
        double x_mid = 0.5 * (d.lo[0] + d.hi[0]);
        double y_mid = 0.5 * (d.lo[1] + d.hi[1]);
        double z_mid = 0.5 * (d.lo[2] + d.hi[2]);

        for (auto& f : facets) {
            for (int i = 0; i < 3; i++) {
                f[i][0] -= x_mid;
                f[i][1] -= y_mid;
                f[i][2] -= z_mid;
            }
        }
    }

    inline void translateCenterToOrigin(std::vector<Facet>& facets) {
        auto d = findDomain(facets);
        return translateCenterToOrigin(d, facets);
    }

    inline std::vector<Facet> load(std::string filename) {
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension == "obj") {
            return ObjReader::read(filename);
        }

        BinaryReader reader(filename);
        if (reader.isConsistent()) {
            return reader.readFacets();
        } else {
            AsciiReader reader(filename);
            return reader.readFacets();
        }
    }

    inline std::vector<Facet> loadBinaryFile(std::string fileName) {
        BinaryReader reader(fileName);
        return reader.readFacets();
    }

    template <typename Container>
    void write(const Container& facets, std::string filename, std::string solid_name) {
        writeBinaryFile(facets, filename, solid_name);
    }

    template <typename Container>
    void writeBinaryFile(const Container& facets, std::string filename, std::string solid_name) {
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension != "stl") filename += ".stl";
        FILE* fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for writing. " + filename);
        char buffer[80];
        printf("\nWriting stl to <%s>", filename.c_str());
        size_t length = std::min(solid_name.size() + 1, static_cast<size_t>(80));
        solid_name.copy(buffer, length);
        fwrite(buffer, sizeof(char), 80, fp);
        int num_facets = facets.size();
        fwrite(&num_facets, sizeof(int), 1, fp);

        std::array<float, 12> out_float_buff;
        char buff[2];
        for (auto& f : facets) {
            out_float_buff[0] = f.normal[0];
            out_float_buff[1] = f.normal[1];
            out_float_buff[2] = f.normal[2];
            out_float_buff[3] = f[0][0];
            out_float_buff[4] = f[0][1];
            out_float_buff[5] = f[0][2];
            out_float_buff[6] = f[1][0];
            out_float_buff[7] = f[1][1];
            out_float_buff[8] = f[1][2];
            out_float_buff[9] = f[2][0];
            out_float_buff[10] = f[2][1];
            out_float_buff[11] = f[2][2];
            fwrite(out_float_buff.data(), sizeof(float), 12, fp);
            fwrite(buff, sizeof(char), 2, fp);
        }
        fclose(fp);
    }

    inline void writeAsciiFile(const std::vector<Facet>& facets, std::string fname, std::string solidName) {
        auto extension = Parfait::StringTools::getExtension(fname);
        if (extension != "stl") fname += ".stl";
        FILE* fp = fopen(fname.c_str(), "w");
        assert(fp != nullptr);
        printf("\nWriting stl to <%s>", fname.c_str());
        fflush(stdout);

        fprintf(fp, "solid %s", solidName.c_str());
        for (const auto& facet : facets) {
            fprintf(fp, "\nfacet normal %lf %lf %lf", facet.normal[0], facet.normal[1], facet.normal[2]);
            fprintf(fp, "\nouter loop");
            fprintf(fp, "\nvertex %lf %lf %lf", facet[0][0], facet[0][1], facet[0][2]);
            fprintf(fp, "\nvertex %lf %lf %lf", facet[1][0], facet[1][1], facet[1][2]);
            fprintf(fp, "\nvertex %lf %lf %lf", facet[2][0], facet[2][1], facet[2][2]);
            fprintf(fp, "\nendloop\nendfacet");
        }
        fprintf(fp, "\nendsolid %s", solidName.c_str());

        fclose(fp);
    }
    inline std::vector<std::vector<int>> buildNodeToNode(const std::vector<int>& triangle_node_ids, int node_count) {
        std::vector<std::vector<int>> node_to_node(node_count);
        int num_facets = triangle_node_ids.size() / 3;
        for (int i = 0; i < num_facets; i++) {
            int node0 = triangle_node_ids[3 * i + 0];
            int node1 = triangle_node_ids[3 * i + 1];
            int node2 = triangle_node_ids[3 * i + 2];
            Parfait::VectorTools::insertUnique(node_to_node[node0], node1);
            Parfait::VectorTools::insertUnique(node_to_node[node0], node2);

            Parfait::VectorTools::insertUnique(node_to_node[node1], node0);
            Parfait::VectorTools::insertUnique(node_to_node[node1], node2);

            Parfait::VectorTools::insertUnique(node_to_node[node2], node0);
            Parfait::VectorTools::insertUnique(node_to_node[node2], node1);
        }
        return node_to_node;
    }

    inline std::vector<std::vector<int>> buildNodeToCell(const std::vector<int>& triangle_node_ids, int node_count) {
        std::vector<std::vector<int>> n2c(node_count);
        int num_facets = triangle_node_ids.size() / 3;
        for (int triangle_id = 0; triangle_id < num_facets; triangle_id++) {
            int node0 = triangle_node_ids[3 * triangle_id + 0];
            int node1 = triangle_node_ids[3 * triangle_id + 1];
            int node2 = triangle_node_ids[3 * triangle_id + 2];
            Parfait::VectorTools::insertUnique(n2c[node0], triangle_id);
            Parfait::VectorTools::insertUnique(n2c[node1], triangle_id);
            Parfait::VectorTools::insertUnique(n2c[node2], triangle_id);
            printf("Building Adjacency: n2c %2.lf%%\r", 100 * (double)triangle_id / (double)num_facets);
        }
        return n2c;
    }

    inline std::vector<Parfait::Facet> convertToFacets(const std::vector<int>& node_ids,
                                                       const std::vector<Parfait::Point<double>>& points) {
        std::vector<Parfait::Facet> facets;
        int num_facets = node_ids.size() / 3;
        for (int i = 0; i < num_facets; i++) {
            int n0 = node_ids[3 * i + 0];
            int n1 = node_ids[3 * i + 1];
            int n2 = node_ids[3 * i + 2];
            facets.emplace_back(points[n0], points[n1], points[n2]);
        }
        return facets;
    }

    inline Parfait::Extent<double> findDomain(const Facet* facets, size_t num_facets) {
        auto domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for (size_t i = 0; i < num_facets; i++) {
            auto& f = facets[i];
            for (int j = 0; j < 3; j++) {
                Parfait::ExtentBuilder::add(domain, f[j]);
            }
        }
        return domain;
    }

    inline Parfait::Extent<double> findDomain(const std::vector<Facet>& facets) {
        auto domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for (auto& f : facets) {
            Parfait::ExtentBuilder::add(domain, f[0]);
            Parfait::ExtentBuilder::add(domain, f[1]);
            Parfait::ExtentBuilder::add(domain, f[2]);
        }
        return domain;
    }
    inline int findFirstUnlabeled(const std::vector<int>& labels) {
        for (size_t i = 0; i < labels.size(); i++) {
            if (labels[i] == -1) {
                return i;
            }
        }
        return -1;
    }

    inline std::pair<std::vector<int>, int> labelShells(const std::vector<int>& triangle_nodes,
                                                        const std::vector<std::vector<int>>& node_to_cell) {
        std::vector<int> labels(triangle_nodes.size() / 3, -1);
        int current_shell = 0;
        int seed = findFirstUnlabeled(labels);
        while (seed != -1) {
            printf("Labeling shell %d\r", current_shell);
            std::queue<int> process;
            labels[seed] = current_shell;
            process.push(seed);
            while (not process.empty()) {
                printf("Amount in queue %lu\r", process.size());
                int tri = process.front();
                process.pop();
                for (int i = 0; i < 3; i++) {
                    int node = triangle_nodes[3 * tri + i];
                    for (auto& neighbor : node_to_cell[node]) {
                        if (labels[neighbor] == -1) {
                            labels[neighbor] = current_shell;
                            process.push(neighbor);
                        }
                    }
                }
            }
            seed = findFirstUnlabeled(labels);
            current_shell++;
        }
        return {labels, current_shell};
    }

    inline std::tuple<std::vector<int>, int> labelShells(const std::vector<int>& triangle_nodes, int node_count) {
        auto n2n = buildNodeToCell(triangle_nodes, node_count);
        return labelShells(triangle_nodes, n2n);
    }

    inline std::vector<std::vector<Parfait::Facet>> splitByLabel(const std::vector<int>& triangle_nodes,
                                                                 const std::vector<Parfait::Point<double>>& points,
                                                                 const std::vector<int>& triangle_labels,
                                                                 int num_labels) {
        std::vector<std::vector<Parfait::Facet>> facets(num_labels);
        for (int label = 0; label < num_labels; label++) {
            for (int t = 0; t < int(triangle_nodes.size()) / 3; t++) {
                if (triangle_labels[t] == label) {
                    Parfait::Facet f;
                    f[0] = points[triangle_nodes[3 * t + 0]];
                    f[1] = points[triangle_nodes[3 * t + 1]];
                    f[2] = points[triangle_nodes[3 * t + 2]];
                    facets[label].push_back(f);
                }
            }
        }
        return facets;
    }
}

inline Extent<double> findDomain(const std::vector<std::array<Parfait::Point<double>, 3>>& facets) {
    auto domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto& facet : facets) {
        for (int i = 0; i < 3; i++) {
            domain.lo[i] = (domain.lo[i] < facet[0][i]) ? (domain.lo[i]) : (facet[0][i]);
            domain.lo[i] = (domain.lo[i] < facet[1][i]) ? (domain.lo[i]) : (facet[1][i]);
            domain.lo[i] = (domain.lo[i] < facet[2][i]) ? (domain.lo[i]) : (facet[2][i]);

            domain.hi[i] = (domain.hi[i] > facet[0][i]) ? (domain.hi[i]) : (facet[0][i]);
            domain.hi[i] = (domain.hi[i] > facet[1][i]) ? (domain.hi[i]) : (facet[1][i]);
            domain.hi[i] = (domain.hi[i] > facet[2][i]) ? (domain.hi[i]) : (facet[2][i]);
        }
    }
    return domain;
}
}

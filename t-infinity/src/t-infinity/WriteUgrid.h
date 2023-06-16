#pragma once
#include <parfait/CellWindingConverters.h>
#include <parfait/StringTools.h>
#include <parfait/ToString.h>
#include <parfait/UgridWriter.h>
#include <t-infinity/MeshInterface.h>
#include <memory>
#include <string>
#include <vector>

namespace inf {
void writeUgrid(std::string filename, std::shared_ptr<inf::MeshInterface> mesh) {
    int num_tri = 0, num_quad = 0, num_tet = 0, num_pyr = 0, num_prism = 0, num_hex = 0;
    for (int c = 0; c < mesh->cellCount(); c++) {
        auto type = mesh->cellType(c);
        switch (type) {
            case inf::MeshInterface::TRI_3:
                num_tri++;
                break;
            case inf::MeshInterface::QUAD_4:
                num_quad++;
                break;
            case inf::MeshInterface::TETRA_4:
                num_tet++;
                break;
            case inf::MeshInterface::PYRA_5:
                num_pyr++;
                break;
            case inf::MeshInterface::PENTA_6:
                num_prism++;
                break;
            case inf::MeshInterface::HEXA_8:
                num_hex++;
                break;
            default:
                PARFAIT_WARNING("Found an unsupported element type: " +
                                inf::MeshInterface::cellTypeString(type) + " skipping.");
        }
    }

    std::vector<int> tri;
    tri.reserve(3 * num_tri);
    std::vector<int> quad;
    quad.reserve(4 * num_quad);
    std::vector<int> tet;
    tet.reserve(4 * num_tet);
    std::vector<int> pyramid;
    pyramid.reserve(5 * num_pyr);
    std::vector<int> prism;
    prism.reserve(6 * num_prism);
    std::vector<int> hex;
    hex.reserve(8 * num_hex);
    std::vector<int> tri_tags, quad_tags;

    std::vector<int> cell(8);
    for (int c = 0; c < mesh->cellCount(); c++) {
        auto type = mesh->cellType(c);
        switch (type) {
            case inf::MeshInterface::TRI_3:
                cell.resize(3);
                mesh->cell(c, cell.data());
                for (auto n : cell) {
                    tri.push_back(n);
                }
                tri_tags.push_back(mesh->cellTag(c));
                break;
            case inf::MeshInterface::QUAD_4:
                cell.resize(4);
                mesh->cell(c, cell.data());
                for (auto n : cell) {
                    quad.push_back(n);
                }
                quad_tags.push_back(mesh->cellTag(c));
                break;
            case inf::MeshInterface::TETRA_4:
                cell.resize(4);
                mesh->cell(c, cell.data());
                Parfait::CGNSToAflr::convertTet(cell.data());
                for (auto n : cell) {
                    tet.push_back(n);
                }
                break;
            case inf::MeshInterface::PYRA_5:
                cell.resize(5);
                mesh->cell(c, cell.data());
                Parfait::CGNSToAflr::convertPyramid(cell.data());
                for (auto n : cell) {
                    pyramid.push_back(n);
                }
                break;
            case inf::MeshInterface::PENTA_6:
                cell.resize(6);
                mesh->cell(c, cell.data());
                Parfait::CGNSToAflr::convertPrism(cell.data());
                for (auto n : cell) {
                    prism.push_back(n);
                }
                break;
            case inf::MeshInterface::HEXA_8:
                cell.resize(8);
                mesh->cell(c, cell.data());
                Parfait::CGNSToAflr::convertHex(cell.data());
                for (auto n : cell) {
                    hex.push_back(n);
                }
                break;
            default:
                PARFAIT_THROW("Found an unsupported element type");
        }
    }

    std::vector<Parfait::Point<double>> nodes(mesh->nodeCount());
    for (size_t i = 0; i < nodes.size(); i++) {
        nodes[i] = mesh->node(i);
    }
    bool swapBytes = false;
    Parfait::UgridWriter::writeHeader(filename,
                                      mesh->nodeCount(),
                                      num_tri,
                                      num_quad,
                                      num_tet,
                                      num_pyr,
                                      num_prism,
                                      num_hex,
                                      swapBytes);

    Parfait::UgridWriter::writeNodes(filename, nodes.size(), nodes.front().data(), swapBytes);
    Parfait::UgridWriter::writeTriangles(filename, num_tri, tri.data(), swapBytes);
    Parfait::UgridWriter::writeQuads(filename, num_quad, quad.data(), swapBytes);
    Parfait::UgridWriter::writeTriangleBoundaryTags(filename, num_tri, tri_tags.data(), swapBytes);
    Parfait::UgridWriter::writeQuadBoundaryTags(filename, num_quad, quad_tags.data(), swapBytes);
    Parfait::UgridWriter::writeTets(filename, num_tet, tet.data(), swapBytes);
    Parfait::UgridWriter::writePyramids(filename, num_pyr, pyramid.data(), swapBytes);
    Parfait::UgridWriter::writePrisms(filename, num_prism, prism.data(), swapBytes);
    Parfait::UgridWriter::writeHexs(filename, num_hex, hex.data(), swapBytes);
}
}

#pragma once
#include <set>
#include <vector>
#include "NaiveMesh.h"

class GraphBuilder {
  public:
    GraphBuilder(const NaiveMesh& mesh);

    std::vector<std::vector<long>> build();

  private:
    const NaiveMesh& mesh;
    std::vector<bool> do_own;

    void addCellEdges(std::vector<std::set<long>>& n2n, const std::vector<long>& cell) const;
    std::vector<std::vector<long>> flattenGraph(const std::vector<std::set<long>>& n2n) const;
    void addEdgesForCellType(const inf::MeshInterface::CellType& type, std::vector<std::set<long>>& n2n) const;
};
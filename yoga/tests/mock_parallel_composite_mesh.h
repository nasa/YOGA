#ifndef MOCK_PARALLEL_COMPOSITE_MESH
#define MOCK_PARALLEL_COMPOSITE_MESH

class MockParallelCompositeMesh {
  public:
    int numberOfNodes() { return 0; }
    int numberOfNonGhostNodes() { return 0; }
    void getNode(int id, double* p) {}
    int getGlobalNodeId(int id) { return 0; }
    int getImesh(int id) { return 0; }
    int numberOfBoundaryFaces() { return 0; }
    int numberOfNodesInBoundaryFace(int id) { return 0; }
    void getNodesInBoundaryFace(int id, int* face) {}
    int getBoundaryCondition(int id) { return 0; }

  private:
    std::vector<double> nodes{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};
};

#endif

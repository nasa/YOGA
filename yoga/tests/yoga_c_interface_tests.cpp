#include <RingAssertions.h>
#include "yoga_c_interface.h"

namespace c_interface_mocks {
int comm = 0;
int nnodes() { return 0; }
int ncells() { return 0; }
int nnodesInCell(int i) { return 0; }
void getNodesInCell(int i, int* nodes) {}
long getGlobalNodeId(int i) { return 0; }
int getComponentId(int i) { return 0; }
void getPoint(int i, double* p) {}
int isGhostNode(int i) { return false; }
int nBoundaryFaces() { return 0; }
int nnodesInFace(int i) { return 0; }
void getNodesInFace(int i, int* face) {}
int getBoundaryCondition(int i) { return 0; }
void getSolutionInCell(double x, double y, double z, int i, double* Q) {}
void getSolutionAtNode(int n, double* Q) {}
}

#if 0
TEST_CASE("Make sure can call stuff"){
  using namespace c_interface_mocks;
 
  auto yoga_instance = yoga_create_instance(comm,
      nnodes,ncells,nnodesInCell,getNodesInCell,
      getGlobalNodeId,getComponentId,
      getPoint,isGhostNode,nBoundaryFaces,
      nnodesInFace,
      getNodesInFace,getBoundaryCondition,
      getSolutionInCell,
	  getSolutionAtNode);

  yoga_perform_domain_assembly(yoga_instance);
 
  REQUIRE_THROWS(yoga_get_node_status(yoga_instance, 0));
  //yoga_get_solution_for_receptor(yoga_instance, 0);
  yoga_cleanup(yoga_instance);
 
}

TEST_CASE("Make sure it doesn't die if you don't perform assembly"){
  using namespace c_interface_mocks;

    auto yoga_instance = yoga_create_instance(comm,
                                              nnodes, ncells, nnodesInCell, getNodesInCell,
                                              getGlobalNodeId, getComponentId,
                                              getPoint, isGhostNode, nBoundaryFaces,
                                              nnodesInFace,
                                              getNodesInFace, getBoundaryCondition,
                                              getSolutionInCell,
											  getSolutionAtNode);

    yoga_cleanup(yoga_instance);
}
#endif

#if 0
void yoga_perform_domain_assembly(void* yoga_instance);
int yoga_get_node_status(void* yoga_instance, int node_id);
void yoga_get_solution_for_receptor(void* yoga_instance, int node_id, double* Q);
void yoga_cleanup(void* yoga_instance);
#endif

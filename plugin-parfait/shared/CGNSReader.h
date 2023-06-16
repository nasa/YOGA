#pragma once
#include <string>
#include <vector>
#include <map>
#include <array>
#include <cgnslib.h>
#include <parfait/Point.h>
#include <stdexcept>
#include <set>
#include <t-infinity/MeshInterface.h>
#include "Reader.h"

class CGNSReader : public Reader {
  private:
    struct Section {
        Section(int id, CGNS_ENUMT(ElementType_t) e, long s, long n, std::string name)
            : id(id), element_type(e), global_element_start(s), global_element_end(n), name(name) {}
        int id;
        CGNS_ENUMT(ElementType_t) element_type;
        long global_element_start;
        long global_element_end;
        std::string name;
    };
    struct ElementAddress {
        ElementAddress(long s, long start, long end, long element_start, long element_end)
            : section(s),
              global_index_start(start),
              global_index_end(end),
              element_start(element_start),
              element_end(element_end) {}
        long section;
        long global_index_start;
        long global_index_end;
        long element_start;
        long element_end;
    };

    class Boundary {
      public:
        Boundary(int id, long start, long end, std::string name) : id(id), start(start), end(end), name(name) {}

        bool containsElement(long element_global_id) const {
            return element_global_id >= start and element_global_id < end;
        }
        int id;
        long start;
        long end;
        std::string name;
    };

  public:
    enum { NILL_TAG = -11 };
    enum { NODES = 0, CELLS = 1 };

    CGNSReader(const std::string& filename);

    ~CGNSReader();

    long nodeCount() const override;
    long cellCount() const;
    int boundaryCount() const;
    long cellCount(inf::MeshInterface::CellType etype) const override;
    int cellTypeLength(inf::MeshInterface::CellType type) const;

    std::vector<Parfait::Point<double>> readCoords() const;
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const override;
    std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override;
    std::vector<int> readCellTags(inf::MeshInterface::CellType cell_type) const;
    std::vector<int> readCellTags(inf::MeshInterface::CellType element_type,
                                  long element_start,
                                  long element_end) const override;

    std::vector<inf::MeshInterface::CellType> cellTypes() const override;

  private:
    const int fortran = 1;
    std::string filename;
    int index_file;
    bool isOpen;
    int index_base;
    int zone_count;
    int current_zone;
    int current_section_count;
    int node_count;
    int cell_count;
    std::set<CGNS_ENUMT(ElementType_t)> volume_element_types;
    std::set<CGNS_ENUMT(ElementType_t)> boundary_element_types;

    std::map<CGNS_ENUMT(ElementType_t), std::vector<ElementAddress>> element_locations;
    std::vector<Boundary> boundary_headers;
    std::vector<inf::MeshInterface::CellType> cell_types;

    void open();
    void close();
    void loadZone(int zone);
    int countZones();
    int sectionCount() const;
    Section loadSection(int section) const;
    std::array<long, 2> sectionRange(int section) const;
    bool doesSectionContainElement(int section, CGNS_ENUMT(ElementType_t)) const;

    std::array<long, 3> entityCount();
    CGNS_ENUMT(ElementType_t) sectionElementType(int section) const;

    void throwIfError(int ier) const;
    void throwIfNot3D();
    void throwIfMultipleBases();
    void throwIfNotUnstructured();
    long countSectionElements(int section) const;
    long extractElementsFromSection(int section, long start, long end, cgsize_t* elements) const;
    long countElementsInSection(const CGNS_ENUMT(ElementType_t) & type, int section) const;
    std::vector<long> countElementsPerSection(CGNS_ENUMT(ElementType_t) type) const;
    long boundaryConditionLength(int b);
    void throwIfNotRange(CGNS_ENUMT(PointSetType_t) type);

    Boundary loadBoundary(int boundary_index);
    void loadElementLocations();
    long elementGlobalId(CGNS_ENUMT(ElementType_t) element_type, long element_index) const;
    void loadBoundaryHeaders();
    std::array<long, 2> elementRangeInBoundary(int i);

    std::set<CGNS_ENUMT(ElementType_t)> volumeTypes();
    std::set<CGNS_ENUMT(ElementType_t)> setBoundaryElementTypes();

    CGNS_ENUMT(ElementType_t) cellTypeToEtype(inf::MeshInterface::CellType type) const;
    std::string elementTypeToString(CGNS_ENUMT(ElementType_t) etype) const;
    int boundaryTag(CGNS_ENUMT(ElementType_t) element_type, long element_index) const;

    void loadCellTypes();

    inf::MeshInterface::CellType etypeToCellType(CGNS_ENUMT(ElementType_t) type);
};
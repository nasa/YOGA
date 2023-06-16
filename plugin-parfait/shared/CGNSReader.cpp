#include <t-infinity/MeshInterface.h>
#include "CGNSReader.h"
#include "SliceAcrossSections.h"
#include "ElementCounter.h"
#include <parfait/Point.h>

#define CGNS_CALL(cgns_func)                                                                          \
    {                                                                                                 \
        auto err = cgns_func;                                                                         \
        if (err != 0) {                                                                               \
            throw std::logic_error("CGNS Error detected at " + std::string(__FILE__) + " " +          \
                                   std::to_string(__LINE__) + "\nError code " + std::to_string(err) + \
                                   "\nMessage: " + cg_get_error());                                   \
        }                                                                                             \
    }

CGNSReader::CGNSReader(const std::string& filename)
    : filename(filename),
      index_file(0),
      isOpen(false),
      index_base(1),
      volume_element_types(volumeTypes()),
      boundary_element_types(setBoundaryElementTypes()) {
    open();
    loadZone(0);
    loadElementLocations();
    loadBoundaryHeaders();
    loadCellTypes();
}

CGNSReader::~CGNSReader() {
    if (isOpen) close();
}

int CGNSReader::sectionCount() const { return current_section_count; }

void CGNSReader::loadZone(int zone) {
    if (zone < 0 or zone >= zone_count) throw std::logic_error("CGNSReader:: Requested invalid zone");
    zone += fortran;
    current_zone = zone;
    CGNS_CALL(cg_goto(index_file, index_base, "Zone_t", current_zone, "end"));
    CGNS_CALL(cg_nsections(index_file, index_base, current_zone, &current_section_count));
    auto count = entityCount();
    node_count = (int)count[NODES];
    cell_count = (int)count[CELLS];
}

std::vector<Parfait::Point<double>> CGNSReader::readCoords() const { return readCoords(0, node_count); }

std::vector<Parfait::Point<double>> CGNSReader::readCoords(long start, long end) const {
    long count = end - start;
    count = std::max(count, long(0));
    std::vector<Parfait::Point<double>> points(count);
    if (points.size() == 0) {
        return points;
    }

    std::vector<double> buffer(count);
    cgsize_t i = start + fortran;
    cgsize_t j = end;
    CGNS_CALL(cg_coord_read(
        index_file, index_base, current_zone, "CoordinateX", CGNS_ENUMT(RealDouble), &i, &j, buffer.data()));
    for (size_t n = 0; n < buffer.size(); n++) points[n][0] = buffer[n];

    CGNS_CALL(cg_coord_read(
        index_file, index_base, current_zone, "CoordinateY", CGNS_ENUMT(RealDouble), &i, &j, buffer.data()));

    for (size_t n = 0; n < buffer.size(); n++) points[n][1] = buffer[n];

    CGNS_CALL(cg_coord_read(
        index_file, index_base, current_zone, "CoordinateZ", CGNS_ENUMT(RealDouble), &i, &j, buffer.data()));
    for (size_t n = 0; n < buffer.size(); n++) points[n][2] = buffer[n];

    return points;
}

int CGNSReader::boundaryCount() const {
    CGNS_CALL(cg_goto(index_file, index_base, "Zone_t", 1, "ZoneBC_t", 1, "end"));
    int boundary_condition_count;
    CGNS_CALL(cg_nbocos(index_file, index_base, current_zone, &boundary_condition_count));
    return boundary_condition_count;
}

std::array<long, 2> CGNSReader::elementRangeInBoundary(int boundary_index) {
    auto length = boundaryConditionLength(boundary_index);

    std::array<cgsize_t, 2> boundary_elements;
    std::vector<std::array<double, 3>> normal_list(length);
    boundary_index += fortran;
    CGNS_CALL(cg_boco_read(
        index_file, index_base, current_zone, boundary_index, boundary_elements.data(), normal_list.data()));
    boundary_elements[1] += fortran;
    std::array<long, 2> out;
    out[0] = boundary_elements[0];
    out[1] = boundary_elements[1];
    return out;
}

long CGNSReader::boundaryConditionLength(int b) {
    b += fortran;
    char name[256];
    CGNS_ENUMT(BCType_t) ibocotype;
    CGNS_ENUMT(PointSetType_t) set_type;
    cgsize_t npts;
    cgsize_t normallistflag;
    int ndataset;
    std::array<int, 3> normalindex;
    CGNS_ENUMT(DataType_t) normal_data_type;
    CGNS_CALL(cg_boco_info(index_file,
                           index_base,
                           current_zone,
                           b,
                           name,
                           &ibocotype,
                           &set_type,
                           &npts,
                           normalindex.data(),
                           &normallistflag,
                           &normal_data_type,
                           &ndataset));
    throwIfNotRange(set_type);
    if (npts != 2)
        throw std::logic_error("the length of a boundary condition object is not 2.  It should just be a range");
    return npts;
}

CGNSReader::Boundary CGNSReader::loadBoundary(int boundary_index) {
    auto length = boundaryConditionLength(boundary_index);
    boundary_index += fortran;
    char name[256];
    CGNS_ENUMT(BCType_t) ibocotype;
    CGNS_ENUMT(PointSetType_t) set_type;
    cgsize_t npts;
    cgsize_t normallistflag;
    int ndataset;
    std::array<int, 3> normalindex;
    CGNS_ENUMT(DataType_t) normal_data_type;
    CGNS_CALL(cg_boco_info(index_file,
                           index_base,
                           current_zone,
                           boundary_index,
                           name,
                           &ibocotype,
                           &set_type,
                           &npts,
                           normalindex.data(),
                           &normallistflag,
                           &normal_data_type,
                           &ndataset));

    std::array<cgsize_t, 2> boundary_range;
    std::vector<std::array<double, 3>> normal_list(length);
    CGNS_CALL(
        cg_boco_read(index_file, index_base, current_zone, boundary_index, boundary_range.data(), normal_list.data()));
    throwIfNotRange(set_type);
    Boundary boundary(boundary_index - fortran, boundary_range[0] - fortran, boundary_range[1], name);
    return boundary;
}

std::string CGNSReader::elementTypeToString(CGNS_ENUMT(ElementType_t) etype) const {
    switch (etype) {
        case CGNS_ENUMT(TRI_3):
            return "TRI_3";
        case CGNS_ENUMT(TRI_6):
            return "TRI_6";
        case CGNS_ENUMT(TRI_10):
            return "TRI_10";
        case CGNS_ENUMT(TRI_15):
            return "TRI_15";
        case CGNS_ENUMT(QUAD_4):
            return "QUAD_4";
        case CGNS_ENUMT(QUAD_9):
            return "QUAD_9";
        case CGNS_ENUMT(QUAD_16):
            return "QUAD_16";
        case CGNS_ENUMT(QUAD_25):
            return "QUAD_25";
        case CGNS_ENUMT(TETRA_4):
            return "TET_4";
        case CGNS_ENUMT(TETRA_10):
            return "TET_10";
        case CGNS_ENUMT(TETRA_20):
            return "TET_20";
        case CGNS_ENUMT(TETRA_35):
            return "TET_35";
        case CGNS_ENUMT(PYRA_5):
            return "PYRAMID_5";
        case CGNS_ENUMT(PYRA_14):
            return "PYRAMID_14";
        case CGNS_ENUMT(PYRA_30):
            return "PYRAMID_30";
        case CGNS_ENUMT(PYRA_55):
            return "PYRAMID_55";
        case CGNS_ENUMT(PENTA_6):
            return "PENTA_6";
        case CGNS_ENUMT(PENTA_15):
            return "PENTA_15";
        case CGNS_ENUMT(PENTA_18):
            return "PENTA_18";
        case CGNS_ENUMT(PENTA_40):
            return "PENTA_40";
        case CGNS_ENUMT(PENTA_75):
            return "PENTA_75";
        case CGNS_ENUMT(HEXA_8):
            return "HEXA_8";
        case CGNS_ENUMT(HEXA_27):
            return "HEXA_27";
        case CGNS_ENUMT(HEXA_64):
            return "HEXA_64";
        case CGNS_ENUMT(HEXA_125):
            return "HEXA_125";
        default:
            return "UNKNOWN_ETYPE";
    }
}

CGNS_ENUMT(ElementType_t) CGNSReader::sectionElementType(int section) const {
    section += fortran;
    char section_name[256];
    CGNS_ENUMT(ElementType_t) elementType;
    cgsize_t nstart, nend;
    int nboundary, flag;
    CGNS_CALL(cg_section_read(
        index_file, index_base, current_zone, section, section_name, &elementType, &nstart, &nend, &nboundary, &flag));
    return elementType;
}

CGNSReader::Section CGNSReader::loadSection(int section_id) const {
    section_id += fortran;
    char section_name[256];
    CGNS_ENUMT(ElementType_t) elementType;
    cgsize_t nstart, nend;
    int nboundary, flag;
    CGNS_CALL(cg_section_read(index_file,
                              index_base,
                              current_zone,
                              section_id,
                              section_name,
                              &elementType,
                              &nstart,
                              &nend,
                              &nboundary,
                              &flag));
    Section section(section_id - 1, elementType, nstart - fortran, nend, section_name);
    return section;
}
void CGNSReader::close() {
    int err = cg_close(index_file);
    throwIfError(err);
    isOpen = false;
}
void CGNSReader::open() {
    CGNS_CALL(cg_open(filename.c_str(), CG_MODE_READ, &index_file));
    isOpen = true;

    throwIfMultipleBases();
    throwIfNot3D();
    throwIfNotUnstructured();
    zone_count = countZones();
}

std::array<long, 3> CGNSReader::entityCount() {
    char zone_name[256];
    std::array<cgsize_t, 3> isize;
    CGNS_CALL(cg_zone_read(index_file, index_base, current_zone, zone_name, isize.data()));
    std::array<long, 3> out;
    for (int i = 0; i < 3; i++) out[i] = isize[i];
    return out;
}

void CGNSReader::throwIfError(int ier) const {}

void CGNSReader::throwIfNot3D() {
    char basename[256];
    int celldim, physicaldim;
    CGNS_CALL(cg_base_read(index_file, index_base, basename, &celldim, &physicaldim));
    if (celldim != 3) throw std::logic_error("CGNSReader:: celldim is not 3d.");
    if (physicaldim != 3) throw std::logic_error("CGNSReader:: physicalim is not 3d.");
}

int CGNSReader::countZones() {
    int nzones;
    CGNS_CALL(cg_nzones(index_file, index_base, &nzones));
    if (nzones != 1) throw std::logic_error("CGNSReader:: multiple zones found, but not supported.");
    return nzones;
}

void CGNSReader::throwIfMultipleBases() {
    int nbases;
    CGNS_CALL(cg_nbases(index_file, &nbases));
    if (nbases != 1) throw std::logic_error("CGNS file does not have 1 base (not that anyone knows what that means)");
}

void CGNSReader::throwIfNotUnstructured() {
    int index_zone = 1;
    char zone_name[256];
    std::array<cgsize_t, 3> isize;
    CGNS_CALL(cg_zone_read(index_file, index_base, index_zone, zone_name, isize.data()));
    CGNS_ENUMT(ZoneType_t) zone_type;
    CGNS_CALL(cg_zone_type(index_file, index_base, index_zone, &zone_type));
    if (zone_type != CGNS_ENUMT(Unstructured))
        throw std::logic_error("CGNSReader:: non-unstructured grid found but not supported.");
}

long CGNSReader::cellCount() const { return cell_count; }

long CGNSReader::countElementsInSection(const CGNS_ENUMT(ElementType_t) & type, int section) const {
    if (doesSectionContainElement(section, type)) return countSectionElements(section);
    return 0;
}

bool CGNSReader::doesSectionContainElement(int section, CGNS_ENUMT(ElementType_t) type) const {
    auto elemnt_type = sectionElementType(section);
    return elemnt_type == type;
}

std::array<long, 2> CGNSReader::sectionRange(int section) const {
    section += fortran;
    char section_name[256];
    CGNS_ENUMT(ElementType_t) elementType;
    cgsize_t nstart, nend;
    int nboundary, flag;
    CGNS_CALL(cg_section_read(
        index_file, index_base, current_zone, section, section_name, &elementType, &nstart, &nend, &nboundary, &flag));

    return std::array<long, 2>{(long)nstart, (long)nend};
}

long CGNSReader::countSectionElements(int section) const {
    section += fortran;
    char section_name[256];
    CGNS_ENUMT(ElementType_t) elementType;
    cgsize_t nstart, nend;
    int nboundary, flag;
    CGNS_CALL(cg_section_read(
        index_file, index_base, current_zone, section, section_name, &elementType, &nstart, &nend, &nboundary, &flag));

    return nend - nstart + fortran;
}

long CGNSReader::extractElementsFromSection(int section, long start, long end, cgsize_t* elements) const {
    auto range = sectionRange(section);
    section += fortran;
    auto count = end - start;
    cgsize_t start_cgns = start + range[0];
    cgsize_t end_cgns = start_cgns + count - 1;
    cgsize_t parent_flag;
    cgsize_t elements_length;

    CGNS_CALL(
        cg_ElementPartialSize(index_file, index_base, current_zone, section, start_cgns, end_cgns, &elements_length));
    CGNS_CALL(cg_elements_partial_read(
        index_file, index_base, current_zone, section, start_cgns, end_cgns, elements, &parent_flag));
    return (long)elements_length;
}

std::vector<long> CGNSReader::readCells(inf::MeshInterface::CellType cell_type, long start, long end) const {
    printf("Reading cgns cells of type %s, between: %ld %ld",
           inf::MeshInterface::cellTypeString(cell_type).c_str(),
           start,
           end);
    fflush(stdout);
    auto type = cellTypeToEtype(cell_type);
    long element_count = end - start;
    element_count = std::max(element_count, long(0));
    auto element_count_per_section = countElementsPerSection(type);
    auto slices = SliceAcrossSections::calcSlicesForRange(element_count_per_section, start, end);

    std::vector<cgsize_t> elements(element_count * cellTypeLength(cell_type));
    if (elements.size() == 0) return std::vector<long>();

    long offset = 0;
    for (auto slice : slices) {
        printf("From section %d, reading between %ld %ld\n", slice.section, slice.start, slice.end);
        fflush(stdout);
        auto buffer_length = extractElementsFromSection(slice.section, slice.start, slice.end, &elements[offset]);
        offset += buffer_length;
    }

    for (auto& n : elements) {
        n -= fortran;
    }

    std::vector<long> out(elements.begin(), elements.end());
    return out;
}

std::vector<long> CGNSReader::countElementsPerSection(CGNS_ENUMT(ElementType_t) type) const {
    std::vector<long> count(sectionCount());
    for (size_t s = 0; s < count.size(); s++) {
        count[s] = countElementsInSection(type, s);
    }
    return count;
}

void CGNSReader::throwIfNotRange(CGNS_ENUMT(PointSetType_t) type) {
    // Don't believe the PointRange type, it isn't a point.
    // It's an element type.  Terrible name.
    // Anyway, you can either list out each element in a boundary explicitly element by element.
    // Or you can define simply a range of elements.
    // We only are supporting a range out of the box.
    // If you're got this exception.  It _probably_ means that the type is actually PointList.
    // Okay, if you found out that it _is_ point range, you have to change the logic up to identify boundaries
    // based on a list of boundary elements.  And it's probably going to be in some really confusing
    // global ordering.  Good luck!
    //
    if (type != CGNS_ENUMT(PointRange)) {
        std::string message = "CGNSReader boundary condition set type is not range.\n";
        if (type == CGNS_ENUMT(ElementList))
            message.append("  type is ElementList");
        else if (type == CGNS_ENUMT(ElementRange))
            message.append("  type is ElementRange");
        throw std::logic_error(message);
    }
}

long CGNSReader::nodeCount() const { return node_count; }

long CGNSReader::cellCount(inf::MeshInterface::CellType cell_type) const {
    CGNS_ENUMT(ElementType_t) etype = cellTypeToEtype(cell_type);
    if (element_locations.count(etype) == 0) return 0;
    auto locations = element_locations.at(etype);
    long count = 0;
    for (auto l : locations) {
        count += (l.global_index_end - l.global_index_start);
    }
    printf("Element Type: %s has %ld cells\n", inf::MeshInterface::cellTypeString(cell_type).c_str(), count);
    fflush(stdout);
    return count;
}

void CGNSReader::loadCellTypes() {
    std::set<inf::MeshInterface::CellType> type_set;
    for (auto& location : element_locations) {
        inf::MeshInterface::CellType cell_type = etypeToCellType(location.first);
        type_set.insert(cell_type);
    }

    cell_types = std::vector<inf::MeshInterface::CellType>(type_set.begin(), type_set.end());
}

void CGNSReader::loadElementLocations() {
    int section_count = sectionCount();
    ElementCounter element_counter;
    for (int s = 0; s < section_count; s++) {
        auto section = loadSection(s);
        long element_index = element_counter.currentCount(section.element_type);
        long delta = section.global_element_end - section.global_element_start;
        element_locations[section.element_type].emplace_back(ElementAddress{
            s, section.global_element_start, section.global_element_end, element_index, element_index + delta});
        element_counter.setCounter(section.element_type, element_index + delta);
    }
}

long CGNSReader::elementGlobalId(CGNS_ENUMT(ElementType_t) element_type, long element_index) const {
    auto location = element_locations.at(element_type);
    for (auto l : location) {
        if (element_index >= l.element_start and element_index < l.element_end) {
            long delta = element_index - l.element_start;
            return l.global_index_start + delta;
        }
    }
    throw std::logic_error("could not find element " + std::to_string(element_index) + " of type" +
                           elementTypeToString(element_type));
}

int CGNSReader::boundaryTag(CGNS_ENUMT(ElementType_t) element_type, long element_index) const {
    long gid = elementGlobalId(element_type, element_index);
    for (auto& b : boundary_headers) {
        if (b.containsElement(gid)) {
            return b.id;
        }
    }
    throw std::logic_error("Could not find boundary tag for global element " + std::to_string(element_index) +
                           " type: " + elementTypeToString(element_type));
}

void CGNSReader::loadBoundaryHeaders() {
    int boundary_count = boundaryCount();
    for (int b = 0; b < boundary_count; b++) {
        boundary_headers.emplace_back(loadBoundary(b));
    }
}

std::vector<int> CGNSReader::readCellTags(inf::MeshInterface::CellType cell_type) const {
    return readCellTags(cell_type, 0, cellCount(cell_type));
}

std::vector<int> CGNSReader::readCellTags(inf::MeshInterface::CellType cell_type,
                                          long element_start,
                                          long element_end) const {
    auto element_type = cellTypeToEtype(cell_type);
    std::vector<int> tags(element_end - element_start);
    if (boundary_element_types.count(element_type) != 0) {
        for (long eid = element_start; eid < element_end; eid++) {
            long index = eid - element_start;
            int tag = boundaryTag(element_type, eid);
            tags[index] = tag;
        }
    } else
        tags = std::vector<int>(element_end - element_start, NILL_TAG);
    return tags;
}

std::set<CGNS_ENUMT(ElementType_t)> CGNSReader::volumeTypes() {
    std::set<CGNS_ENUMT(ElementType_t)> volume_types;
    volume_types.insert(CGNS_ENUMT(TETRA_4));
    volume_types.insert(CGNS_ENUMT(TETRA_10));
    volume_types.insert(CGNS_ENUMT(TETRA_16));
    volume_types.insert(CGNS_ENUMT(TETRA_20));
    volume_types.insert(CGNS_ENUMT(TETRA_22));
    volume_types.insert(CGNS_ENUMT(TETRA_34));
    volume_types.insert(CGNS_ENUMT(TETRA_35));
    volume_types.insert(CGNS_ENUMT(PYRA_5));
    volume_types.insert(CGNS_ENUMT(PYRA_13));
    volume_types.insert(CGNS_ENUMT(PYRA_14));
    volume_types.insert(CGNS_ENUMT(PYRA_21));
    volume_types.insert(CGNS_ENUMT(PYRA_29));
    volume_types.insert(CGNS_ENUMT(PYRA_30));
    volume_types.insert(CGNS_ENUMT(PYRA_50));
    volume_types.insert(CGNS_ENUMT(PYRA_55));
    volume_types.insert(CGNS_ENUMT(PYRA_P4_29));
    volume_types.insert(CGNS_ENUMT(PENTA_6));
    volume_types.insert(CGNS_ENUMT(PENTA_15));
    volume_types.insert(CGNS_ENUMT(PENTA_18));
    volume_types.insert(CGNS_ENUMT(PENTA_24));
    volume_types.insert(CGNS_ENUMT(PENTA_33));
    volume_types.insert(CGNS_ENUMT(PENTA_38));
    volume_types.insert(CGNS_ENUMT(PENTA_40));
    volume_types.insert(CGNS_ENUMT(PENTA_66));
    volume_types.insert(CGNS_ENUMT(PENTA_75));
    volume_types.insert(CGNS_ENUMT(HEXA_8));
    volume_types.insert(CGNS_ENUMT(HEXA_20));
    volume_types.insert(CGNS_ENUMT(HEXA_27));
    volume_types.insert(CGNS_ENUMT(HEXA_32));
    volume_types.insert(CGNS_ENUMT(HEXA_44));
    volume_types.insert(CGNS_ENUMT(HEXA_56));
    volume_types.insert(CGNS_ENUMT(HEXA_64));
    volume_types.insert(CGNS_ENUMT(HEXA_98));
    volume_types.insert(CGNS_ENUMT(HEXA_125));
    return volume_types;
}

std::set<CGNS_ENUMT(ElementType_t)> CGNSReader::setBoundaryElementTypes() {
    std::set<CGNS_ENUMT(ElementType_t)> types;
    types.insert(CGNS_ENUMT(TRI_3));
    types.insert(CGNS_ENUMT(TRI_6));
    types.insert(CGNS_ENUMT(TRI_9));
    types.insert(CGNS_ENUMT(TRI_10));
    types.insert(CGNS_ENUMT(TRI_12));
    types.insert(CGNS_ENUMT(TRI_15));
    types.insert(CGNS_ENUMT(QUAD_4));
    types.insert(CGNS_ENUMT(QUAD_8));
    types.insert(CGNS_ENUMT(QUAD_9));
    types.insert(CGNS_ENUMT(QUAD_12));
    types.insert(CGNS_ENUMT(QUAD_16));
    types.insert(CGNS_ENUMT(QUAD_25));
    types.insert(CGNS_ENUMT(QUAD_P4_16));
    return types;
}

int CGNSReader::cellTypeLength(inf::MeshInterface::CellType cell_type) const {
    auto type = cellTypeToEtype(cell_type);
    switch (type) {
        case CGNS_ENUMT(TRI_3):
            return 3;
        case CGNS_ENUMT(TRI_6):
            return 6;
        case CGNS_ENUMT(TRI_10):
            return 10;
        case CGNS_ENUMT(TRI_15):
            return 15;
        case CGNS_ENUMT(QUAD_4):
            return 8;
        case CGNS_ENUMT(QUAD_9):
            return 9;
        case CGNS_ENUMT(QUAD_16):
            return 16;
        case CGNS_ENUMT(QUAD_25):
            return 25;
        case CGNS_ENUMT(TETRA_4):
            return 4;
        case CGNS_ENUMT(TETRA_10):
            return 10;
        case CGNS_ENUMT(TETRA_20):
            return 20;
        case CGNS_ENUMT(TETRA_35):
            return 35;
        case CGNS_ENUMT(PYRA_5):
            return 5;
        case CGNS_ENUMT(PYRA_14):
            return 14;
        case CGNS_ENUMT(PYRA_30):
            return 30;
        case CGNS_ENUMT(PYRA_55):
            return 55;
        case CGNS_ENUMT(PENTA_6):
            return 6;
        case CGNS_ENUMT(PENTA_18):
            return 18;
        case CGNS_ENUMT(PENTA_40):
            return 40;
        case CGNS_ENUMT(PENTA_75):
            return 75;
        case CGNS_ENUMT(HEXA_8):
            return 8;
        case CGNS_ENUMT(HEXA_27):
            return 27;
        case CGNS_ENUMT(HEXA_64):
            return 64;
        case CGNS_ENUMT(HEXA_125):
            return 125;
        default:
            throw std::logic_error("UNKNOWN_ETYPE");
    }
}

CGNS_ENUMT(ElementType_t) CGNSReader::cellTypeToEtype(inf::MeshInterface::CellType type) const {
    switch (type) {
        case inf::MeshInterface::CellType::TRI_3:
            return CGNS_ENUMT(TRI_3);
        case inf::MeshInterface::CellType::TRI_6:
            return CGNS_ENUMT(TRI_6);
        case inf::MeshInterface::CellType::TRI_9:
            return CGNS_ENUMT(TRI_9);
        case inf::MeshInterface::CellType::TRI_10:
            return CGNS_ENUMT(TRI_10);
        case inf::MeshInterface::CellType::TRI_12:
            return CGNS_ENUMT(TRI_12);
        case inf::MeshInterface::CellType::TRI_15:
            return CGNS_ENUMT(TRI_15);
        case inf::MeshInterface::CellType::QUAD_4:
            return CGNS_ENUMT(QUAD_4);
        case inf::MeshInterface::CellType::QUAD_8:
            return CGNS_ENUMT(QUAD_8);
        case inf::MeshInterface::CellType::QUAD_9:
            return CGNS_ENUMT(QUAD_9);
        case inf::MeshInterface::CellType::QUAD_12:
            return CGNS_ENUMT(QUAD_12);
        case inf::MeshInterface::CellType::QUAD_16:
            return CGNS_ENUMT(QUAD_16);
        case inf::MeshInterface::CellType::QUAD_P4_16:
            return CGNS_ENUMT(QUAD_P4_16);
        case inf::MeshInterface::CellType::QUAD_25:
            return CGNS_ENUMT(QUAD_25);
        case inf::MeshInterface::CellType::TETRA_4:
            return CGNS_ENUMT(TETRA_4);
        case inf::MeshInterface::CellType::TETRA_10:
            return CGNS_ENUMT(TETRA_10);
        case inf::MeshInterface::CellType::TETRA_16:
            return CGNS_ENUMT(TETRA_16);
        case inf::MeshInterface::CellType::TETRA_20:
            return CGNS_ENUMT(TETRA_20);
        case inf::MeshInterface::CellType::TETRA_22:
            return CGNS_ENUMT(TETRA_22);
        case inf::MeshInterface::CellType::TETRA_34:
            return CGNS_ENUMT(TETRA_34);
        case inf::MeshInterface::CellType::TETRA_35:
            return CGNS_ENUMT(TETRA_35);
        case inf::MeshInterface::CellType::PENTA_6:
            return CGNS_ENUMT(PENTA_6);
        case inf::MeshInterface::CellType::PENTA_15:
            return CGNS_ENUMT(PENTA_15);
        case inf::MeshInterface::CellType::PENTA_18:
            return CGNS_ENUMT(PENTA_18);
        case inf::MeshInterface::CellType::PENTA_24:
            return CGNS_ENUMT(PENTA_24);
        case inf::MeshInterface::CellType::PENTA_33:
            return CGNS_ENUMT(PENTA_33);
        case inf::MeshInterface::CellType::PENTA_38:
            return CGNS_ENUMT(PENTA_38);
        case inf::MeshInterface::CellType::PENTA_40:
            return CGNS_ENUMT(PENTA_40);
        case inf::MeshInterface::CellType::PENTA_66:
            return CGNS_ENUMT(PENTA_66);
        case inf::MeshInterface::CellType::PENTA_75:
            return CGNS_ENUMT(PENTA_75);
        case inf::MeshInterface::CellType::PYRA_5:
            return CGNS_ENUMT(PYRA_5);
        case inf::MeshInterface::CellType::PYRA_13:
            return CGNS_ENUMT(PYRA_13);
        case inf::MeshInterface::CellType::PYRA_14:
            return CGNS_ENUMT(PYRA_14);
        case inf::MeshInterface::CellType::PYRA_21:
            return CGNS_ENUMT(PYRA_21);
        case inf::MeshInterface::CellType::PYRA_29:
            return CGNS_ENUMT(PYRA_29);
        case inf::MeshInterface::CellType::PYRA_30:
            return CGNS_ENUMT(PYRA_30);
        case inf::MeshInterface::CellType::PYRA_50:
            return CGNS_ENUMT(PYRA_50);
        case inf::MeshInterface::CellType::PYRA_55:
            return CGNS_ENUMT(PYRA_55);
        case inf::MeshInterface::CellType::PYRA_P4_29:
            return CGNS_ENUMT(PYRA_P4_29);
        case inf::MeshInterface::CellType::HEXA_8:
            return CGNS_ENUMT(HEXA_8);
        case inf::MeshInterface::CellType::HEXA_20:
            return CGNS_ENUMT(HEXA_20);
        case inf::MeshInterface::CellType::HEXA_27:
            return CGNS_ENUMT(HEXA_27);
        case inf::MeshInterface::CellType::HEXA_32:
            return CGNS_ENUMT(HEXA_32);
        case inf::MeshInterface::CellType::HEXA_44:
            return CGNS_ENUMT(HEXA_44);
        case inf::MeshInterface::CellType::HEXA_56:
            return CGNS_ENUMT(HEXA_56);
        case inf::MeshInterface::CellType::HEXA_64:
            return CGNS_ENUMT(HEXA_64);
        case inf::MeshInterface::CellType::HEXA_98:
            return CGNS_ENUMT(HEXA_98);
        case inf::MeshInterface::CellType::HEXA_125:
            return CGNS_ENUMT(HEXA_125);
        default:
            throw std::logic_error("Santa: Cell Type unknown");
    }
}

std::vector<inf::MeshInterface::CellType> CGNSReader::cellTypes() const { return cell_types; }

inf::MeshInterface::CellType CGNSReader::etypeToCellType(CGNS_ENUMT(ElementType_t) type) {
    switch (type) {
        case CGNS_ENUMT(TRI_3):
            return inf::MeshInterface::CellType::TRI_3;
        case CGNS_ENUMT(TRI_6):
            return inf::MeshInterface::CellType::TRI_6;
        case CGNS_ENUMT(TRI_9):
            return inf::MeshInterface::CellType::TRI_9;
        case CGNS_ENUMT(TRI_10):
            return inf::MeshInterface::CellType::TRI_10;
        case CGNS_ENUMT(TRI_12):
            return inf::MeshInterface::CellType::TRI_12;
        case CGNS_ENUMT(TRI_15):
            return inf::MeshInterface::CellType::TRI_15;
        case CGNS_ENUMT(QUAD_4):
            return inf::MeshInterface::CellType::QUAD_4;
        case CGNS_ENUMT(QUAD_8):
            return inf::MeshInterface::CellType::QUAD_8;
        case CGNS_ENUMT(QUAD_9):
            return inf::MeshInterface::CellType::QUAD_9;
        case CGNS_ENUMT(QUAD_12):
            return inf::MeshInterface::CellType::QUAD_12;
        case CGNS_ENUMT(QUAD_16):
            return inf::MeshInterface::CellType::QUAD_16;
        case CGNS_ENUMT(QUAD_P4_16):
            return inf::MeshInterface::CellType::QUAD_P4_16;
        case CGNS_ENUMT(QUAD_25):
            return inf::MeshInterface::CellType::QUAD_25;
        case CGNS_ENUMT(TETRA_4):
            return inf::MeshInterface::CellType::TETRA_4;
        case CGNS_ENUMT(TETRA_10):
            return inf::MeshInterface::CellType::TETRA_10;
        case CGNS_ENUMT(TETRA_16):
            return inf::MeshInterface::CellType::TETRA_16;
        case CGNS_ENUMT(TETRA_20):
            return inf::MeshInterface::CellType::TETRA_20;
        case CGNS_ENUMT(TETRA_22):
            return inf::MeshInterface::CellType::TETRA_22;
        case CGNS_ENUMT(TETRA_34):
            return inf::MeshInterface::CellType::TETRA_34;
        case CGNS_ENUMT(TETRA_35):
            return inf::MeshInterface::CellType::TETRA_35;
        case CGNS_ENUMT(PENTA_6):
            return inf::MeshInterface::CellType::PENTA_6;
        case CGNS_ENUMT(PENTA_15):
            return inf::MeshInterface::CellType::PENTA_15;
        case CGNS_ENUMT(PENTA_18):
            return inf::MeshInterface::CellType::PENTA_18;
        case CGNS_ENUMT(PENTA_24):
            return inf::MeshInterface::CellType::PENTA_24;
        case CGNS_ENUMT(PENTA_33):
            return inf::MeshInterface::CellType::PENTA_33;
        case CGNS_ENUMT(PENTA_38):
            return inf::MeshInterface::CellType::PENTA_38;
        case CGNS_ENUMT(PENTA_40):
            return inf::MeshInterface::CellType::PENTA_40;
        case CGNS_ENUMT(PENTA_66):
            return inf::MeshInterface::CellType::PENTA_66;
        case CGNS_ENUMT(PENTA_75):
            return inf::MeshInterface::CellType::PENTA_75;
        case CGNS_ENUMT(PYRA_5):
            return inf::MeshInterface::CellType::PYRA_5;
        case CGNS_ENUMT(PYRA_13):
            return inf::MeshInterface::CellType::PYRA_13;
        case CGNS_ENUMT(PYRA_14):
            return inf::MeshInterface::CellType::PYRA_14;
        case CGNS_ENUMT(PYRA_21):
            return inf::MeshInterface::CellType::PYRA_21;
        case CGNS_ENUMT(PYRA_29):
            return inf::MeshInterface::CellType::PYRA_29;
        case CGNS_ENUMT(PYRA_30):
            return inf::MeshInterface::CellType::PYRA_30;
        case CGNS_ENUMT(PYRA_50):
            return inf::MeshInterface::CellType::PYRA_50;
        case CGNS_ENUMT(PYRA_55):
            return inf::MeshInterface::CellType::PYRA_55;
        case CGNS_ENUMT(PYRA_P4_29):
            return inf::MeshInterface::CellType::PYRA_P4_29;
        case CGNS_ENUMT(HEXA_8):
            return inf::MeshInterface::CellType::HEXA_8;
        case CGNS_ENUMT(HEXA_20):
            return inf::MeshInterface::CellType::HEXA_20;
        case CGNS_ENUMT(HEXA_27):
            return inf::MeshInterface::CellType::HEXA_27;
        case CGNS_ENUMT(HEXA_32):
            return inf::MeshInterface::CellType::HEXA_32;
        case CGNS_ENUMT(HEXA_44):
            return inf::MeshInterface::CellType::HEXA_44;
        case CGNS_ENUMT(HEXA_56):
            return inf::MeshInterface::CellType::HEXA_56;
        case CGNS_ENUMT(HEXA_64):
            return inf::MeshInterface::CellType::HEXA_64;
        case CGNS_ENUMT(HEXA_98):
            return inf::MeshInterface::CellType::HEXA_98;
        case CGNS_ENUMT(HEXA_125):
            return inf::MeshInterface::CellType::HEXA_125;
        default:
            throw std::logic_error("Unknown Etype cannot be converted to CellType");
    }
}

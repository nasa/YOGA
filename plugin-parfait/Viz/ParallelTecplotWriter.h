#pragma once
#include <array>
#include <memory>
#include <stdlib.h>
#include <string>
#include <MessagePasser/MessagePasser.h>
#include <parfait/TecplotBinaryHelpers.h>
#include <t-infinity/MeshExtents.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <Tracer.h>
#include <parfait/TecplotBrickCollapse.h>
#include <parfait/StringTools.h>
#include <t-infinity/InfinityToVtk.h>
#include <t-infinity/CellIdFilter.h>
#include <t-infinity/Extract.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/FilterFactory.h>

class ParallelTecplotZoneWriter {
  public:
    using ZoneType = Parfait::tecplot::binary_helpers::ZoneHeaderInfo::Type;
    using Association = std::string;
    ParallelTecplotZoneWriter(MessagePasser mp, std::shared_ptr<inf::MeshInterface> mesh, ZoneType zone_type)
        : mp(mp),
          mesh(mesh),
          zone_type(zone_type),
          global_num_points(inf::globalNodeCount(mp, *mesh)),
          global_num_elements(inf::globalCellCount(mp, *mesh)) {
        addMeshTopologies();
    }

    inline void addField(std::shared_ptr<inf::FieldInterface> field) {
        if (inf::FieldAttributes::Node() == field->association()) {
            node_fields.push_back(field);
            return;
        }
        if (inf::FieldAttributes::Cell() == field->association()) {
            cell_fields.push_back(field);
            return;
        }
        PARFAIT_WARNING("Tecplot Writer encountered field " + field->name() +
                        " with unsupported association: " + field->association());
    }

    inline void writeZone(FILE* fp_in) {
        fp = fp_in;

        float zone_marker = 299.0;
        int xyz_fields = 3;
        if (mp.Rank() == 0) {
            writeFloat(zone_marker);
            for (int i = 0; i < int(node_fields.size() + cell_fields.size() + xyz_fields); i++) {
                writeInt(2);  // we're going to write all fields as doubles
            }
            writeInt(
                0);  // no passive variables (these are variables in the global variable list that are NOT in this zone)
            writeInt(0);   // no variable sharing (whatever that is)
            writeInt(-1);  // no zone sharing (whatever THAT is)
        }

        writeRanges();
        writeNodes();
        writeFields();
        if (zone_type == ZoneType::VOLUME) writeVolumeCells();
        if (zone_type == ZoneType::SURFACE) writeSurfaceCells();

        fp = nullptr;
    }

  public:
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> mesh;
    ZoneType zone_type;
    std::vector<std::shared_ptr<inf::FieldInterface>> node_fields;  // will be written out in order added
    std::vector<std::shared_ptr<inf::FieldInterface>> cell_fields;  // nodes first, then cells
    FILE* fp = nullptr;

    struct Topology {
        std::vector<long> global_ids;
        std::vector<bool> do_own;
        std::map<long, std::set<int>> global_to_local;
    };

    long global_num_points;
    long global_num_elements;
    std::map<Association, Topology> topologies;

    inline void writeRanges() {
        auto e = inf::meshExtent(mp, *mesh);
        if (mp.Rank() == 0) {
            writeDouble(e.lo[0]);
            writeDouble(e.hi[0]);
            writeDouble(e.lo[1]);
            writeDouble(e.hi[1]);
            writeDouble(e.lo[2]);
            writeDouble(e.hi[2]);
        }

        for (auto& field : node_fields) {
            auto& top = getTopology(field->association());
            warnIfFieldContainsNanOrInf(mp, *field);
            auto min_max = getMinMax_field(mp, field, top.do_own);
            if (mp.Rank() == 0) {
                if (min_max[0] < -1.0e75 or min_max[1] > 1e75) {
                    PARFAIT_WARNING("Field " + field->name() + " has extreme values in it's range.  " +
                                    std::to_string(min_max[0]) + " " + std::to_string(min_max[1]) +
                                    "This is know to break Tecplot.  \n"
                                    "You likely will get a blank field upon opening the file.\n"
                                    "You might have better luck writing out .vtk files and using ParaView instead. \n"
                                    "Or determine what is creating these extreme values and remove them")
                }
                writeDoubles(min_max.data(), 2);
            }
        }
        for (auto& field : cell_fields) {
            auto& top = getTopology(field->association());
            warnIfFieldContainsNanOrInf(mp, *field);
            auto min_max = getMinMax_field(mp, field, top.do_own);
            if (mp.Rank() == 0) {
                if (min_max[0] < -1.0e75 or min_max[1] > 1e75) {
                    PARFAIT_WARNING("Field " + field->name() + " has extreme values in it's range.  " +
                                    std::to_string(min_max[0]) + " " + std::to_string(min_max[1]) +
                                    "This is know to break Tecplot.  \n"
                                    "You likely will get a blank field upon opening the file.\n"
                                    "You might have better luck writing out .vtk files and using ParaView instead. \n"
                                    "Or determine what is creating these extreme values and remove them")
                }
                writeDoubles(min_max.data(), 2);
            }
        }
    }

    inline void addMeshTopologies() {
        addCellTopology();
        addNodeTopology();
    }

    inline std::map<long, std::set<int>> buildGlobalToLocals(const std::vector<long>& gids) const {
        std::map<long, std::set<int>> g2l;
        for (size_t i = 0; i < gids.size(); i++) {
            auto global = gids[i];
            auto local = i;
            g2l[global].insert(local);
        }
        return g2l;
    }

    inline const Topology& getTopology(std::string association) const {
        if (topologies.count(association) == 0) {
            PARFAIT_THROW("Attempting to access Topology for Unknown Association: " + association);
        }
        return topologies.at(association);
    }
    inline void setTopology(std::string association,
                            const std::vector<long>& global_ids,
                            const std::vector<bool>& do_own) {
        auto& top = topologies[association];
        top.global_ids = global_ids;
        top.do_own = do_own;
        top.global_to_local = buildGlobalToLocals(global_ids);
    }

    inline void addCellTopology() {
        std::vector<long> local_to_global_cell(mesh->cellCount());
        std::vector<bool> do_own(mesh->cellCount());

        for (int c = 0; c < mesh->cellCount(); c++) {
            local_to_global_cell[c] = mesh->globalCellId(c);
            do_own[c] = mesh->cellOwner(c) == mp.Rank();
        }
        setTopology(inf::FieldAttributes::Cell(), local_to_global_cell, do_own);
    }

    inline void addNodeTopology() {
        std::vector<long> local_to_global(mesh->nodeCount());
        std::vector<bool> do_own(mesh->nodeCount());
        for (int c = 0; c < mesh->nodeCount(); c++) {
            local_to_global[c] = mesh->globalNodeId(c);
            do_own[c] = mesh->nodeOwner(c) == mp.Rank();
        }
        setTopology(inf::FieldAttributes::Node(), local_to_global, do_own);
    }

    inline std::map<long, double> extractScalarFieldAsMapInRange(const inf::FieldInterface& field,
                                                                 long start,
                                                                 long end) const {
        auto& top = getTopology(field.association());
        std::map<long, double> out;
        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            if (local_set.size() != 1)
                PARFAIT_THROW(
                    "Could not write field " + field.name() +
                    " to tecplot.  It has the same global entity referenced multiple times.  We don't know which "
                    "one should be used as truth.");
            int local = *local_set.begin();
            if (top.do_own[local] and (global >= start and global < end)) {
                double d;
                field.value(local, &d);
                out[global] = d;
            }
        }
        return out;
    }

    inline void writeNodes() {
        auto x = getPointDimensionAsField("X", 0);
        writeGlobalFieldInChunks(*x, global_num_points);
        x = getPointDimensionAsField("Y", 1);
        writeGlobalFieldInChunks(*x, global_num_points);
        x = getPointDimensionAsField("Z", 2);
        writeGlobalFieldInChunks(*x, global_num_points);
    }
    inline void writeFields() {
        for (auto& field : node_fields) {
            writeGlobalFieldInChunks(*field, global_num_points);
        }
        for (auto& field : cell_fields) {
            writeGlobalFieldInChunks(*field, global_num_elements);
        }
    }

    inline void writeVolumeCells() {
        size_t n_global_items = global_num_elements;

        size_t chunk_max_size_in_MB = 100;
        size_t using_chunk_size = chunk_max_size_in_MB;
        auto memory_remaining = Tracer::availableMemoryMB();
        if (memory_remaining < using_chunk_size) {
            using_chunk_size = memory_remaining / 2;
        }
        using_chunk_size = mp.ParallelMin(using_chunk_size);
        if (using_chunk_size <= 0) {
            PARFAIT_WARNING("Free Memory available (" + std::to_string(using_chunk_size * 2) +
                            "MB) may not be enough to write snap file\n");
            using_chunk_size = 10;  // defaulting to 10MB
        }
        size_t megabyte = 1024 * 1024;
        size_t chunk_size = using_chunk_size * megabyte / sizeof(double);
        size_t num_written = 0;
        std::vector<std::array<int, 8>> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractCollapsedVolumeCellsInRange(start, end), start, end, 0);
            if (mp.Rank() == 0) {
                fwrite(chunk_data.data(), sizeof(int), chunk_data.size() * 8, fp);
            }
            num_written += num_to_write;
        }
    }

    inline void writeSurfaceCells() {
        size_t n_global_items = global_num_elements;

        size_t chunk_max_size_in_MB = 100;
        size_t using_chunk_size = chunk_max_size_in_MB;
        auto memory_remaining = Tracer::availableMemoryMB();
        if (memory_remaining < using_chunk_size) {
            using_chunk_size = memory_remaining / 2;
        }
        using_chunk_size = mp.ParallelMin(using_chunk_size);
        if (using_chunk_size <= 0) {
            PARFAIT_WARNING("Free Memory available (" + std::to_string(using_chunk_size * 2) +
                            "MB) may not be enough to write snap file\n");
            using_chunk_size = 10;  // defaulting to 10MB
        }
        size_t megabyte = 1024 * 1024;
        size_t chunk_size = using_chunk_size * megabyte / sizeof(double);
        size_t num_written = 0;
        std::vector<std::array<int, 4>> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractCollapsedSurfaceCellsInRange(start, end), start, end, 0);
            if (mp.Rank() == 0) {
                fwrite(chunk_data.data(), sizeof(int), chunk_data.size() * 4, fp);
            }
            num_written += num_to_write;
        }
    }

    inline std::map<long, std::array<int, 8>> extractCollapsedVolumeCellsInRange(long start, long end) {
        auto& top = getTopology(inf::FieldAttributes::Cell());
        std::map<long, std::array<int, 8>> out;
        std::array<int, 8> cell_nodes;
        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            int local = *local_set.begin();
            if (top.do_own[local] and (global >= start and global < end)) {
                mesh->cell(local, cell_nodes.data());
                auto type = mesh->cellType(local);
                auto length = mesh->cellTypeLength(type);
                for (int i = 0; i < length; i++) {
                    cell_nodes[i] = mesh->globalNodeId(cell_nodes[i]);
                }
                int int_type = inf::infinityToVtkCellType(type);
                out[global] = Parfait::tecplot::collapseCellToBrick(int_type, cell_nodes);
            }
        }
        return out;
    }

    inline std::map<long, std::array<int, 4>> extractCollapsedSurfaceCellsInRange(long start, long end) {
        auto& top = getTopology(inf::FieldAttributes::Cell());
        std::map<long, std::array<int, 4>> out;
        std::array<int, 4> cell_nodes;
        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            int local = *local_set.begin();
            if (top.do_own[local] and (global >= start and global < end)) {
                mesh->cell(local, cell_nodes.data());
                auto type = mesh->cellType(local);
                auto length = mesh->cellTypeLength(type);
                for (int i = 0; i < length; i++) {
                    cell_nodes[i] = mesh->globalNodeId(cell_nodes[i]);
                }
                if (length == 3) {
                    cell_nodes[3] = cell_nodes[0];
                }
                out[global] = cell_nodes;
            }
        }
        return out;
    }

    inline void writeGlobalFieldInChunks(const inf::FieldInterface& field, size_t n_global_items) {
        size_t chunk_max_size_in_MB = 100;
        size_t using_chunk_size = chunk_max_size_in_MB;
        auto memory_remaining = Tracer::availableMemoryMB();
        if (memory_remaining < using_chunk_size) {
            using_chunk_size = memory_remaining / 2;
        }
        using_chunk_size = mp.ParallelMin(using_chunk_size);
        if (using_chunk_size <= 0) {
            PARFAIT_WARNING("Free Memory available (" + std::to_string(using_chunk_size * 2) +
                            "MB) may not be enough to write snap file\n");
            using_chunk_size = 10;  // defaulting to 10MB
        }
        size_t megabyte = 1024 * 1024;
        size_t chunk_size = using_chunk_size * megabyte / sizeof(double);
        size_t num_written = 0;
        std::vector<double> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractScalarFieldAsMapInRange(field, start, end), start, end, 0);
            if (mp.Rank() == 0) {
                fwrite(chunk_data.data(), sizeof(double), chunk_data.size(), fp);
            }
            num_written += num_to_write;
        }
    }

    static std::array<double, 2> getMinMax_field(MessagePasser mp,
                                                 std::shared_ptr<inf::FieldInterface> field,
                                                 const std::vector<bool>& do_own) {
        auto min_max = field_min_max(*field, do_own);
        min_max[0] = mp.ParallelMin(min_max[0]);
        min_max[1] = mp.ParallelMax(min_max[1]);
        return min_max;
    }

    static std::array<double, 2> field_min_max(const inf::FieldInterface& field, const std::vector<bool>& do_own) {
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        for (int n = 0; n < field.size(); n++) {
            if (do_own[n]) {
                double d;
                field.value(n, &d);
                min = std::min(min, d);
                max = std::max(max, d);
            }
        }
        return {min, max};
    }

    static void warnIfFieldContainsNanOrInf(MessagePasser mp, const inf::FieldInterface& field) {
        bool inf_nan_found = false;
        for (int n = 0; n < field.size(); n++) {
            double d;
            field.value(n, &d);
            if (std::isnan(d) or std::isinf(d)) {
                inf_nan_found = true;
                break;
            }
        }
        inf_nan_found = mp.ParallelOr(inf_nan_found);
        if (inf_nan_found and mp.Rank() == 0)
            PARFAIT_WARNING("field " + field.name() +
                            " contains NaN and/or Inf values.  The output file is likely unreadable by tecplot.");
    }

    inline std::shared_ptr<inf::FieldInterface> getPointDimensionAsField(std::string name, int d) {
        std::vector<double> x(mesh->nodeCount());
        std::array<double, 3> p;
        for (int n = 0; n < mesh->nodeCount(); n++) {
            mesh->nodeCoordinate(n, p.data());
            x[n] = p[d];
        }
        return std::make_shared<inf::VectorFieldAdapter>(name, inf::FieldAttributes::Node(), 1, x);
    }

    inline void writeFloat(float f) { fwrite(&f, sizeof(float), 1, fp); }
    inline void writeDouble(double f) { fwrite(&f, sizeof(double), 1, fp); }
    inline void writeInt(int i) { fwrite(&i, sizeof(int), 1, fp); }
    inline void writeDoubles(double* d, int num_to_write) { fwrite(d, sizeof(double), num_to_write, fp); }
};

class ParallelTecplotWriter {
  public:
    using ZoneType = Parfait::tecplot::binary_helpers::ZoneHeaderInfo::Type;
    using Association = std::string;
    inline ParallelTecplotWriter(std::string filename, std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm)
        : mp(comm),
          filename(filename),
          mesh(mesh),
          volume_filter(comm, mesh, std::make_shared<inf::DimensionalitySelector>(3)),
          volume_zone_writer(mp, volume_filter.getMesh(), ZoneType::VOLUME),
          n2c(inf::NodeToCell::build(*mesh)),
          n2c_surface(inf::NodeToCell::buildSurfaceOnly(*mesh)) {
        auto tags = inf::extractAllTagsWithDimensionality(mp, *mesh, 2);
        for (auto t : tags) {
            auto f = inf::FilterFactory::surfaceTagFilter(mp.getCommunicator(), mesh, std::vector<int>{t});
            surface_tag_filters.insert({t, f});
            auto& filter = surface_tag_filters.at(t);
            surface_tag_writers.insert({t, ParallelTecplotZoneWriter(mp, filter->getMesh(), ZoneType::SURFACE)});
        }
    }

    inline void addField(std::shared_ptr<inf::FieldInterface> field) {
        if (field->association() != inf::FieldAttributes::Cell() and
            field->association() != inf::FieldAttributes::Node()) {
            mp_rootprint("Skipping field with unsupported association: %s\n", field->association().c_str());
        }
        auto solution_time_string = field->attribute(inf::FieldAttributes::solutionTime());
        if (not solution_time_string.empty()) {
            try {
                double s = Parfait::StringTools::toDouble(solution_time_string);
                solution_time = std::max(solution_time, s);
            } catch (...) {
            }
        }
        if (field->association() == inf::FieldAttributes::Node()) {
            addInOrder(ordered_node_fields, field);
        } else if (field->association() == inf::FieldAttributes::Cell()) {
            addInOrder(ordered_cell_fields, field);
        }
    }

    inline void write() {
        open();
        addFieldsToSubZones();
        writeHeader();
        if (write_volume) volume_zone_writer.writeZone(fp);
        for (auto& pair : surface_tag_writers) {
            pair.second.writeZone(fp);
        }
        close();
    }

    inline void setSolutionTime(double s) { solution_time = s; }
    static inline void addInOrder(std::vector<std::shared_ptr<inf::FieldInterface>>& ordered_fields,
                                  std::shared_ptr<inf::FieldInterface> field) {
        try {
            auto outputOrder = inf::FieldAttributes::outputOrder();
            auto f_output_order = field->attribute(outputOrder);
            if (ordered_fields.size() == 0 or f_output_order.empty()) {  // empty fields go to the back (unsorted)
                ordered_fields.push_back(field);
                return;
            }
            int f_output_order_int = Parfait::StringTools::toInt(f_output_order);
            for (size_t i = 0; i < ordered_fields.size(); i++) {
                auto output_order = ordered_fields[i]->attribute(inf::FieldAttributes::outputOrder());
                if (output_order.empty()) {
                    ordered_fields.insert(ordered_fields.begin() + i, field);
                    return;
                }
                // We have to explicitly convert to int because string "10" is less than string "2"
                int output_order_int = Parfait::StringTools::toInt(output_order);
                if (f_output_order_int < output_order_int) {
                    ordered_fields.insert(ordered_fields.begin() + i, field);
                    return;
                }
            }
            // Hey, we're lucky, the last spot IS the right order.
            ordered_fields.push_back(field);
        } catch (std::exception& e) {
            ordered_fields.push_back(field);
        }
    }
    inline void addFieldsToSubZones() {
        auto push_field = [this](auto& field) {
            volume_zone_writer.addField(volume_filter.apply(field));
            for (auto& pair : surface_tag_filters) {
                auto& tag = pair.first;
                auto& filter = pair.second;
                surface_tag_writers.at(tag).addField(filter->apply(field));
            }
            if (inf::FieldAttributes::Cell() == field->association()) {
                ordered_cell_field_names.push_back(field->name());
            }
            if (inf::FieldAttributes::Node() == field->association()) {
                ordered_node_field_names.push_back(field->name());
            }
        };

        for (auto& field : ordered_node_fields) {
            push_field(field);
        }
        for (auto& field : ordered_cell_fields) {
            push_field(field);
        }
    }

  public:
    MessagePasser mp;
    std::string filename;
    std::shared_ptr<inf::MeshInterface> mesh;
    inf::CellIdFilter volume_filter;
    std::map<int, std::shared_ptr<inf::CellIdFilter>> surface_tag_filters;
    bool write_volume = true;
    ParallelTecplotZoneWriter volume_zone_writer;
    std::map<int, ParallelTecplotZoneWriter> surface_tag_writers;
    std::vector<std::vector<int>> n2c;
    std::vector<std::vector<int>> n2c_surface;

    std::vector<std::shared_ptr<inf::FieldInterface>> ordered_node_fields;
    std::vector<std::shared_ptr<inf::FieldInterface>> ordered_cell_fields;
    std::vector<std::string> ordered_node_field_names;
    std::vector<std::string> ordered_cell_field_names;

    FILE* fp;
    double solution_time = 0.0;

    inline void open() {
        if (Parfait::StringTools::getExtension(filename) != "plt") filename += ".plt";
        if (mp.Rank() == 0) {
            fp = fopen(filename.c_str(), "w");
            if (fp == nullptr) PARFAIT_THROW("Could not open file for writing: " + filename);
        }
    }

    inline void close() {
        if (mp.Rank() == 0) {
            if (fp) fclose(fp);
        }
    }

    inline void writeHeader() {
        using namespace Parfait::tecplot::binary_helpers;

        std::vector<ZoneHeaderInfo> zone_headers;
        {
            ZoneHeaderInfo volume_zone_info;
            volume_zone_info.type = ZoneHeaderInfo::VOLUME;
            volume_zone_info.name = "Volume";
            volume_zone_info.num_points = volume_zone_writer.global_num_points;
            volume_zone_info.num_elements = volume_zone_writer.global_num_elements;
            if (volume_zone_info.num_elements != 0) {
                zone_headers.push_back(volume_zone_info);
                write_volume = true;
            } else {
                write_volume = false;
            }
        }

        for (auto& pair : surface_tag_writers) {
            auto tag = pair.first;
            auto& writer = pair.second;
            ZoneHeaderInfo zone_info;
            zone_info.type = ZoneHeaderInfo::SURFACE;
            zone_info.name = mesh->tagName(tag);
            zone_info.num_points = writer.global_num_points;
            zone_info.num_elements = writer.global_num_elements;
            if (zone_info.num_elements != 0) {
                zone_headers.push_back(zone_info);
            }
        }

        std::vector<std::string> variable_names;
        std::vector<Parfait::tecplot::VariableLocation> variable_locations;
        for (auto& name : ordered_node_field_names) {
            variable_names.push_back(name);
            variable_locations.push_back(Parfait::tecplot::NODE);
        }
        for (auto& name : ordered_cell_field_names) {
            variable_names.push_back(name);
            variable_locations.push_back(Parfait::tecplot::CELL);
        }
        if (mp.Rank() == 0) {
            int x_y_z_names = 3;
            Parfait::tecplot::binary_helpers::writeHeader(fp,
                                                          x_y_z_names + variable_names.size(),
                                                          variable_names,
                                                          variable_locations,
                                                          zone_headers,
                                                          solution_time);
        }
    }

    inline void writeFloat(float f) { fwrite(&f, sizeof(float), 1, fp); }
    inline void writeDouble(double f) { fwrite(&f, sizeof(double), 1, fp); }
    inline void writeInt(int i) { fwrite(&i, sizeof(int), 1, fp); }
    inline void writeDoubles(double* d, int num_to_write) { fwrite(d, sizeof(double), num_to_write, fp); }
};

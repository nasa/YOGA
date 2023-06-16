#pragma once
#include <string>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MeshExtents.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/FileStreamer.h>
#include <Tracer.h>
#include <parfait/StringTools.h>
#include <t-infinity/InfinityToVtk.h>

class ParallelVTKWriter {
  public:
    using Association = std::string;
    inline ParallelVTKWriter(std::string filename, std::shared_ptr<inf::MeshInterface> mesh, MPI_Comm comm)
        : mp(comm),
          filename(filename),
          mesh(mesh),
          global_num_points(inf::globalNodeCount(mp, *mesh)),
          global_num_elements(inf::globalCellCount(mp, *mesh)) {
        addMeshTopologies();
    }

    inline void addField(std::shared_ptr<inf::FieldInterface> field) {
        if (field->association() == inf::FieldAttributes::Cell()) {
            cell_fields[field->name()] = field;
        } else {
            node_fields[field->name()] = field;
        }
    }

    inline void write() {
        open();
        writeHeader();
        writeNodes();
        writeCells();
        writeFields();
        close();
    }

  public:
    struct Topology {
        std::vector<long> global_ids;
        std::vector<bool> do_own;
        std::map<long, std::set<int>> global_to_local;
    };

    MessagePasser mp;
    std::string filename;
    std::shared_ptr<inf::MeshInterface> mesh;
    std::map<std::string, std::shared_ptr<inf::FieldInterface>> node_fields;
    std::map<std::string, std::shared_ptr<inf::FieldInterface>> cell_fields;
    long global_num_points;
    long global_num_elements;
    std::map<Association, Topology> topologies;

    std::shared_ptr<inf::FileStreamer> f = nullptr;

    inline void open() {
        filename = Parfait::StringTools::stripExtension(filename, "pvtk");
        if (Parfait::StringTools::getExtension(filename) != "vtk") filename += ".vtk";
        if (mp.Rank() == 0) {
            f = inf::FileStreamer::create("default");
            if (not f->openWrite(filename)) {
                PARFAIT_THROW("Could not open file for writing: " + filename);
            }
        }
    }

    inline void close() {
        if (mp.Rank() == 0) {
            f->close();
        }
    }

    inline void writeHeader() {
        if (mp.Rank() == 0) {
            auto header = Parfait::vtk::Writer::getHeader();
            f->write(header.data(), sizeof(char), header.size());
        }
    }

    inline std::map<long, Parfait::Point<double>> extractOwnedNodesInRange(long start, long end) {
        auto& top = getTopology(inf::FieldAttributes::Node());
        std::map<long, Parfait::Point<double>> out;
        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            if (local_set.size() != 1) PARFAIT_THROW("Multiple local nodes");
            int local = *local_set.begin();
            if (top.do_own[local] and (global >= start and global < end)) {
                auto p = mesh->node(local);
                out[global] = p;
            }
        }
        return out;
    }

    inline void writeNodes() {
        if (mp.Rank() == 0) {
            std::string s = "POINTS " + std::to_string(global_num_points) + " double\n";
            f->write(s.data(), sizeof(char), s.size());
        }

        size_t n_global_items = global_num_points;

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
        std::vector<Parfait::Point<double>> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractOwnedNodesInRange(start, end), start, end, 0);
            if (mp.Rank() == 0) {
                for (auto& p : chunk_data) {
                    bswap_64(&p[0]);
                    bswap_64(&p[1]);
                    bswap_64(&p[2]);
                }
                f->write(chunk_data.data(), sizeof(double), chunk_data.size() * 3);
            }
            num_written += num_to_write;
        }
    }
    inline void writeFields() {
        if (cell_fields.size() != 0) {
            if (mp.Rank() == 0) {
                std::string s = "\nCELL_DATA " + std::to_string(global_num_elements) + "\n";
                f->write(s.data(), sizeof(char), s.size());
            }
            for (auto& key_value : cell_fields) {
                writeGlobalFieldInChunks(*key_value.second, global_num_elements);
            }
        }

        if (node_fields.size() != 0) {
            if (mp.Rank() == 0) {
                std::string s = "\nPOINT_DATA " + std::to_string(global_num_points) + "\n";
                f->write(s.data(), sizeof(char), s.size());
            }
            for (auto& key_value : node_fields) {
                writeGlobalFieldInChunks(*key_value.second, global_num_points);
            }
        }
    }

    inline std::map<long, int> extractOwnedCellTypesInRange(long start, long end) {
        auto& top = getTopology(inf::FieldAttributes::Cell());
        std::map<long, int> out;

        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            if (local_set.size() != 1)
                PARFAIT_THROW("Multiple local cells map to global id " + std::to_string(global) +
                              ", local: " + Parfait::to_string(local_set));
            int local = *local_set.begin();
            if (top.do_own[local] and (global >= start and global < end)) {
                int cell_type = int(mesh->cellType(local));
                int vtk_type = inf::infinityToVtkCellType(inf::MeshInterface::CellType(cell_type));
                out[global] = vtk_type;
            }
        }
        return out;
    }

    template <size_t CellBufferLength>
    inline std::map<long, std::array<long, CellBufferLength>> extractOwnedCellsInRange(long start, long end) {
        auto& top = getTopology(inf::FieldAttributes::Cell());
        // WARNING!  THIS IS CONFUSING
        // We have to write out each cell in global order
        // with a leading number of how many nodes the cell has.
        // So we're collecting cells in global order
        // with that number shoved into the front of the cell nodes buffer
        // We're using a templated buffer length that must be at least 2
        // larger than the maximum node per cell of any cell in the mesh.
        // For example, if the mesh has any hexes, the buffer length must be at least 10
        // 8 for the nodes of the hex, one for the length of the hex (8) and one for the hex's global id.
        // This is because I couldn't think of a simpler way of getting the cells back
        // in order on root with the ragged structured of each cell possibly being a different length.
        // Doing this means we can leverage MessagePasser's Gather and Sort routine to at least
        // get the cells in global ordering.
        // --Matt.
        std::map<long, std::array<long, CellBufferLength>> out;

        std::vector<int> cell_nodes_local;
        std::array<long, CellBufferLength> cell_nodes_buffer;
        for (auto pair : top.global_to_local) {
            long global = pair.first;
            auto local_set = pair.second;
            if (local_set.size() != 1)
                PARFAIT_THROW("Multiple local cells map to global id " + std::to_string(global) +
                              ", local: " + Parfait::to_string(local_set));
            int local = *local_set.begin();
            using TYPES = inf::MeshInterface;
            if (top.do_own[local] and (global >= start and global < end)) {
                int cell_type = int(mesh->cellType(local));
                mesh->cell(local, cell_nodes_local);
                // All these bi-quadratic types are not supported by VTK
                // but the quadratic versions are
                // luckily we can simply strip off the mid face & mid cell points since they're at the end.
                if (TYPES::QUAD_9 == cell_type)
                    cell_nodes_local.resize(8);
                else if (TYPES::PYRA_14 == cell_type)
                    cell_nodes_local.resize(13);
                else if (TYPES::PENTA_18 == cell_type)
                    cell_nodes_local.resize(15);
                else if (TYPES::HEXA_27 == cell_type)
                    cell_nodes_local.resize(20);

                cell_nodes_buffer[0] = cell_type;
                cell_nodes_buffer[1] = cell_nodes_local.size();
                for (int i = 0; i < cell_nodes_buffer[1]; i++) {
                    int local_node_id = cell_nodes_local[i];
                    PARFAIT_ASSERT(local_node_id >= 0,
                                   "cell local node id is negative: " + std::to_string(local_node_id));
                    cell_nodes_buffer.at(i + 2) = mesh->globalNodeId(local_node_id);
                }
                out[global] = cell_nodes_buffer;
            }
        }
        return out;
    }

    template <size_t BufferLength>
    inline void writePackedCells(const std::vector<std::array<long, BufferLength>>& packed_cells,
                                 long starting_global_cell_id) {
        std::vector<int> cell_nodes;
        for (auto cell : packed_cells) {
            int num_points = int(cell[1]);
            int cell_type = int(cell[0]);
            int vtk_type = inf::infinityToVtkCellType(inf::MeshInterface::CellType(cell_type));

            cell_nodes.resize(num_points);
            for (int i = 0; i < num_points; i++) {
                cell_nodes[i] = cell[i + 2];
            }
            Parfait::vtk::Writer::rewindCellNodes(Parfait::vtk::Writer::CellType(vtk_type), cell_nodes);

            int num_points_big_endian = num_points;
            bswap_32(&num_points_big_endian);
            f->write(&num_points_big_endian, sizeof(num_points_big_endian), 1);

            for (int i = 0; i < num_points; i++) {
                bswap_32(&cell_nodes[i]);
            }

            f->write(cell_nodes.data(), sizeof(int), num_points);
        }
    }

    inline size_t calcCellBufferSize() {
        size_t my_buffer_size = 0;
        for (int c = 0; c < mesh->cellCount(); c++) {
            if (mesh->cellOwner(c) == mp.Rank()) {
                int length = mesh->cellLength(c);
                my_buffer_size += length;
                my_buffer_size += 1;  // add one integer for the cell length itself.
            }
        }
        return mp.ParallelSum(my_buffer_size);
    }
    void writeCellTypes() {
        if (mp.Rank() == 0) {
            std::string s = "\nCELL_TYPES " + std::to_string(global_num_elements) + "\n";
            f->write(s.data(), sizeof(char), s.size());
        }
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
        size_t chunk_size = using_chunk_size * megabyte / sizeof(int);
        size_t num_written = 0;
        std::vector<int> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractOwnedCellTypesInRange(start, end), start, end, 0);
            if (mp.Rank() == 0) {
                for (auto& type : chunk_data) {
                    bswap_32(&type);
                }
                f->write(chunk_data.data(), sizeof(int), chunk_data.size());
            }
            num_written += num_to_write;
        }
        if (mp.Rank() == 0) {
            std::string s = "\n";
            f->write(s.data(), sizeof(char), s.size());
        }
    }

    int calcMaxCellLength() {
        int max_length = 0;
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto l = mesh->cellLength(c);
            max_length = std::max(l, max_length);
        }
        return max_length;
    }
    template <size_t CellBufferLength>
    void writeCellsTemplatedLength() {
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
        std::vector<std::array<long, CellBufferLength>> chunk_data;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_data = mp.GatherByOrdinalRange(extractOwnedCellsInRange<CellBufferLength>(start, end), start, end, 0);
            if (mp.Rank() == 0) {
                writePackedCells(chunk_data, start);
            }
            num_written += num_to_write;
        }
    }
    inline void writeCells() {
        size_t cell_buffer_size = calcCellBufferSize();
        if (mp.Rank() == 0) {
            std::string s =
                "\nCELLS " + std::to_string(global_num_elements) + " " + std::to_string(cell_buffer_size) + "\n";
            f->write(s.data(), sizeof(char), s.size());
        }
        int max_cell_length = calcMaxCellLength();

        if (max_cell_length <= 8) {
            writeCellsTemplatedLength<10>();
        } else if (max_cell_length <= 27) {
            writeCellsTemplatedLength<29>();
        } else {
            PARFAIT_WARNING("Cannot write file " + filename +
                            " the maximum cell length is too large: " + std::to_string(max_cell_length));
        }

        writeCellTypes();
    }

    inline void writeGlobalFieldInChunks(const inf::FieldInterface& field, size_t n_global_items) {
        std::string field_name = field.name();
        field_name = Parfait::StringTools::findAndReplace(field_name, " ", "_");
        if (mp.Rank() == 0) {
            std::string s = "SCALARS " + field_name + " DOUBLE 1\n";
            s += "LOOKUP_TABLE DEFAULT\n";
            f->write(s.data(), sizeof(char), s.size());
        }

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
                for (auto& d : chunk_data) {
                    bswap_64(&d);
                }
                f->write(chunk_data.data(), sizeof(double), chunk_data.size());
            }
            num_written += num_to_write;
        }
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
                    " to snap.  It has the same global entity referenced multiple times.  We don't know which "
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

    inline const Topology& getTopology(std::string association) const {
        if (topologies.count(association) == 0) {
            PARFAIT_THROW("Attempting to access Topology for Unknown Association: " + association);
        }
        return topologies.at(association);
    }
    inline void addMeshTopologies() {
        addCellTopology();
        addNodeTopology();
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
    inline void setTopology(std::string association,
                            const std::vector<long>& global_ids,
                            const std::vector<bool>& do_own) {
        auto& top = topologies[association];
        top.global_ids = global_ids;
        top.do_own = do_own;
        top.global_to_local = buildGlobalToLocals(global_ids);
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
};

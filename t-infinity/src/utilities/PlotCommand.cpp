#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/DistanceCalculator.h>
#include <t-infinity/MeshSpacing.h>
#include <t-infinity/Extract.h>
#include <t-infinity/SurfaceSmoothness.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/MeshHelpers.h>
#include <parfait/FileTools.h>
#include <parfait/MapbcWriter.h>
#include <t-infinity/MeshExploder.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/FieldStatistics.h>
#include <t-infinity/MeshQualityMetrics.h>
#include <t-infinity/CommonSubcommandFunctions.h>

using namespace inf;

class PlotCommand : public SubCommand {
  public:
    std::string description() const override {
        return "slice/filter meshes/field data & export to files";
    }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::snap(), Help::snap(), false);
        m.addParameter({"animate"}, "Animate a series of .snap files", false);
        m.addParameter(
            {"slice"}, "create a crinkle slice at requested location (ex: --slice x = 0.0)", false);
        m.addParameter(
            {"distance"}, "Plot distance to set of surface tags (ex: --distance 1,2,3)", false);
        m.addParameter({"o"}, "Output filename", false, "out");
        m.addParameter({"select"}, "Only include these fields in the output file.", false);
        m.addParameter({"at"}, "Convert fields to either <nodes> or <cells>", false);
        m.addParameter({"--tags", "-t"}, "1,2,3 = plot only tags 1 2 and 3", false);
        m.addParameter(
            {"--cell-list"}, "filename of ascii file of global cell ids to include", false);
        m.addParameter(
            {"--sphere"}, "x y z r = clip to sphere centered at x,y,z with radius r", false);
        m.addFlag({"node-owners"}, "add field to output with MPI rank of node owners");
        m.addFlag({"smooth"}, "smooth output fields (laplacian)");
        m.addFlag({"smoothness"}, "plot local mesh smoothness");
        m.addFlag({"surface"}, "surface mesh only");
        m.addFlag({"volume"}, "volume mesh only");
        m.addFlag({"clean"}, "don't automatically add cell owner/tag as fields");
        m.addFlag({"explode"}, "visualize an exploded view of the mesh (no fields)");
        m.addFlag({"cell-quality", "-q"},
                  "visualize mesh quality where 0 is optimal, 1 is degenerate, >1 is twisted.");
        m.addFlag({"partition"}, "Add the partition id to the output");
        m.addFlag({"edge-length"}, "Plot node avg and min edge lengths");
        m.addFlag({"area"}, "Plot surface cell directed area");
        m.addFlag({"edge-stencil-count"}, "Plot number of edge-connected nodes at nodes");
        m.addFlag({"gids"}, "Add gids of cells and nodes to the output");
        m.addParameter({"--scale"}, "The explode scale", false, "0.75");
        //        m.addParameter({"script"}, "plot from script", false);
        m.addHiddenFlag({"--verbose"}, "print entire mesh to screen");
        return m;
    }

    void plotOneFile(MessagePasser mp,
                     std::shared_ptr<MeshInterface> mesh,
                     const Parfait::CommandLineMenu& m,
                     std::string field_input_filename,
                     std::string output_filename) {
        mp_rootprint("Plotting %s\n", output_filename.c_str());
        auto fields = importFieldsAndMaybeAddSome(mp, mesh, m, field_input_filename);
        for (auto field : fields) {
            printf("about to plot field: %s\n", field->name().c_str());
        }
        plot(mesh, fields, m, mp, output_filename);
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        TRACER_PROFILE_SCOPE("PlotCommand::run");
        auto mesh = importMesh(m, mp);
        mp_rootprint("Imported mesh\n");
        if (m.has("animate")) {
            auto movie_filenames = m.getAsVector("animate");
            mp_rootprint("Animating: %s\n", Parfait::to_string(movie_filenames).c_str());
            for (int i = 0; i < int(movie_filenames.size()); i++) {
                auto f = movie_filenames[i];
                auto output = m.get(Alias::outputFileBase());
                auto extension = Parfait::StringTools::getExtension(output);
                output = Parfait::StringTools::stripExtension(output);
                output += "." + std::to_string(i);
                if (not extension.empty()) output += "." + extension;
                plotOneFile(mp, mesh, m, f, output);
            }
        } else {
            std::string filename;
            if (m.has(Alias::snap())) {
                filename = m.get(Alias::snap());
            }
            plotOneFile(mp, mesh, m, filename, m.get(Alias::outputFileBase()));
        }
        if (Parfait::StringTools::getExtension(m.get(Alias::mesh())) == "csm") {
            writeMapbcFile(m, mp, *mesh);
        }
    }

  private:
    int getRequestedSliceDimension(std::string plane) const {
        if (Parfait::StringTools::tolower(plane) == "x") return 0;
        if (Parfait::StringTools::tolower(plane) == "y") return 1;
        if (Parfait::StringTools::tolower(plane) == "z") return 2;
        PARFAIT_THROW("Slice plane unknown: " + plane);
    }

    double getSliceLocation(const std::vector<std::string>& args) const {
        for (const auto& arg : args) {
            if (arg == "=") continue;
            return Parfait::StringTools::toDouble(arg);
        }
        PARFAIT_THROW("Could not determine slice location: " + Parfait::to_string(args));
    }

    Parfait::Extent<double> createExtentFromSliceOption(
        const std::vector<std::string>& args) const {
        Parfait::Extent<double> e = Parfait::ExtentBuilder::createMaximumExtent<double>();
        auto plane = args.front();
        int d = getRequestedSliceDimension(plane);
        double location = getSliceLocation(std::vector<std::string>(args.begin() + 1, args.end()));
        double epsilon = 1.0e-10;
        e.lo[d] = location - epsilon;
        e.hi[d] = location + epsilon;
        return e;
    }

    std::shared_ptr<CompositeSelector> getCellSelector(MessagePasser mp,
                                                       const Parfait::CommandLineMenu& m,
                                                       int dimensionality) const {
        auto cell_selector = std::make_shared<CompositeSelector>();
        if (m.has("--surface"))
            cell_selector->add(std::make_shared<DimensionalitySelector>(dimensionality - 1));
        if (m.has("--tags")) cell_selector->add(std::make_shared<TagSelector>(getTags(m)));
        if (m.has("--volume"))
            cell_selector->add(std::make_shared<DimensionalitySelector>(dimensionality));
        if (m.has("--sphere")) {
            auto doubles_string = m.getAsVector("--sphere");
            std::vector<double> doubles;
            for (auto s : doubles_string) {
                doubles.push_back(Parfait::StringTools::toDouble(s));
            }
            cell_selector->add(std::make_shared<SphereSelector>(
                doubles[3], Parfait::Point<double>{doubles[0], doubles[1], doubles[2]}));
        }
        if (m.has("--cell-list")) {
            auto filename = m.get("--cell-list");
            auto contents = Parfait::FileTools::loadFileToString(filename);
            auto ids = Parfait::StringTools::parseIntegerList(contents);
            mp_rootprint("plotting cells based on list %s\n", Parfait::to_string(ids).c_str());
            cell_selector->add(
                std::make_shared<GlobalCellSelector>(Parfait::VectorTools::to_long(ids)));
        }
        if (m.has("--slice"))
            cell_selector->add(std::make_shared<ExtentSelector>(
                createExtentFromSliceOption(m.getAsVector("--slice"))));
        return cell_selector;
    }

    std::shared_ptr<FieldInterface> getCellTags(const MeshInterface& mesh) const {
        std::vector<double> cell_tag(mesh.cellCount());
        for (int i = 0; i < mesh.cellCount(); i++) {
            cell_tag[i] = mesh.cellTag(i);
        }
        return std::make_shared<VectorFieldAdapter>(
            "Cell tag", FieldAttributes::Cell(), 1, cell_tag);
    }

    void appendGIDFields(MessagePasser mp,
                         const MeshInterface& mesh,
                         std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        std::vector<double> node_owner(mesh.nodeCount());
        for (int n = 0; n < mesh.nodeCount(); n++) {
            node_owner[n] = double(mesh.globalNodeId(n));
        }

        std::vector<double> cell_owner(mesh.cellCount());
        for (int c = 0; c < mesh.cellCount(); c++) {
            cell_owner[c] = double(mesh.globalCellId(c));
        }

        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "node-global-id", inf::FieldAttributes::Node(), 1, node_owner));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "cell-global-id", inf::FieldAttributes::Cell(), 1, cell_owner));
    }

    void appendDistance(MessagePasser mp,
                        const MeshInterface& mesh,
                        std::vector<int> tags,
                        std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        printf("Computing distance to tags <%s>\n", Parfait::to_string(tags).c_str());
        auto node_distance =
            inf::calculateDistance(mp, mesh, std::set<int>(tags.begin(), tags.end()));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.distance", inf::FieldAttributes::Node(), 1, node_distance));
    }

    void appendMeshSmoothness(MessagePasser mp,
                              const MeshInterface& mesh,
                              std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto mesh_smoothness = inf::calcSurfaceSmoothness(mesh);
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.mesh-smoothness", inf::FieldAttributes::Node(), 1, mesh_smoothness));
    }
    void appendNodeOwners(MessagePasser mp,
                          const MeshInterface& mesh,
                          std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto node_owners = extractNodeOwners(mesh);
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.mesh-smoothness", inf::FieldAttributes::Node(), 1, node_owners));
    }
    void appendEdgeLengths(MessagePasser mp,
                           const MeshInterface& mesh,
                           std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto node_length = inf::calcNodeMinLength(mesh);
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.min_edge_length", inf::FieldAttributes::Node(), 1, node_length));
        node_length = inf::calcNodeAvgLength(mesh);
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.avg_edge_length", inf::FieldAttributes::Node(), 1, node_length));
    }
    void appendSurfaceArea(MessagePasser mp,
                           const MeshInterface& mesh,
                           std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto mesh_dimensionality = maxCellDimensionality(mp, mesh);
        auto surface_area = inf::extractSurfaceArea(mesh, mesh_dimensionality - 1);
        std::vector<double> surface_area_x(mesh.cellCount());
        std::vector<double> surface_area_y(mesh.cellCount());
        std::vector<double> surface_area_z(mesh.cellCount());
        for (int c = 0; c < mesh.cellCount(); c++) {
            surface_area_x[c] = surface_area[c][0];
            surface_area_y[c] = surface_area[c][1];
            surface_area_z[c] = surface_area[c][2];
        }
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "surface_area.x", inf::FieldAttributes::Cell(), 1, surface_area_x));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "surface_area.y", inf::FieldAttributes::Cell(), 1, surface_area_y));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "surface_area.z", inf::FieldAttributes::Cell(), 1, surface_area_z));

        std::vector<double> dual_area_x(mesh.nodeCount(), 0.0);
        std::vector<double> dual_area_y(mesh.nodeCount(), 0.0);
        std::vector<double> dual_area_z(mesh.nodeCount(), 0.0);
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellDimensionality(c) == mesh_dimensionality - 1) {
                auto nodes_in_cell = mesh.cell(c);
                for (int n : nodes_in_cell) {
                    dual_area_x[n] += surface_area_x[c] / double(nodes_in_cell.size());
                    dual_area_y[n] += surface_area_y[c] / double(nodes_in_cell.size());
                    dual_area_z[n] += surface_area_z[c] / double(nodes_in_cell.size());
                }
            }
        }
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_area.x", inf::FieldAttributes::Node(), 1, dual_area_x));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_area.y", inf::FieldAttributes::Node(), 1, dual_area_y));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_area.z", inf::FieldAttributes::Node(), 1, dual_area_z));

        std::vector<double> dual_unit_normal_x(mesh.nodeCount(), 0.0);
        std::vector<double> dual_unit_normal_y(mesh.nodeCount(), 0.0);
        std::vector<double> dual_unit_normal_z(mesh.nodeCount(), 0.0);
        for (int node_id = 0; node_id < mesh.nodeCount(); ++node_id) {
            Parfait::Point<double> n = {
                dual_area_x[node_id], dual_area_y[node_id], dual_area_z[node_id]};
            n.normalize();
            dual_unit_normal_x[node_id] = n[0];
            dual_unit_normal_y[node_id] = n[1];
            dual_unit_normal_z[node_id] = n[2];
        }
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_unit_normal.x", inf::FieldAttributes::Node(), 1, dual_unit_normal_x));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_unit_normal.y", inf::FieldAttributes::Node(), 1, dual_unit_normal_y));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "dual_unit_normal.z", inf::FieldAttributes::Node(), 1, dual_unit_normal_z));
    }
    void appendCellQuality(MessagePasser mp,
                           const MeshInterface& mesh,
                           std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto mesh_dimension = maxCellDimensionality(mp, mesh);
        auto cell_hilbert_cost = inf::MeshQuality::cellHilbertCost(mesh);
        auto face_neighbors = inf::FaceNeighbors::build(mesh);
        inf::FieldTools::setSurfaceCellToVolumeNeighborValue(
            mesh, face_neighbors, mesh_dimension, cell_hilbert_cost);
        auto cell_flattness_cost = inf::MeshQuality::cellFlattnessCost(mesh);
        auto n2c_volume = inf::NodeToCell::buildVolumeOnly(mesh);
        auto node_hilbert_cost = inf::MeshQuality::nodeHilbertCost(mesh, n2c_volume);
        auto node_statistics = inf::FieldStatistics::calcNodes(mp, mesh, node_hilbert_cost);
        auto cell_statistics = inf::FieldStatistics::calcCells(mp, mesh, cell_hilbert_cost);
        mp_rootprint("Hilbert Cost: 0.0 ideal,  1.0 degenerate,  over 1.0 inverted \n");
        inf::FieldStatistics::print(mp, node_statistics, "Hilbert Node Cost");
        inf::FieldStatistics::print(mp, cell_statistics, "Hilbert Cell Cost");
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "Hilbert_cost", inf::FieldAttributes::Node(), node_hilbert_cost));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "Hilbert_cost", inf::FieldAttributes::Cell(), cell_hilbert_cost));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "cell_flattness", inf::FieldAttributes::Cell(), cell_flattness_cost));
    }
    void appendEdgeStencilCount(MessagePasser mp,
                                const MeshInterface& mesh,
                                std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto edge_stencil_size = calcStencilSize(NodeToNode::build(mesh));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "plot.edge-stencil-size", inf::FieldAttributes::Node(), 1, edge_stencil_size));
    }
    void appendPartitionFields(MessagePasser mp,
                               const MeshInterface& mesh,
                               std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        std::vector<double> node_owner(mesh.nodeCount());
        for (int n = 0; n < mesh.nodeCount(); n++) {
            node_owner[n] = double(mesh.nodeOwner(n));
        }

        std::vector<double> cell_owner(mesh.cellCount());
        for (int c = 0; c < mesh.cellCount(); c++) {
            cell_owner[c] = double(mesh.cellOwner(c));
        }

        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "partition", inf::FieldAttributes::Node(), 1, node_owner));
        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "partition", inf::FieldAttributes::Cell(), 1, cell_owner));
    }

    void convertToNodes(MessagePasser mp,
                        const MeshInterface& mesh,
                        std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        if (inf::is2D(mp, mesh)) {
            for (auto& field : fields)
                if (field->association() == FieldAttributes::Cell())
                    field = FieldTools::cellToNode_simpleAverage(mesh, *field);
        } else {
            for (auto& field : fields)
                if (field->association() == FieldAttributes::Cell())
                    field = FieldTools::cellToNode_volume(mesh, *field);
        }
    }

    void convertToCells(const MeshInterface& mesh,
                        std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        for (auto& field : fields)
            if (field->association() == FieldAttributes::Node())
                field = FieldTools::nodeToCell(mesh, *field);
    }

    std::vector<std::shared_ptr<FieldInterface>> importFieldsAndMaybeAddSome(
        MessagePasser mp,
        std::shared_ptr<MeshInterface> mesh,
        const Parfait::CommandLineMenu& m,
        std::string filename) const {
        std::vector<std::shared_ptr<FieldInterface>> fields = importFields(mp, mesh, m, filename);
        bool should_add_tags = false;
        if (m.has("--surface")) should_add_tags = true;
        if (m.has("--tags")) should_add_tags = true;
        if (m.has("select")) should_add_tags = false;
        if (m.has("clean")) should_add_tags = false;
        if (should_add_tags) {
            fields.push_back(getCellTags(*mesh));
        }
        if (m.has("partition")) {
            appendPartitionFields(mp, *mesh, fields);
        }
        if (m.has("gids")) {
            appendGIDFields(mp, *mesh, fields);
        }
        if (m.has("distance")) {
            appendDistance(
                mp, *mesh, Parfait::StringTools::parseIntegerList(m.get("distance")), fields);
        }
        if (m.has("edge-length")) {
            appendEdgeLengths(mp, *mesh, fields);
        }
        if (m.has("edge-stencil-count")) {
            appendEdgeStencilCount(mp, *mesh, fields);
        }
        if (m.has("area")) {
            appendSurfaceArea(mp, *mesh, fields);
        }
        if (m.has("cell-quality")) {
            appendCellQuality(mp, *mesh, fields);
        }
        if (m.has("smoothness")) {
            appendMeshSmoothness(mp, *mesh, fields);
        }
        if (m.has("node-owners")) {
            appendNodeOwners(mp, *mesh, fields);
        }
        return fields;
    }

    struct MeshAndFields {
        std::shared_ptr<MeshInterface> mesh;
        std::vector<std::shared_ptr<FieldInterface>> fields;
    };

    MeshAndFields filter(MessagePasser mp,
                         Parfait::CommandLineMenu m,
                         std::shared_ptr<inf::MeshInterface> mesh,
                         const std::vector<std::shared_ptr<FieldInterface>>& fields) const {
        auto mesh_dimensionality = maxCellDimensionality(mp, *mesh);
        auto cell_selector = getCellSelector(mp, m, mesh_dimensionality);
        if (cell_selector->numSelectors() != 0) {
            mp_rootprint("Applying filters to mesh\n");
            CellIdFilter filter(mp.getCommunicator(), mesh, cell_selector);
            auto filtered_mesh = filter.getMesh();

            mp_rootprint("Applying filters to field data\n");
            std::vector<std::shared_ptr<FieldInterface>> filtered_fields;
            for (auto& f : fields) filtered_fields.emplace_back(filter.apply(f));
            return {filtered_mesh, filtered_fields};
        } else {
            return {mesh, fields};
        }
    }

    std::vector<int> calcStencilSize(const std::vector<std::vector<int>>& mesh_connectivity) const {
        std::vector<int> stencil_sizes;
        stencil_sizes.reserve(mesh_connectivity.size());
        for (const auto& s : mesh_connectivity) stencil_sizes.push_back(s.size());
        return stencil_sizes;
    }

    void smooth(MessagePasser mp, MeshAndFields& mesh_and_fields) const {
        auto& mesh = mesh_and_fields.mesh;
        for (auto& f : mesh_and_fields.fields) {
            mp_rootprint("Smoothing field %s\n", f->name().c_str());
            f = inf::FieldTools::laplacianSmoothing(*mesh, *f);
        }
    }

    void plot(std::shared_ptr<MeshInterface> mesh,
              std::vector<std::shared_ptr<FieldInterface>> fields,
              Parfait::CommandLineMenu m,
              MessagePasser mp,
              std::string output_base) const {
        if (atNodes(m)) convertToNodes(mp, *mesh, fields);
        if (atCells(m)) convertToCells(*mesh, fields);
        auto mesh_and_fields = filter(mp, m, mesh, fields);
        if (m.has("smooth")) {
            smooth(mp, mesh_and_fields);
        }

        for (auto field : fields) {
            printf("2 about to plot field: %s\n", field->name().c_str());
        }
        exportMeshAndFields(output_base, mesh_and_fields.mesh, m, mp, mesh_and_fields.fields);

        if (m.has("explode")) {
            auto name = m.get(Alias::postprocessor());
            name = selectDefaultPostProcessorFromMeshFilename(output_base);
            double scale = 0.75;
            if (m.has("scale")) {
                scale = m.getDouble("scale");
            }
            auto exploded_mesh = inf::explodeMesh(mp, mesh_and_fields.mesh, scale);
            inf::shortcut::visualize(
                m.get(Alias::outputFileBase()) + ".explode", mp, exploded_mesh, {}, name);
        }
    }

    void writeMapbcFile(const Parfait::CommandLineMenu& m,
                        const MessagePasser& mp,
                        const MeshInterface& mesh) const {
        using namespace Parfait::StringTools;
        auto boundary_cell_dimensionality = maxCellDimensionality(mp, mesh) - 1;

        if (mp.Rank() == 0) {
            auto output = m.get(Alias::outputFileBase());
            output = stripExtension(output);
            if (contains(output, "lb8") or contains(output, "b8"))
                output = split(output, ".").front();
            auto mapbc_filename = output + ".mapbc";

            constexpr int freestream_bc_enum = 5000;

            Parfait::BoundaryConditionMap bc_map;
            for (const auto& tag_to_name :
                 inf::extractTagsToNames(mp, mesh, boundary_cell_dimensionality)) {
                bc_map[tag_to_name.first] = {freestream_bc_enum, tag_to_name.second};
            }
            Parfait::writeMapbcFile(mapbc_filename, bc_map);
        }
    }
};

CREATE_INF_SUBCOMMAND(PlotCommand)
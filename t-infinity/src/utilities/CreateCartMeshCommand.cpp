#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/Snap.h>
#include <t-infinity/RepartitionerInterface.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/Cell.h>

using namespace inf;

class CreateCartMeshCommand : public SubCommand {
  public:
    std::string description() const override { return "generate a cartesian ugrid mesh"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::outputFileBase(), "Output mesh filename", true);
        m.addParameter({"--dimensions", "-d"}, "cartesian mesh cell dimensions nx ny nz", true);
        m.addParameter({"--lo"}, "cartesian mesh extent lower coordinates", false, "0 0 0");
        m.addParameter({"--hi"}, "cartesian mesh extent higher coordinates", false, "1 1 1");
        m.addFlag({"--sphere"}, "Map cart mesh to sphere");
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "CC_ReProcessor");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = createMesh(m, mp);
        ParallelUgridExporter::write(m.get(Alias::outputFileBase()), *mesh, mp);
    }

  private:
    std::vector<int> toInts(const std::vector<std::string>& s) const {
        std::vector<int> converted(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            converted[i] = Parfait::StringTools::toInt(s[i]);
        }
        return converted;
    }
    std::vector<double> toDoubles(const std::vector<std::string>& s) const {
        std::vector<double> converted(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            converted[i] = Parfait::StringTools::toDouble(s[i]);
        }
        return converted;
    }
    void setSurfaceCellTags(MeshInterface& mesh) const {
        auto tinf_mesh = dynamic_cast<TinfMesh*>(&mesh);
        PARFAIT_ASSERT(tinf_mesh, "expected a mutable TinfMesh");
        auto& mesh_data = tinf_mesh->mesh;
        for (int cell_id = 0; cell_id < mesh.cellCount(); ++cell_id) {
            if (mesh.cellType(cell_id) != MeshInterface::QUAD_4) continue;
            auto cell = Cell(mesh, cell_id);
            Parfait::Point<double> n = cell.faceAreaNormal(0);
            n.normalize();

            auto type_and_id = tinf_mesh->cellIdToTypeAndLocalId(cell_id);
            auto& tag = mesh_data.cell_tags[type_and_id.first][type_and_id.second];
            if (n.approxEqual({1, 0, 0})) tag = 1;
            if (n.approxEqual({-1, 0, 0})) tag = 2;
            if (n.approxEqual({0, 1, 0})) tag = 3;
            if (n.approxEqual({0, -1, 0})) tag = 4;
            if (n.approxEqual({0, 0, 1})) tag = 5;
            if (n.approxEqual({0, 0, -1})) tag = 6;
        }
    }
    Parfait::Point<double> parsePointFromVexingCommandLine(const Parfait::CommandLineMenu& m, std::string key) const {
        auto args = m.getAsVector(key);
        // try to get 3 doubles as a vector
        if (args.size() == 3) {
            auto doubles = toDoubles(args);
            return {doubles[0], doubles[1], doubles[2]};
        }
        // try to get 3 doubles from a space delimited string
        args = Parfait::StringTools::split(m.get(key), " ");
        if (args.size() == 3) {
            auto doubles = toDoubles(args);
            return {doubles[0], doubles[1], doubles[2]};
        }
        // try to get 3 doubles from a mix of comma and space delimited strings
        auto entry = m.get(key);
        entry = Parfait::StringTools::findAndReplace(entry, ",", " ");
        args = Parfait::StringTools::split(m.get(key), " ");
        if (args.size() == 3) {
            auto doubles = toDoubles(args);
            return {doubles[0], doubles[1], doubles[2]};
        }
        PARFAIT_THROW("Could not parse point from: " + m.get(key));
    }

    std::vector<int> getCellDimensions(const Parfait::CommandLineMenu& m) const {
        auto dimensions = toInts(m.getAsVector("dimensions"));
        PARFAIT_ASSERT(dimensions.size() == 3, "Could not parse all three cell dimensions");
        return dimensions;
    }

    std::shared_ptr<MeshInterface> createMesh(const Parfait::CommandLineMenu& m, const MessagePasser& mp) const {
        std::shared_ptr<inf::TinfMesh> cube = nullptr;
        auto dimensions = getCellDimensions(m);
        auto lo = parsePointFromVexingCommandLine(m, "lo");
        auto hi = parsePointFromVexingCommandLine(m, "hi");
        Parfait::Extent<double> e(lo, hi);
        std::shared_ptr<inf::TinfMesh> mesh;
        if (m.has("sphere")) {
            mesh = inf::CartMesh::createSphere(dimensions[0], dimensions[1], dimensions[2], e);
        } else {
            mesh = inf::CartMesh::create(dimensions[0], dimensions[1], dimensions[2], e);
        }
        return mesh;
    }
};

CREATE_INF_SUBCOMMAND(CreateCartMeshCommand)
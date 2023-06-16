#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <parfait/LinearPartitioner.h>
#include <t-infinity/Snap.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <thread>
#include <chrono>
#include <stdio.h>
#include "MockMeshes.h"
#include <t-infinity/CartMesh.h>
#include <t-infinity/FieldLoader.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/Shortcuts.h>

using namespace inf;

TEST_CASE("create a Snap object") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    std::vector<long> global_ids{2, 3, 1, 4, 0, 5};
    std::vector<bool> do_own(global_ids.size(), true);

    std::vector<double> density_vec = {5, 4, 3, 2, 1, 0};
    auto node_association = FieldAttributes::Node();
    auto density = std::make_shared<inf::VectorFieldAdapter>("density", node_association, 1, density_vec);

    Snap snap(mp.getCommunicator());
    snap.setTopology(node_association, global_ids, do_own);
    snap.add(density);

    SECTION("Add a field and check it") {
        auto d = snap.retrieve("density");
        REQUIRE(4.0 == d->at(1)[0]);
    }

    SECTION("throw if fields don't make sense") {
        std::string invalid_field = "dennnsity";
        REQUIRE_THROWS(snap.retrieve(invalid_field));
    }

    SECTION("dump snap file") {
        auto hash = Parfait::StringTools::randomLetters(6);
        snap.writeFile("dog" + hash + ".snap");
        Snap snap2(mp.getCommunicator());
        snap2.setTopology(node_association, {2, 4}, {true, true});
        snap2.load("dog" + hash + ".snap");
        auto d = snap2.retrieve("density");
        REQUIRE(2.0 == d->at(1)[0]);
    }

    SECTION("multiple fields") {
        auto hash = Parfait::StringTools::randomLetters(6);
        std::vector<double> viscosity_vec = {.3, .4, .5, .7, .6, .3};
        auto viscosity = std::make_shared<inf::VectorFieldAdapter>("viscosity", node_association, 1, viscosity_vec);
        snap.add(viscosity);
        snap.writeFile("dog" + hash + ".snap");
        Snap snap2(mp.getCommunicator());
        snap2.setTopology(node_association, {1, 2}, {true, true});
        snap2.load("dog" + hash + ".snap");
        auto v = snap2.retrieve("viscosity");
        auto d = snap2.retrieve("density");
        REQUIRE(0.5 == v->at(0)[0]);
        REQUIRE(3 == d->at(0)[0]);
    }

    SECTION("The same GID in multiple local spots when reading ghosts") {
        auto hash = Parfait::StringTools::randomLetters(6);
        snap.writeFile("dog" + hash + ".snap");
        Snap snap2(mp.getCommunicator());
        snap2.setTopology(node_association, {2, 4, 2}, {false, true, false});
        snap2.load("dog" + hash + ".snap");
        auto d = snap2.retrieve("density");
        REQUIRE(d->size() == 3);
        REQUIRE(5.0 == d->at(0)[0]);
        REQUIRE(2.0 == d->at(1)[0]);
        REQUIRE(5.0 == d->at(2)[0]);
    }
}

TEST_CASE("2-proc test with outdated ghost data") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    if (2 != mp.NumberOfProcesses()) return;
    auto hash = Parfait::StringTools::randomLetters(6);
    std::string snap_file = "dog.outgoing" + hash + ".snap";
    std::vector<long> global_ids;
    std::vector<double> density_vec;
    std::vector<bool> do_own;
    std::vector<double> pokemon_mass_vec;
    if (0 == mp.Rank()) {
        do_own = {true, true, true};
        global_ids = {3, 0, 1};
        density_vec = {.1, .2, .3};
        pokemon_mass_vec = {9, 8, 7};
    } else {
        do_own = {true, false};
        global_ids = {2, 0};
        density_vec = {0.15, -.99};
        pokemon_mass_vec = {6, 99};
    }
    auto node_association = FieldAttributes::Node();
    auto density = std::make_shared<inf::VectorFieldAdapter>("density", FieldAttributes::Node(), 1, density_vec);
    auto pokemon_mass =
        std::make_shared<inf::VectorFieldAdapter>("pokemon mass", FieldAttributes::Node(), 1, pokemon_mass_vec);
    Snap outgoing_snap(mp.getCommunicator());
    outgoing_snap.setTopology(node_association, global_ids, do_own);
    outgoing_snap.add(density);
    outgoing_snap.add(pokemon_mass);
    outgoing_snap.writeFile(snap_file);

    if (0 == mp.Rank()) {
        std::vector<long> new_global_ids = {0, 1};
        Snap incoming_snap(mp.getCommunicator());
        incoming_snap.setTopology(node_association, new_global_ids, {true, true});
        incoming_snap.load(snap_file);
        auto d0 = incoming_snap.retrieve("density");
        REQUIRE(0.2 == d0->at(0)[0]);
        REQUIRE(0.3 == d0->at(1)[0]);
        auto mass = incoming_snap.retrieve("pokemon mass");
        REQUIRE(7 == mass->at(1)[0]);
    } else {
        std::vector<long> new_global_ids = {2, 3};
        Snap incoming_snap(mp.getCommunicator());
        incoming_snap.setTopology(node_association, new_global_ids, {true, true});
        incoming_snap.load(snap_file);
        auto d2 = incoming_snap.retrieve("density");
        REQUIRE(0.15 == d2->at(0)[0]);
        REQUIRE(0.1 == d2->at(1)[0]);
        auto mass = incoming_snap.retrieve("pokemon mass");
        REQUIRE(9 == mass->at(1)[0]);
    }
}

TEST_CASE("Snap complains if you try to write a file before setting a topology") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    Snap snap(mp.getCommunicator());
    std::vector<double> density_vec = {5, 4, 3, 2, 1, 0};
    auto density = std::make_shared<inf::VectorFieldAdapter>("density", FieldAttributes::Node(), 1, density_vec);
    snap.add(density);
    REQUIRE_THROWS(snap.writeFile("no.snap"));
}

TEST_CASE("Snap complains if you try to read a file before setting a topology") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    Snap snap(mp.getCommunicator());
    REQUIRE_THROWS(snap.load("nope.snap"));
}

TEST_CASE("Mulltiple assiciations") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    if (2 != mp.NumberOfProcesses()) return;
    auto node_association = FieldAttributes::Node();
    auto cell_association = FieldAttributes::Cell();

    std::vector<long> cell_global_ids;
    std::vector<bool> cell_do_own;
    std::vector<long> node_global_ids;
    std::vector<bool> node_do_own;

    std::vector<double> density_vec = {5, 4, 3, 2, 1, 0};
    std::vector<double> energy_vec = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    if (mp.Rank() == 0) {
        cell_global_ids = {4, 5, 6, 7, 8};
        cell_do_own.resize(cell_global_ids.size(), true);
        node_global_ids = {0, 1, 2, 3};
        node_do_own.resize(node_global_ids.size(), true);
        energy_vec = {4, 5, 6, 7, 8};
        density_vec = {5, 4, 3, 2};
    } else {
        cell_global_ids = {0, 1, 2, 3};
        cell_do_own.resize(cell_global_ids.size(), true);
        node_global_ids = {4, 5};
        node_do_own.resize(node_global_ids.size(), true);
        energy_vec = {0, 1, 2, 3};
        density_vec = {1, 0};
    }

    auto hash = Parfait::StringTools::randomLetters(6);

    {
        Snap snap(mp.getCommunicator());
        snap.setTopology(node_association, node_global_ids, node_do_own);
        snap.setTopology(cell_association, cell_global_ids, cell_do_own);
        auto density = std::make_shared<inf::VectorFieldAdapter>("density", FieldAttributes::Node(), 1, density_vec);
        auto energy = std::make_shared<inf::VectorFieldAdapter>("energy", FieldAttributes::Cell(), 1, energy_vec);
        density->setAdapterAttribute("units", "kg/m^3");
        snap.add(density);
        snap.add(energy);
        snap.writeFile("both" + hash + ".snap");
    }

    {
        Snap snap(mp.getCommunicator());
        snap.setTopology(node_association, node_global_ids, node_do_own);
        snap.setTopology(cell_association, cell_global_ids, cell_do_own);
        snap.load("both" + hash + ".snap");
        auto density = snap.retrieve("density");
        REQUIRE(density.get()->association() == inf::FieldAttributes::Node());
        REQUIRE(density->attribute("units") == "kg/m^3");

        for (size_t i = 0; i < density_vec.size(); i++) {
            REQUIRE(density->at(i)[0] == density_vec[i]);
        }
        auto energy = snap.retrieve("energy");
        REQUIRE(energy.get()->association() == inf::FieldAttributes::Cell());
        for (size_t i = 0; i < energy_vec.size(); i++) {
            REQUIRE(energy->at(i)[0] == energy_vec[i]);
        }
    }
}

TEST_CASE("can save and re-read non-scalar fields") {
    auto hash = Parfait::StringTools::randomLetters(6);
    std::vector<long> node_global_ids;
    std::vector<bool> node_do_own;

    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) {
        return;
    }

    std::vector<double> velocity;

    node_global_ids = {0, 1, 2, 3};

    node_do_own.resize(node_global_ids.size(), true);
    velocity.resize(3 * node_global_ids.size());
    for (size_t i = 0; i < node_global_ids.size(); i++) {
        velocity[3 * i + 0] = i;
        velocity[3 * i + 1] = 10 * i;
        velocity[3 * i + 2] = 100 * i;
    }

    auto velocity_field =
        std::make_shared<inf::VectorFieldAdapter>("velocity", inf::FieldAttributes::Node(), 3, velocity);

    inf::Snap snap(mp.getCommunicator());
    snap.setTopology(inf::FieldAttributes::Node(), node_global_ids, node_do_own);
    snap.add(velocity_field);

    snap.writeFile(hash);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    inf::Snap read_snap(mp.getCommunicator());
    snap.setTopology(inf::FieldAttributes::Node(), node_global_ids, node_do_own);
    snap.load(hash);
    REQUIRE(snap.has("velocity"));

    std::shared_ptr<inf::VectorFieldAdapter> read_field =
        std::dynamic_pointer_cast<VectorFieldAdapter>(snap.retrieve("velocity"));
    REQUIRE(read_field->size() == int(node_global_ids.size()));
    REQUIRE(read_field->blockSize() == 3);

    auto& read_velocity = read_field->getVector();

    for (size_t i = 0; i < node_global_ids.size(); i++) {
        REQUIRE(read_velocity[3 * i + 0] == i);
        REQUIRE(read_velocity[3 * i + 1] == 10 * i);
        REQUIRE(read_velocity[3 * i + 2] == 100 * i);
    }
}

TEST_CASE("Write map to file and read it") {
    std::unordered_map<std::string, std::string> my_map;
    my_map["sail"] = "boat";
    my_map["cup"] = "cake";
    my_map["hammer"] = "time";

    auto hash = Parfait::StringTools::randomLetters(6);
    auto fp = FileStreamer::create("default");
    fp->openWrite(hash.c_str());
    SnapIOHelpers::writeMapToFile(*fp, my_map);
    fp->close();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    fp->openRead(hash.c_str());

    auto my_read_map = SnapIOHelpers::readMapFromFile(*fp);
    REQUIRE(my_read_map.at("sail") == "boat");
    REQUIRE(my_read_map.at("cup") == "cake");
    REQUIRE(my_read_map.at("hammer") == "time");

    uint64_t map_size = SnapIOHelpers::calcMapSize(my_map);
    uint64_t expected_size = sizeof(int64_t) + 4 + 4 + 3 + 4 + 6 + 4 + sizeof(int64_t) * 2 * 3;
    REQUIRE(expected_size == map_size);
}

TEST_CASE("Ensure generated snap topologies have correct ownership") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    inf::Snap snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    auto cell_top = snap.copyTopology(inf::FieldAttributes::Cell());
    for (int c = 0; c < mesh->cellCount(); c++) {
        bool do_own = mesh->cellOwner(c) == mp.Rank();
        REQUIRE(do_own == cell_top.do_own[c]);
        REQUIRE(mesh->globalCellId(c) == cell_top.global_ids[c]);
    }

    auto node_top = snap.copyTopology(inf::FieldAttributes::Node());
    for (int n = 0; n < mesh->nodeCount(); n++) {
        bool do_own = mesh->nodeOwner(n) == mp.Rank();
        REQUIRE(do_own == node_top.do_own[n]);
        REQUIRE(mesh->globalNodeId(n) == node_top.global_ids[n]);
    }
}

TEST_CASE("Get Summary with sparse reading") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::string filename;
    {
        inf::Snap snap(mp.getCommunicator());
        auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
        snap.addMeshTopologies(*mesh);

        std::vector<double> density_vec;
        for (int n = 0; n < mesh->nodeCount(); n++) {
            density_vec.push_back(double(n));
        }
        auto density = std::make_shared<inf::VectorFieldAdapter>("density", FieldAttributes::Node(), 1, density_vec);
        std::vector<double> energy_vec;
        for (int c = 0; c < mesh->cellCount(); c++) {
            energy_vec.push_back(double(c));
        }
        auto energy = std::make_shared<inf::VectorFieldAdapter>("energy", FieldAttributes::Cell(), 1, energy_vec);
        density->setAdapterAttribute("units", "kg/m^3");
        snap.add(density);
        snap.add(energy);
        auto hash = Parfait::StringTools::randomLetters(5);
        filename = "test" + hash + ".snap";
        snap.writeFile(filename);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    auto snap = inf::Snap(mp.getCommunicator());
    auto summary = snap.readSummary(filename);
    REQUIRE(summary.size() == 2);
    REQUIRE(summary[0].at("name") == "density");
    REQUIRE(summary[1].at("name") == "energy");
}

TEST_CASE("Set default topology for snap object") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::string filename = "test_default_topology" + Parfait::StringTools::randomLetters(5) + ".snap";
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    {
        Snap snap(mp);
        snap.addMeshTopologies(*mesh);
        snap.add(std::make_shared<inf::VectorFieldAdapter>(
            "node field", FieldAttributes::Node(), 1, std::vector<double>(mesh->nodeCount(), 2.24)));
        snap.add(std::make_shared<inf::VectorFieldAdapter>(
            "cell field", FieldAttributes::Cell(), 1, std::vector<double>(mesh->cellCount(), -1.4)));
        snap.writeFile(filename);
    }

    Snap snap(mp);
    snap.setDefaultTopology(filename);
    {
        auto range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(), 1600, mp.NumberOfProcesses());
        size_t cell_count = range.end - range.start;
        auto topology = snap.copyTopology(FieldAttributes::Cell());
        REQUIRE(cell_count == topology.global_ids.size());
        REQUIRE(cell_count == topology.do_own.size());
    }
    {
        auto range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(), 1331, mp.NumberOfProcesses());
        size_t node_count = range.end - range.start;
        auto topology = snap.copyTopology(FieldAttributes::Node());
        REQUIRE(node_count == topology.global_ids.size());
        REQUIRE(node_count == topology.do_own.size());
    }
    snap.load(filename);
    REQUIRE_NOTHROW(snap.retrieve("node field"));
    REQUIRE_NOTHROW(snap.retrieve("cell field"));
    if (mp.Rank() == 0) REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Can write and read fields from shortcut") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::string filename = Parfait::StringTools::randomLetters(5) + ".snap";

    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    std::vector<double> node_field(mesh->nodeCount(), 7.77);
    int node_id = 4;
    node_field[node_id] = 8.88;
    inf::shortcut::visualize_at_nodes(filename, mp, mesh, {node_field}, {"my-cool-node-field"});

    auto fields = inf::shortcut::loadFields(mp, filename, *mesh);
    REQUIRE(fields.size() == 1);
    REQUIRE(fields[0]->name() == "my-cool-node-field");
    REQUIRE(fields[0]->getDouble(node_id) == 8.88);
    REQUIRE(fields[0]->getDouble(0) == 7.77);
}
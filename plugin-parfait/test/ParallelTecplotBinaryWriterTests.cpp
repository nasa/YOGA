#include <RingAssertions.h>
#include <NodeCenteredPreProcessor/NC_PreProcessor.h>
#include <Viz/ParallelTecplotWriter.h>
#include <parfait/TecplotBinaryReader.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>

template <typename Container>
bool contains(const Container& container, const Parfait::Point<double>& p) {
    double closest_distance = std::numeric_limits<double>::max();
    for (auto& some_p : container) {
        double d = (p - some_p).magnitude();
        closest_distance = std::min(d, closest_distance);
        if (d < 1.0e-6) return true;
    }
    return false;
}

TEST_CASE("Get parallel range of field in parallel") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::vector<double> test_function;
    test_function.push_back(double(mp.Rank()));
    std::vector<bool> do_own = {true};

    auto field = std::make_shared<inf::VectorFieldAdapter>("test", inf::FieldAttributes::Node(), 1, test_function);
    auto min_max = ParallelTecplotZoneWriter::getMinMax_field(mp, field, do_own);
    REQUIRE(min_max[0] == 0);
    REQUIRE(min_max[1] == double(mp.NumberOfProcesses() - 1));
}

TEST_CASE("Can write a tecplt binary file in parallel") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto pp = NC_PreProcessor();
    std::string filename = GRIDS_DIR;
    filename += "lb8.ugrid/6cell.lb8.ugrid";
    auto mesh = pp.load(mp.getCommunicator(), filename);

    auto volume_filter = inf::FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
    ParallelTecplotWriter writer("6cell.plt", volume_filter->getMesh(), mp.getCommunicator());
    writer.write();

    sleep(1);  // let the filesystem have a second to finish writing.

    Parfait::tecplot::BinaryReader reader("6cell.plt");
    reader.read();
    auto min_max = reader.getRangeOfField("X");
    REQUIRE(min_max[0] == -1.0);
    REQUIRE(min_max[1] == 1.0);

    min_max = reader.getRangeOfField("Y");
    REQUIRE(min_max[0] == -1.0);
    REQUIRE(Approx(min_max[1]).margin(1e-6) == 0.0);

    min_max = reader.getRangeOfField("Z");
    REQUIRE(min_max[0] == Approx(-0.8660253));
    REQUIRE(min_max[1] == Approx(0.8660253));
    auto points_read = reader.getPoints();
    REQUIRE(points_read.size() == 14);
    REQUIRE(contains(points_read, Parfait::Point<double>{-1, 0, 0}));
    REQUIRE(contains(points_read, Parfait::Point<double>{-0.5, 0, -0.8660253}));

    for (int n = 0; n < mesh->nodeCount(); n++) {
        auto p = mesh->node(n);
        REQUIRE(contains(points_read, p));
    }

    // for(auto& p : points_read){
    //     printf("%s \n", p.to_string().c_str());
    // }

    auto cells = reader.getCells();
    REQUIRE(cells.size() == 6);
    for (auto& c : cells) {
        printf("[%s]\n", Parfait::to_string(c).c_str());
    }
}

TEST_CASE("Will use the largest solution time field attribute as the file solution time") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    double dt = 1.0 / 50.0;

    for (int i = 0; i < 50; i++) {
        double time = 5.0 * dt * i;
        auto function = [=](double x, double y, double z) {
            return cos(10 * M_PI * x + time) + sin(10 * M_PI * y + time);
        };
        auto f = inf::FieldTools::createNodeFieldFromCallback("test-field", *mesh, function);
        f->setAdapterAttribute(inf::FieldAttributes::solutionTime(), std::to_string(time));
        // disabling output for unit tests.  But if uncommented this should create a series of files that
        // tecplot knows how to animate
        //        inf::shortcut::visualize("time." + std::to_string(i) + ".plt", mp, mesh, {f});
    }
}

TEST_CASE("Can add fields and maintain user requested order") {
    std::vector<double> mock_field(10);
    std::vector<std::shared_ptr<inf::FieldInterface>> ordered_fields;
    {
        auto f = std::make_shared<inf::VectorFieldAdapter>("dog", inf::FieldAttributes::Node(), mock_field);
        f->setAdapterAttribute(inf::FieldAttributes::outputOrder(), "1");
        ParallelTecplotWriter::addInOrder(ordered_fields, f);
        REQUIRE(ordered_fields.size() == 1);
        REQUIRE(ordered_fields[0]->name() == "dog");
    }
    {
        auto f = std::make_shared<inf::VectorFieldAdapter>("fish", inf::FieldAttributes::Node(), mock_field);
        f->setAdapterAttribute(inf::FieldAttributes::outputOrder(), "3");
        ParallelTecplotWriter::addInOrder(ordered_fields, f);
        REQUIRE(ordered_fields.size() == 2);
        REQUIRE(ordered_fields[0]->name() == "dog");
        REQUIRE(ordered_fields[1]->name() == "fish");
    }
    {
        auto f = std::make_shared<inf::VectorFieldAdapter>("bear", inf::FieldAttributes::Node(), mock_field);
        f->setAdapterAttribute(inf::FieldAttributes::outputOrder(), "2");
        ParallelTecplotWriter::addInOrder(ordered_fields, f);
        REQUIRE(ordered_fields.size() == 3);
        REQUIRE(ordered_fields[0]->name() == "dog");
        REQUIRE(ordered_fields[1]->name() == "bear");
        REQUIRE(ordered_fields[2]->name() == "fish");
    }
    {
        auto f = std::make_shared<inf::VectorFieldAdapter>("pig", inf::FieldAttributes::Node(), mock_field);
        f->setAdapterAttribute(inf::FieldAttributes::outputOrder(), "12");
        ParallelTecplotWriter::addInOrder(ordered_fields, f);
        REQUIRE(ordered_fields.size() == 4);
        REQUIRE(ordered_fields[0]->name() == "dog");
        REQUIRE(ordered_fields[1]->name() == "bear");
        REQUIRE(ordered_fields[2]->name() == "fish");
        REQUIRE(ordered_fields[3]->name() == "pig");
    }
    {
        auto f = std::make_shared<inf::VectorFieldAdapter>("bull", inf::FieldAttributes::Node(), mock_field);
        f->setAdapterAttribute(inf::FieldAttributes::outputOrder(), "pikachu");
        REQUIRE_NOTHROW(ParallelTecplotWriter::addInOrder(ordered_fields, f));
        REQUIRE(ordered_fields.size() == 5);
        REQUIRE(ordered_fields[0]->name() == "dog");
        REQUIRE(ordered_fields[1]->name() == "bear");
        REQUIRE(ordered_fields[2]->name() == "fish");
        REQUIRE(ordered_fields[3]->name() == "pig");
        REQUIRE(ordered_fields[4]->name() == "bull");
    }
}
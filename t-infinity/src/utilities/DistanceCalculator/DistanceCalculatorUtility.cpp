#include <Tracer.h>
#include <parfait/DistanceTree.h>
#include <parfait/Flatten.h>
#include <parfait/ImportedUgrid.h>
#include <parfait/ImportedUgridFactory.h>
#include <parfait/ParallelExtent.h>
#include <parfait/Point.h>
#include <parfait/STL.h>
#include <parfait/StringTools.h>
#include <parfait/Timing.h>
#include <parfait/ToString.h>
#include <t-infinity/DistanceCalculator.h>

#include <iostream>
#include <set>
#include <vector>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/Communicator.h>
#include <t-infinity/Version.h>

bool shouldWriteToAscii(std::vector<std::string> args) {
    for (auto a : args) {
        if (a == "--write-ascii") return true;
    }
    return false;
}

std::vector<bool> buildDoOwn(const inf::MeshInterface& mesh) {
    std::vector<bool> do_own(mesh.nodeCount());
    for (int n = 0; n < mesh.nodeCount(); n++) {
        do_own[n] = mesh.ownedNode(n);
    }
    return do_own;
}

std::set<int> findAllTags(MessagePasser mp, const inf::MeshInterface& mesh) {
    std::set<int> tags;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.is2DCell(c)) {
            int tag = mesh.cellTag(c);
            tags.insert(tag);
        }
    }
    auto all_tags = mp.ParallelUnion(tags);
    return all_tags;
}

void tellUserTags(const std::set<int>& tags) {
    std::cout << "Using the following tags as walls: ";
    for (auto t : tags) std::cout << " " << t;
    std::cout << std::endl;
}

void printRequestedUnavailableTagsAndExit(MessagePasser mp,
                                          const std::set<int>& requested_tags,
                                          const std::set<int>& available_tags) {
    if (mp.Rank() == 0) {
        printf("ERROR: Requested tags invalid\n");
        printf("Requested tags: %s\n", Parfait::to_string(requested_tags).c_str());
        printf("\nMesh has tags: %s\n", Parfait::to_string(available_tags).c_str());
        printf("\n");
    }
    exit(1);
}

void validateTags(MessagePasser mp, const std::set<int>& requested_tags, const std::set<int>& available_tags) {
    if (requested_tags.empty()) printRequestedUnavailableTagsAndExit(mp, requested_tags, available_tags);
    for (auto t : requested_tags) {
        if (available_tags.count(t) == 0) {
            printRequestedUnavailableTagsAndExit(mp, requested_tags, available_tags);
        }
    }
    if (mp.Rank() == 0) tellUserTags(requested_tags);
}

std::vector<int> determineIfBoundaryNodes(const Parfait::ImportedUgrid& ugrid) {
    int nnodes = ugrid.nodes.size() / 3;
    std::vector<int> is_boundary(nnodes, 0);
    for (size_t t = 0; t < ugrid.triangles.size() / 3; t++) {
        for (int i = 0; i < 3; i++) {
            int n = ugrid.triangles[3 * t + i];
            is_boundary[n] = 1;
        }
    }
    for (size_t q = 0; q < ugrid.quads.size() / 4; q++) {
        for (int i = 0; i < 4; i++) {
            int n = ugrid.quads[4 * q + i];
            is_boundary[n] = 1;
        }
    }
    return is_boundary;
}

void writeToFile(std::string filename,
                 const std::vector<double>& dist,
                 const std::vector<Parfait::Point<double>>& xyz) {
    FILE* fp = fopen(filename.c_str(), "w");

    fprintf(fp, "#nodeId: x y z distance\n");
    for (size_t n = 0; n < dist.size(); n++) {
        fprintf(fp, "%ld: %1.15e %1.15e %1.15e %1.15e\n", n, xyz[n][0], xyz[n][1], xyz[n][2], dist[n]);
    }

    fclose(fp);
}

void printVersion(MessagePasser mp) {
    if (mp.Rank() == 0) std::cout << "ParallelDistanceCalculator VERSION: " << inf::version() << std::endl;
}

bool isVersionRequested(const std::vector<std::string>& arguments) {
    for (const auto& a : arguments) {
        if (a == "-v" or a == "--version") return true;
    }
    return false;
}

bool isAppropriateFilenameRequested(const std::vector<std::string>& arguments) {
    if (arguments.size() == 1) return false;
    auto should_be_filename = arguments[1];
    auto filename_parts = Parfait::StringTools::split(should_be_filename, ".");
    return (filename_parts.back() == "ugrid" or filename_parts.back() == "meshb" or filename_parts.back() == "cgns");
}

void printHelpAndExit(MessagePasser mp, int exit_code) {
    if (mp.Rank() == 0) {
        std::cout << "DistanceCalculator version " << inf::version() << std::endl;
        std::cout << "Usage: ./DistanceCalculator <filename> --tags 7 5 6 3" << std::endl;
        std::cout << " Options: " << std::endl;
        std::cout << "    --tags -t        : list of surface tags that will be considered walls [required]"
                  << std::endl;
        std::cout << "    --pre-processor  : The pre-processor plugin used for importing the mesh [optional]"
                  << std::endl;
        std::cout << "    --output-plugin  : The output plugin used for exporting distance [optional]" << std::endl;
    }
    mp.Barrier();
    exit(exit_code);
}

bool isBigEndian(std::string filename) { return filename.find(".b8.") != std::string::npos; }

std::vector<std::string> getArgsAsStrings(int argc, char** argv) {
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; i++) {
        arguments.emplace_back(argv[i]);
    }
    return arguments;
}

bool tagsRequestedWithCommas(MessagePasser mp, const std::vector<std::string>& arguments) {
    for (auto arg : arguments) {
        if (arg == "--commas" or arg == "-c") {
            return true;
        }
    }
    return false;
}

std::set<int> getTagsFromArgumentsWithCommas(MessagePasser mp, const std::vector<std::string>& arguments) {
    std::set<int> tags;
    for (size_t i = 0; i < arguments.size(); i++) {
        if (arguments[i] == "--commas" or arguments[i] == "-c") {
            auto all_tags_with_commas = arguments[i + 1];
            auto tags_as_strings = Parfait::StringTools::split(all_tags_with_commas, ",");
            for (auto tag_as_string : tags_as_strings) {
                tags.insert(Parfait::StringTools::toInt(tag_as_string));
            }
            return tags;
        }
    }
    return tags;
}

std::set<int> getTagsFromArgumentsWithSpaces(MessagePasser mp, const std::vector<std::string>& arguments) {
    std::set<int> tags;
    auto iter = std::find(arguments.begin(), arguments.end(), "-t");
    if (iter == arguments.end()) iter = std::find(arguments.begin(), arguments.end(), "--tags");
    if (iter == arguments.end()) {
        if (mp.Rank() == 0) printf("Error! No tags requested!\n");
        printHelpAndExit(mp, 1);
    }
    for (++iter; iter != arguments.end(); ++iter) {
        if (Parfait::StringTools::isInteger(*iter))
            tags.insert(Parfait::StringTools::toInt(*iter));
        else
            break;
    }
    return tags;
}

std::set<int> getTagsFromArguments(MessagePasser mp, const std::vector<std::string>& arguments) {
    if (tagsRequestedWithCommas(mp, arguments)) {
        return getTagsFromArgumentsWithCommas(mp, arguments);
    } else {
        return getTagsFromArgumentsWithSpaces(mp, arguments);
    }
}

std::string getFilename(MessagePasser mp, const std::vector<std::string>& arguments) {
    if (not isAppropriateFilenameRequested(arguments)) {
        printHelpAndExit(mp, 1);
    }
    return arguments[1];
}

std::string getMeshLoaderPluginName(const std::vector<std::string>& arguments) {
    std::string name = "NC_PreProcessor";
    for (size_t i = 0; i < arguments.size(); i++) {
        auto arg = arguments[i];
        if (arg == "--pre-processor") {
            name = arguments[i + 1];
        }
    }
    return name;
}

std::string getVisualizationPluginName(const std::vector<std::string>& arguments) {
    std::string name = "RefinePlugins";
    for (size_t i = 0; i < arguments.size(); i++) {
        auto arg = arguments[i];
        if (arg == "--output-plugin") {
            name = arguments[i + 1];
        }
    }
    return name;
}

int getMaxDepth(const std::vector<std::string>& arguments) {
    int max_depth = 10;
    for (size_t i = 0; i < arguments.size(); i++) {
        auto arg = arguments[i];
        if (arg == "--max-depth") {
            max_depth = Parfait::StringTools::toInt(arguments[i + 1]);
        }
    }
    return max_depth;
}

int getMaxObjectsPerVoxel(const std::vector<std::string>& arguments) {
    int max = 10;
    for (size_t i = 0; i < arguments.size(); i++) {
        auto arg = arguments[i];
        if (arg == "--max-objects-per-voxel") {
            max = Parfait::StringTools::toInt(arguments[i + 1]);
        }
    }
    return max;
}

void writeToFile(std::string filename, const std::vector<double>& dist, const std::vector<double>& xyz) {
    FILE* fp = fopen(filename.c_str(), "w");

    fprintf(fp, "#nodeId: x y z distance\n");
    for (size_t n = 0; n < dist.size(); n++) {
        fprintf(fp, "%ld: %1.15e %1.15e %1.15e %1.15e\n", n, xyz[3 * n + 0], xyz[3 * n + 1], xyz[3 * n + 2], dist[n]);
    }

    fclose(fp);
}

std::map<long, double> extractScalarFieldAsMapInRange(const inf::FieldInterface& field,
                                                      long start,
                                                      long end,
                                                      const std::map<long, int>& global_to_local,
                                                      const std::vector<bool>& do_own) {
    std::map<long, double> out;
    for (auto pair : global_to_local) {
        long global = pair.first;
        int local = pair.second;
        if (do_own[local] and (global >= start and global < end)) {
            double d;
            field.value(local, &d);
            out[global] = d;
        }
    }
    return out;
}

std::map<long, Parfait::Point<double>> extractPointsAsMapInRange(const inf::MeshInterface& mesh,
                                                                 long start,
                                                                 long end,
                                                                 const std::map<long, int>& global_to_local,
                                                                 const std::vector<bool>& do_own) {
    std::map<long, Parfait::Point<double>> out;
    for (auto pair : global_to_local) {
        long global = pair.first;
        int local = pair.second;
        if (do_own[local] and (global >= start and global < end)) {
            auto p = mesh.node(local);
            out[global] = p;
        }
    }
    return out;
}

int main(int argc, char* argv[]) {
    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);
    std::set<int> ranks_to_trace = {0, mp.NumberOfProcesses() - 1, mp.NumberOfProcesses() / 2};
    Tracer::initialize("trace", mp.Rank(), ranks_to_trace);
    Tracer::setDebug();
    std::vector<std::string> arguments = getArgsAsStrings(argc, argv);
    printVersion(mp);
    if (isVersionRequested(arguments)) {
        exit(0);
    }
    bool write_to_ascii = shouldWriteToAscii(arguments);
    auto tags = getTagsFromArguments(mp, arguments);
    std::string mesh_loader_plugin_name = getMeshLoaderPluginName(arguments);
    std::string visualization_plugin_name = getVisualizationPluginName(arguments);
    std::string mesh_filename = getFilename(mp, arguments);
    if (mp.Rank() == 0) printf("Using mesh pre-processor: %s\n", mesh_loader_plugin_name.c_str());
    auto pp = inf::getMeshLoader(inf::getPluginDir(), mesh_loader_plugin_name);

    auto begin = Parfait::Now();
    Tracer::begin("import mesh");
    auto mesh = pp->load(mp.getCommunicator(), mesh_filename);
    Tracer::end("import mesh");
    auto end = Parfait::Now();

    Tracer::traceMemory();
    auto current_mem_usage = Tracer::usedMemoryMB();
    if (mp.Rank() == 0)
        printf("Mesh importing took: %s, max memory %lu MB\n",
               Parfait::readableElapsedTimeAsString(begin, end).c_str(),
               current_mem_usage);
    auto all_tags = findAllTags(mp, *mesh.get());
    if (mp.Rank() == 0) printf("Mesh has tags: %s\n", Parfait::to_string(all_tags).c_str());
    validateTags(mp, tags, all_tags);

    begin = Parfait::Now();
    int max_depth = getMaxDepth(arguments);
    int max_objects_per_voxel = getMaxObjectsPerVoxel(arguments);
    auto dist = inf::calculateDistance(mp, *mesh.get(), tags, max_depth, max_objects_per_voxel);
    if (mp.Rank() == 0) printf("Done!\n");
    end = Parfait::Now();

    if (mp.Rank() == 0) std::cout << "Distance took: " << Parfait::readableElapsedTimeAsString(begin, end) << std::endl;
    auto distance_field =
        std::make_shared<inf::VectorFieldAdapter>("distance", inf::FieldAttributes::Node(), 1, dist);
    auto viz_plugin = inf::getVizFactory(inf::getPluginDir(), visualization_plugin_name);
    auto output_name = Parfait::StringTools::getPath(mesh_filename) +
                       Parfait::StringTools::stripExtension(Parfait::StringTools::stripPath(mesh_filename),
                                                            {"meshb", "lb8.ugrid", "b8.ugrid", "ugrid"}) +
                       "-distance.solb";
    auto viz = viz_plugin->create(output_name, mesh, inf::getCommunicator());
    viz->addField(distance_field);
    viz->visualize();

    if (write_to_ascii) {
        long start_id = 0;
        long end_id = inf::globalNodeCount(mp, *mesh);
        auto global_to_local = inf::GlobalToLocal::buildNode(*mesh.get());
        auto do_own = buildDoOwn(*mesh.get());
        auto full_distance = mp.GatherByOrdinalRange(
            extractScalarFieldAsMapInRange(*distance_field, start_id, end_id, global_to_local, do_own),
            start_id,
            end_id,
            0);
        auto full_points = mp.GatherByOrdinalRange(
            extractPointsAsMapInRange(*mesh.get(), start_id, end_id, global_to_local, do_own), start_id, end_id, 0);
        if (mp.Rank() == 0) {
            writeToFile("distance.out.txt", full_distance, full_points);
        }
    }

    MessagePasser::Finalize();
}

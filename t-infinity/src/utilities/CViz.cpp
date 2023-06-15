#include "CViz.h"
#include <MessagePasser/MessagePasser.h>
#include <parfait/StringTools.h>
#include <t-infinity/PancakeMeshAdapterFrom.h>
#include <t-infinity/CSolutionFieldExtractor.h>
#include <string>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/VizFactory.h>

std::string getVisualizationPluginName(const std::string filename) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "tec" or extension == "solb") {
        return "RefinePlugins";
    }
    return "ParfaitViz";
}

void mangle(visualize_with_options)(
    int fortran_comm, const char* filename_c, void* tinf_mesh, void* solution, const char* options_c) {
    std::string filename = filename_c;
    auto options = Parfait::StringTools::split(options_c, " ");
    auto plugin_name = getVisualizationPluginName(filename);

    auto comm = MPI_Comm_f2c(fortran_comm);
    auto mesh = std::make_shared<inf::PancakeMeshAdapterFrom>(tinf_mesh);
    auto viz_plugin = inf::getVizFactory(inf::getPluginDir(), plugin_name);

    auto mp = MessagePasser(comm);
    inf::MeshSanityChecker::checkAll(*mesh, mp);

    auto viz = viz_plugin->create(filename, mesh, comm);
    auto fields = extractFromCSolution(solution, mesh->nodeCount());
    for (auto& f : fields) viz->addField(f);
    // viz.add(filter.apply(f));
    viz->visualize();
}

void mangle(visualize)(int fortran_comm, const char* filename, void* tinf_mesh, void* solution) {
    mangle(visualize_with_options)(fortran_comm, filename, tinf_mesh, solution, "--plugin ParfaitViz");
}

void mangle(scatter_plot)(int fortran_comm,
                  const char* filename,
                  int npts,
                  void (*get_xyz)(int, double*),
                  void (*get_gid)(int, long*),
                  void (*get_scalar)(int, double*)) {
    printf(
        "This feature is turned off temporarily.  It can be turned back on after the visualization linking issue is "
        "resolved\n");
    //    MPI_Comm comm = *static_cast<MPI_Comm*>(comm_ptr);
    //    ScatterPlotter::plot(filename,MessagePasser(comm),[&](){return npts;},get_xyz,get_gid,get_scalar);
}

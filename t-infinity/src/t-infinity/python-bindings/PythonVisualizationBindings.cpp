#include "PybindIncludes.h"

#include <t-infinity/VizInterface.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/VisualizePartition.h>
#include <t-infinity/CellIdFilter.h>
#include <t-infinity/RecipeGenerator.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/StructuredMeshPlot.h>
#include <t-infinity/StructuredTinfMesh.h>
#include <t-infinity/SMesh.h>

using namespace inf;
namespace py = pybind11;

void PythonVisualizationBindings(py::module& m) {
    typedef std::shared_ptr<VizInterface> Visualizer;
    py::class_<VizInterface, Visualizer>(m, "PyViz")
        .def(py::init([](std::string output_filename,
                         std::shared_ptr<MeshInterface> mesh,
                         MessagePasser mp,
                         std::string directory,
                         std::string name) {
            return getVizFactory(directory, name)
                ->create(output_filename, mesh, mp.getCommunicator());
        }))
        .def("visualize", &VizInterface::visualize)
        .def("add", [](Visualizer self, std::shared_ptr<FieldInterface> f) { self->addField(f); });

    py::class_<StructuredPlotter>(m, "StructuredPlotter")
        .def(py::init<MessagePasser, std::string, std::shared_ptr<StructuredMesh>>())
        .def(py::init(
            [](MessagePasser mp, std::string filename, std::shared_ptr<StructuredTinfMesh> mesh) {
                return StructuredPlotter(mp, filename, mesh->shallowCopyToStructuredMesh());
            }))
        .def("add", &StructuredPlotter::addField)
        .def("writeGridFile", &StructuredPlotter::writeGridFile)
        .def("writeSolutionFile", &StructuredPlotter::writeSolutionFile)
        .def("visualize", &StructuredPlotter::visualize);

    m.def("visualizePartition", &visualizePartition);
    m.def("visualizeScript",
          [](std::shared_ptr<MeshInterface> mesh,
             const std::vector<std::shared_ptr<FieldInterface>>& fields,
             MessagePasser mp,
             const std::string script) {
              auto comm = mp.getCommunicator();
              auto recipes = RecipeGenerator::parse(script);
              for (auto& r : recipes) {
                  auto selector = r.getCellSelector();
                  CellIdFilter filter(comm, mesh, selector);
                  auto sub_mesh = filter.getMesh();
                  auto viz = getVizFactory(getPluginDir(), r.plugin);
                  auto v = viz->create(r.filename, sub_mesh, comm);
                  std::set<std::string> requested_fields(r.fields.begin(), r.fields.end());
                  for (auto f : fields) {
                      if (requested_fields.count(f->name()) == 1) {
                          auto filtered_field = filter.apply(f);
                          v->addField(filtered_field);
                      }
                  }
                  v->visualize();
              }
          });
}

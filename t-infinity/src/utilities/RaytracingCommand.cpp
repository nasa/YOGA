#include <parfait/SetTools.h>
#include <parfait/STL.h>
#include <parfait/Timing.h>
#include <parfait/Adt3dExtent.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/Extract.h>
#include <t-infinity/LineSampling.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/MeshGatherer.h>
#include <t-infinity/MeshExtents.h>
#include <parfait/VTKWriter.h>
#include "RaytracingFunctions.h"

#include <parfait/PointWriter.h>

namespace inf  {
class RaytracingCommand : public SubCommand {
  public:
    std::string description() const override { return "integrate radiative heating to surface points using ray tracing"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"--bc-names", "-b"}, "surface boundary tag names, e.g. wall1,wall2,wall3", true);
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter({"--snap", "-s"}, "solution file", true);
        m.addParameter({"--integration_point", "-p"}, "location to integrate radiation (will snap to nearest surface point)", false, "0,0,0");
        m.addHiddenFlag({"--stl"}, "Write STL of surface");
        m.addHiddenFlag({"--debug"}, "Write ray visualization file");
        return m;
    }

    template <typename Container>
    void writeSTL(MessagePasser mp, const Container& facets) const {
        Container all_facets;
        mp.Gather(facets, all_facets);
        if (mp.Rank() == 0) {
            Parfait::STL::write(all_facets, "surface");
        }
    }


    Parfait::Point<double> parseIntegrationPoint(Parfait::CommandLineMenu& m) {
        auto split_input = Parfait::StringTools::split(m.get("--integration_point"), ",");
        PARFAIT_ASSERT(split_input.size()==3, "Integration point input size /= 3, check formatting (no spaces allowed at command line).");
        Parfait::Point<double> point;
        int i=0;
        for (auto s : split_input) {
            point[i++] = Parfait::StringTools::toDouble(s);
        }
        return point;
    }


    Parfait::Facet findNearestContainingFacet(Parfait::Point<double> point, const std::vector<Parfait::Facet>& surface_facets) {
        Parfait::Facet best_facet;
        double best_distance = std::numeric_limits<double>::max();
        for (const auto& f : surface_facets) {
            auto closest_point_on_facet = f.getClosestPoint(point);
            auto distance = point.distance(closest_point_on_facet);
            if (distance < best_distance) {
                best_distance = distance;
                best_facet = f;
            }
        }
        return best_facet;
    }


    std::array<Parfait::Point<double>,3> computeFacetMetrics(Parfait::Facet facet) {
        // mesh interface guarantees that normals point out of fluid domain
        auto normal = -1.0*facet.computeNormal();
        auto t1 = facet.points[1] - facet.points[0];
        auto t2 = normal.cross(t1);
        t1.normalize();
        t2.normalize();
        return {normal, t1, t2};
    }


    double getRayMaxAABBLength(Parfait::Extent<double> AABB, Parfait::Point<double> point, Parfait::Point<double> direction) {
        double t_xmin = (AABB.lo[0] - point[0])/direction[0];
        double t_xmax = (AABB.hi[0] - point[0])/direction[0];
        double t_ymin = (AABB.lo[1] - point[1])/direction[1];
        double t_ymax = (AABB.hi[1] - point[1])/direction[1];
        double t_zmin = (AABB.lo[2] - point[2])/direction[2];
        double t_zmax = (AABB.hi[2] - point[2])/direction[2];
        std::vector<double> tvals{t_xmin, t_xmax, t_ymin, t_ymax, t_zmin, t_zmax};
        double min_positive_t_value = std::numeric_limits<double>::max();
        for ( auto t : tvals ) {
            if (t > 0 and t < min_positive_t_value) min_positive_t_value = t;
        }
        return min_positive_t_value;
    }


    double clipRayDistanceToFacets(const std::vector<Parfait::Facet>& facets,
                                   const Parfait::Point<double> origin,
                                   const Parfait::Point<double> direction,
                                   const double max_ray_distance) {
        auto ray_distance = max_ray_distance;
        for (auto f : facets) {
            double distance;
            if (f.DoesRayIntersect(origin, direction, distance)) {
                if (distance < ray_distance) ray_distance = distance;
            }
        }
        return ray_distance;
    }


    double getRayMaxIntersectionLength(std::vector<Parfait::Facet> surface_facets, Parfait::Point<double> point, Parfait::Point<double> direction) {
        double max_distance = std::numeric_limits<double>::max();
        for (auto f : surface_facets) {
            double distance;
            if (f.DoesRayIntersect(point, direction, distance)) {
                if (distance < max_distance) max_distance = distance;
            }
        }
        return max_distance;
    }


    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {

        mp_rootprint("Using %i processes.\n\n", mp.NumberOfProcesses());

        int n_polar = 5;
        int n_equatorial = 10;
        auto azimuthal_point_counts = RT::get_azimuthal_point_counts(n_equatorial, n_polar);
        auto relative_ray_orientations = RT::compute_hemisphere_ray_angles(azimuthal_point_counts);
        int ray_count = relative_ray_orientations.size();
        mp_rootprint("Using %i rays per accumulation\n\n", ray_count);

        auto begin = Parfait::Now();
        auto mesh = importMesh(m, mp);
        auto face_neighbors = inf::FaceNeighbors::build(*mesh);
        auto AABB = inf::meshExtent(mp, *mesh);
        auto partition_extent = inf::partitionExtent(*mesh);
        Parfait::Adt3DExtent adt(partition_extent);
        for (int c = 0; c < mesh->cellCount(); c++) {
            auto cell_extent = Parfait::ExtentBuilder::buildExtentForCellInMesh(*mesh, c);
            adt.store(c, cell_extent);
        }
        auto end = Parfait::Now();
        mp_rootprint("Importing mesh took %s\n\n", Parfait::readableElapsedTimeAsString(begin, end).c_str());

        // Load the snap file and retrieve relevant fields
        inf::Snap snap(mp);
        snap.addMeshTopologies(*mesh);
        snap.load(m.get("--snap"));
        auto density_field = snap.retrieve("density");
        auto temperature_field = snap.retrieve("temperature");

        std::set<std::string> tagnames = Parfait::SetTools::toSet<std::string>(getBCNames(m));
        auto surface_facets = extractOwned2DFacets(mp, *mesh, tagnames);
        surface_facets = mp.Gather(surface_facets, 0, mp.ParallelSum(surface_facets.size()));
        
        // set up surface mesh for plotting
        auto NameToTags = inf::extractNameToTags(mp, *mesh, 2);
        std::set<int> wall_tags;
        for (auto n : tagnames) {
            auto tags = NameToTags[n];
            wall_tags.insert(tags.begin(), tags.end());
        }
        auto tag_filter = inf::FilterFactory::surfaceTagFilter(mp.getCommunicator(), mesh, wall_tags);
        auto surface_mesh = tag_filter->getMesh();
        auto gathered_surface_mesh = inf::gatherMesh(*surface_mesh, mp.getCommunicator(), 0);
        
        // std::vector<double> heating;
        // for (int n = 0; n < surface_mesh->nodeCount(); n++) heating.push_back(static_cast<double>(mp.Rank()));
        // inf::VectorFieldAdapter heating_field{"heating", inf::FieldAttributes::Node(), heating};
        // inf::shortcut::visualize_surface("test", mp, surface_mesh, {std::make_shared<inf::VectorFieldAdapter>(heating_field)});

        auto debug_point = parseIntegrationPoint(m);
        auto debug_facet = findNearestContainingFacet(debug_point, surface_facets);
        auto debug_metrics = computeFacetMetrics(debug_facet);
        debug_point = debug_facet.getClosestPoint(debug_point);
        debug_point += 1e-11 * debug_metrics[0];
        std::vector<Parfait::Point<double>> end_points;
        for (int r = 0; r < ray_count; r++) {
            auto phi = relative_ray_orientations[r][0];
            auto theta = relative_ray_orientations[r][1];
            auto ray_orientation = RT::orientRay(phi, theta, debug_metrics);
            auto ray_distance = getRayMaxAABBLength(AABB, debug_point, ray_orientation);
            ray_distance = clipRayDistanceToFacets(surface_facets, debug_point, ray_orientation, ray_distance);
            auto end_point = debug_point + ray_orientation * ray_distance;
            end_points.push_back(end_point);
        }
        // output the debug rays
        std::string filename = "rays.dat";
        {
            std::ofstream rays(filename, std::ios::out);
            rays << "VARIABLES = \"X\", \"Y\", \"Z\"" << '\n';
            for (int r = 0; r < ray_count; r++) {
                rays << "ZONE I=2, DATAPACKING=POINT, ZONETYPE=ORDERED\n";
                rays << debug_point[0] << ", " << debug_point[1] << ", " << debug_point[2] << '\n';
                rays << end_points[r][0] << ", " << end_points[r][1] << ", " << end_points[r][2] << '\n';
            }
        }

        std::vector<Parfait::Point<double>> surface_points;
        if (mp.Rank() == 0) {
            for (int i = 0; i < gathered_surface_mesh->nodeCount(); i++) {
                surface_points.push_back(gathered_surface_mesh->node(i));
            }
        }
        mp.Broadcast(surface_points, 0);


        std::vector<std::vector<double>> fields{inf::FieldTools::getScalarFieldAsVector(*density_field),
                                                inf::FieldTools::getScalarFieldAsVector(*temperature_field)};
        std::vector<std::string> field_names{"density", "temperature"};
        auto get_emission = [](int i, Parfait::DataFrame line_of_sight) {
            double rho = line_of_sight["density"].at(i); // kg/m^3
            double T = line_of_sight["temperature"].at(i); // K
            double eps = 0.85;
            double sigma =  5.66961e-08; // W/m^2/K^4
            double pi = std::acos(-1.0);
            double rhom = 2940.0; // kg/m^2
            double rp = 2.0e-6; // m
            double emission = 3.0 * rho * eps * sigma * std::pow(T, 4)/(4.0 * pi * rhom * rp); // W/m^3/sr
            return emission;
        };

        std::vector<double> heating;
        int index = 1;
        auto start_time = Parfait::Now();
        for (auto point : surface_points) {
            mp_rootprint("Computing surface node %i of %i\n", index++, gathered_surface_mesh->nodeCount());

            // snap point to nearest facet and get metrics
            auto surface_face = findNearestContainingFacet(point, surface_facets);
            auto integration_point = surface_face.getClosestPoint(point);
            mp_rootprint("Actual integration point: [%f, %f, %f]\n", integration_point[0], integration_point[1], integration_point[2]);
            auto face_metrics = computeFacetMetrics(surface_face);
            auto normal = face_metrics[0];
            // mp_rootprint("Surface normal: %f, %f, %f\n", normal[0], normal[1], normal[2]);
            
            // Integrate each ray
            std::vector<double> wall_intensity_values;
            // std::vector<Parfait::DataFrame> ray_set;
            for (int r = 0; r < ray_count; r++) {
                RT::Ray ray;
                auto phi = relative_ray_orientations[r][0];
                auto theta = relative_ray_orientations[r][1];
                auto ray_orientation = RT::orientRay(phi, theta, face_metrics);
                ray.origin = point + 1.0e-12*normal;
                ray.direction = ray_orientation;
                // mp_rootprint("Orientation: %f, %f, %f\n", ray.direction[0], ray.direction[1], ray.direction[2]);
                auto ray_distance = getRayMaxAABBLength(AABB, ray.origin, ray.direction);
                // mp_rootprint("Ray length: %f\n", ray_distance);
                ray_distance = clipRayDistanceToFacets(surface_facets, ray.origin, ray.direction, ray_distance);
                // mp_rootprint("Ray length: %f\n", ray_distance);
                ray.length = ray_distance;
                
                // auto line = inf::linesampling::sample(mp, *mesh, face_neighbors, ray.origin, ray.origin + ray.direction * ray.length, fields, field_names);
                // mp_rootprint("nsamples: %i\n", line.shape()[0]);
                // ray_set.push_back(line);

                auto wall_intensity = RT::get_wall_intensity(mp, ray, *mesh, face_neighbors, adt, fields, field_names, get_emission);
                wall_intensity_values.push_back(wall_intensity);
                // mp_rootprint("Ray angle: [%f, %f] - Iw = %f\n", phi, theta, wall_intensity);
            }
            // if (mp.Rank() == 0 and m.has("--debug")) {
            //     RT::debug_visualize_ray_set(ray_set);
            // }

            // Integrate the intensity
            auto wall_heat_flux = RT::integrate_flux(azimuthal_point_counts, wall_intensity_values);
            heating.push_back(wall_heat_flux);
            mp_rootprint("Wall heat flux: %f W/m^2\n", wall_heat_flux);

            auto current_time = Parfait::Now();
            auto elapsed = Parfait::elapsedTimeInSeconds(start_time, current_time);
            auto avrg_time = elapsed / static_cast<double>(index);
            auto remaining_time = static_cast<double>(surface_points.size() - index) * avrg_time;
            mp_rootprint("Averaging %f seconds per surface point. ~%f seconds remaining\n\n", avrg_time, remaining_time);
        }

        // visualize surface solution
        if (mp.Rank() == 0) {
            auto get_point = [&](int node_id, double* p) { gathered_surface_mesh->nodeCoordinate(node_id, p); };
            auto get_cell_type = [&](long cell_id) { return Parfait::vtk::Writer::CellType::TET; };
            auto get_cell_nodes = [&](int cell_id, int* cell_nodes) {
                auto cell = gathered_surface_mesh->getCell(cell_id);
                for (size_t i = 0; i < cell.nodes.size(); i++) {
                    cell_nodes[i] = cell.nodes[i];
                }
            };
            int npoints = gathered_surface_mesh->nodeCount();
            int ncells = gathered_surface_mesh->cellCount();
            Parfait::vtk::Writer writer("qw_rad_field", npoints, ncells,
                                        get_point,
                                        get_cell_type,
                                        get_cell_nodes);
            auto getField = [&](int node_id) {
                return heating.at(node_id);
            };
            writer.addNodeField("qw_rad", getField);
            writer.visualize();
        }

        mp.Finalize();
    }

};
}

CREATE_INF_SUBCOMMAND(inf::RaytracingCommand)
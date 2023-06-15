#include <parfait/Point.h>
#include <parfait/PointWriter.h>
#include <parfait/DataFrame.h>
#include <t-infinity/MeshInterface.h>
// #include <t-infinity/LineSampling.h>

namespace RT {

    struct Ray {
        Parfait::Point<double> origin{0.0, 0.0, 0.0};
        Parfait::Point<double> direction{1.0, 0.0, 0.0};
        double length = std::numeric_limits<double>::max();
    };


    double integrate_flux(std::vector<int> azimuthal_point_counts, std::vector<double> Iw) {
        double qw = 0.0;
        std::vector<double> azimuthal_integrals;
        int nphi = azimuthal_point_counts.size();
        auto pi = std::acos(-1.0);
        int index = 0;
        azimuthal_integrals.push_back(Iw[index++] * 2.0 * pi);
        for (int p = 1; p < nphi; p++) {
            double integral = 0.0;
            auto ntheta = azimuthal_point_counts[p];
            auto dtheta = 2.0 * pi / static_cast<double>(ntheta - 1);
            for (int t = 0; t < ntheta - 1; t++) {
                integral += 2.0*Iw[index++];
            }
            azimuthal_integrals.push_back(0.5*dtheta*integral);
        }
        auto dphi = (pi / 2.0) / static_cast<double>(nphi - 1);
        for (int p = 0; p < nphi - 1; p++) {
            auto phi1 = (pi / 2.0) * p / static_cast<double>(nphi - 1);
            auto phi2 = (pi / 2.0) * (p + 1) / static_cast<double>(nphi - 1);
            qw += 0.5 * dphi * (std::sin(phi1) * std::cos(phi1) * azimuthal_integrals[p] + std::sin(phi2) * std::cos(phi2) * azimuthal_integrals[p+1]);
        }
        return qw;
    }

    void sortLineSampleByArcLength(Parfait::Point<double> origin, Parfait::DataFrame& line_sample) {
        std::vector<double> arc_length;
        for (int i=0; i<line_sample.shape()[0]; i++) {
            double x = line_sample["x"].at(i);
            double y = line_sample["y"].at(i);
            double z = line_sample["z"].at(i);
            arc_length.push_back(origin.distance(Parfait::Point<double>{x,y,z}));
        }
        line_sample["s"] = arc_length;
        line_sample.sort("s");
    }


    double integrateAlongRay(Parfait::DataFrame line_of_sight,
                             Parfait::Point<double> origin,
                             std::function<double(int,Parfait::Point<double>,Parfait::DataFrame)> get_arc_length,
                             std::function<double(int,Parfait::DataFrame)> get_intensity) {
        int npoints = line_of_sight.shape()[0];
        // std::cout << "npoints: " << npoints << '\n';
        double integral = 0.0;
        for (int i=0; i<npoints-1; i++) {
            auto si  = get_arc_length(i,   origin, line_of_sight);
            // std::cout << si << '\n';
            auto sip = get_arc_length(i+1, origin, line_of_sight);
            auto fi  = get_intensity(i,   line_of_sight);
            auto fip = get_intensity(i+1, line_of_sight);
            auto dx = sip - si;
            integral += (dx/2.0)*(fi + fip);
        }
        return integral;
    }


    double get_wall_intensity(MessagePasser mp, const RT::Ray ray,
                              const inf::MeshInterface& mesh,
                              const std::vector<std::vector<int>>& face_neighbors,
                              const Parfait::Adt3DExtent& adt,
                              const std::vector<std::vector<double>>& fields,
                              const std::vector<std::string>& field_names,
                              const std::function<double(int,Parfait::DataFrame)>& intensity_function) {
        auto a = ray.origin;
        auto b = ray.origin + ray.direction * ray.length;

        // linesampling::sample outputs a dataframe which is only valid on Rank 0.
        auto line_sample = inf::linesampling::sample(mp, mesh, face_neighbors, adt, a, b, fields, field_names);
        double wall_intensity = 0.0;
        if (mp.Rank() == 0) {
            sortLineSampleByArcLength(a, line_sample);
            
            auto get_arc_length = [](int i, Parfait::Point<double> origin, Parfait::DataFrame line_of_sight) {
                auto x = line_of_sight["x"].at(i);
                auto y = line_of_sight["y"].at(i);
                auto z = line_of_sight["z"].at(i);
                Parfait::Point<double> point{x, y, z};
                return origin.distance(point);
            };

            wall_intensity = integrateAlongRay(line_sample, a, get_arc_length, intensity_function);
        }
        mp.Broadcast(wall_intensity, 0);
        return wall_intensity;
    }


    std::vector<int> get_azimuthal_point_counts(int equatorial_point_count, int polar_point_count) {
        // Johnston, C. and Mazaheri, A., "Impact of Non-Tangent Slab Radiative Transport on Flowfield-Radiation Coupling"
        // Radiative heat flux is computed by integrating the intensity at a point over the sphere of visibility
        //  q_rad = \int_0^{2\pi} \int_0^{\pi/2} I(\phi, \theta) cos(\phi) sin(\phi) d\phi dtheta
        // where I(\phi, \theta) is the intensity at the origin of ray pointing in the \phi, \theta direction
        // \phi is the polar angle (angle away from surface normal, for example), and \theta is the azimuthal angle
        // around the pole. The integral is discretized because it can be solved in two phases:
        // First, the azimuthal angle is integrated at each polar angle, and then the final integral is the
        // integral of the integrals of intensity at each polar angle over the full polar angle range.
        // The following angle discretization is designed for this process.
        
        // std::vector<std::array<double,2>> relative_ray_orientations;
        std::vector<int> azimuthal_point_counts;
        double pi = acos(-1.0);
        for (int p=0; p<polar_point_count; p++) {
            double max_polar_angle = pi / 2.0;
            double phi = max_polar_angle * p / static_cast<double>(polar_point_count - 1);
            int ntheta = std::ceil(equatorial_point_count * std::sin(phi));
            if (ntheta < 3) ntheta = 3; // set minimum point count
            if (p == 0) ntheta = 1;
            azimuthal_point_counts.push_back(ntheta);
        }
        return azimuthal_point_counts;
    }


    std::vector<std::array<double,2>> compute_hemisphere_ray_angles(std::vector<int> azimuthal_point_counts) {
        std::vector<std::array<double,2>> relative_ray_orientations;
        int nphi = azimuthal_point_counts.size();
        double pi = acos(-1.0);
        relative_ray_orientations.push_back({0.0, 0.0});
        for (int p = 1; p < nphi; p++) {
            auto max_polar_angle = 0.99 * (pi / 2.0);
            auto phi = max_polar_angle * p / static_cast<double>(nphi - 1);
            auto ntheta = azimuthal_point_counts[p];
            for (int t = 0; t < ntheta - 1; t++) {
                double theta = 2.0 * pi * t / static_cast<double>(ntheta - 1);
                if (ntheta == 1) theta = 0.0;
                std::array<double,2> orientation{phi,theta};
                relative_ray_orientations.push_back(orientation);
            }
        }
        return relative_ray_orientations;
    }


    Parfait::Point<double> orientRay(double phi, double theta, std::array<Parfait::Point<double>,3> face_metrics) {
        auto Rn = std::cos(phi);
        auto Re = std::sin(phi)*std::cos(theta);
        auto Rf = std::sin(phi)*std::sin(theta);
        auto orientation = Rn*face_metrics[0] + Re*face_metrics[1] + Rf*face_metrics[2];
        orientation.normalize();
        return orientation;
    }


    // Parfait::DataFrame removeDuplicateSamplePoints(Parfait::DataFrame& line_sample) {
    //     Parfait::DataFrame cleaned_line_sample;
    //     auto column_names = line_sample.columns();
    //     std::vector<std::vector<double>> old_columns;
    //     std::vector<std::vector<double>> new_columns;
    //     for (auto n : column_names) {
    //         std::cout << "column: " << n << '\n';
    //         old_columns.push_back(line_sample[n]);
    //         new_columns.push_back({});
    //     }
    //     auto npoints = line_sample.shape()[0];
    //     for (int i=0; i<npoints; i++) {
    //         auto si = 
    //     }
    //     return line_sample; // fix this
    // }


    void visualizeRay(Parfait::DataFrame& line_sample) {
        std::vector<Parfait::Point<double>> sample_points;
        for (int i=0; i<line_sample.shape()[0]; i++) {
            auto x = line_sample["x"].at(i);
            auto y = line_sample["y"].at(i);
            auto z = line_sample["z"].at(i);
            sample_points.push_back({x,y,z});
        }
        Parfait::PointWriter::write("sample_points",sample_points);
    }


    void debug_visualize_ray_set(std::vector<Parfait::DataFrame> rays) {
        std::vector<Parfait::Point<double>> sample_points;
        for (auto r : rays) {
            for (int i = 0; i < r.shape()[0]; i++) {
                auto x = r["x"].at(i);
                auto y = r["y"].at(i);
                auto z = r["z"].at(i);
                sample_points.push_back({x,y,z});
            }
        }
        Parfait::PointWriter::write("sample_points",sample_points);
    }


}
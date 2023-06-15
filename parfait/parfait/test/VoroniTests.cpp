#include <RingAssertions.h>
#include <parfait/PolygonClipping.h>
#include <parfait/Plane.h>
#include <parfait/STL.h>

namespace Parfait {
namespace voroni {

    class Cell {
      public:
        inline Cell(const Point<double>& center, double initial_radius = 1e6) : centroid(center) {
            Point<double> offset(initial_radius, initial_radius, initial_radius);
            auto e = Extent<double>(center - offset, center + offset);
            faces = Polygons::extentAsPolygons(e);
        }
        Point<double> centroid;
        std::vector<Polygons::Polygon> faces;

        Extent<double> findCurrentExtent() const {
            auto e = ExtentBuilder::createEmptyBuildableExtent<double>();
            for (auto& f : faces) {
                for (auto& vertex : f) {
                    ExtentBuilder::add(e, vertex);
                }
            }
            return e;
        }

        void clipPlane(const Plane<double>& p) {
            // make a large polygon that represents the cell.
            // clip the polygon against each polygon in the voroni cell.
            // add whatever remains of the plane polygon to the voroni cell.
            //            auto plane_as_large_quad = createPolygonForPlane(p);
            faces = Polygons::planeClipping(p, faces);
        }

        int faceCount() const { return int(faces.size()); }
    };
}
}

TEST_CASE("Create a large quad representing a plane") { Parfait::Plane<double> p({1, 0, 0}, {0.5, 0.0, 0.0}); }

TEST_CASE("voroni clipping") {
    Parfait::voroni::Cell v({0, 0, 0});
    Parfait::Plane<double> p({1, 0, 0}, {0.5, 0.0, 0.0});
    REQUIRE(v.faceCount() == 6);
    v.clipPlane(p);

    // This is where I'm trying to get to.  But we need to clip the plane against the existing polyhedral cell and add
    // that face to the cell. Right now we just clip the polyhedral cell against the plane, creating a whole in the cell
    // where the face should be.
    // auto tris = Polygons::triangulate(v.faces);
    // STL::write(tris, "voroni");
    // REQUIRE(v.faceCount() == 6);
}

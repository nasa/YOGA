#pragma once
#include <MessagePasser/MessagePasser.h>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <parfait/ExtentBuilder.h>

namespace YOGA{
    class Tree{
      public:
        Tree(int n):items(n){}

        class Item{
          public:
            std::array<Parfait::Point<double>,3> vertices;
            Parfait::Point<double> closestPoint(const Parfait::Point<double>& p) const {
                return vertices.front();
            }
            Parfait::Extent<double>  extent(){
                Parfait::Extent<double> e {vertices.front(),vertices.front()};
                for(auto&p:vertices)
                    Parfait::ExtentBuilder::add(e,p);
                return e;
            }
        };

      private:
        std::vector<Item> items;
    };

    Tree::Item extractFacet(const inf::MeshInterface& mesh,int cell_id){
        Tree::Item facet;
        int n = mesh.cellLength(cell_id);
        std::vector<int> cell(n);
        mesh.cell(cell_id,cell.data());
        for(int i=0;i<n;i++)
            mesh.nodeCoordinate(cell[i],facet.vertices[i].data());
        return facet;
    }

    std::vector<Tree::Item> extractFacets(const inf::MeshInterface& mesh, const std::vector<int>& tags){
        std::set<int> wall_tags(tags.begin(),tags.end());
        std::vector<Tree::Item> facets;
        for(int i=0;i<mesh.cellCount();i++)
            if(inf::MeshInterface::TRI_3 == mesh.cellType(i) and wall_tags.count(mesh.cellTag(i)))
                facets.emplace_back(extractFacet(mesh,i));
        return facets;
    }

    class QueryPoint{
    public:
        int local_id;
        Parfait::Point<double> p;
    };

    std::vector<double> bruteForceSerialDistance(const std::vector<QueryPoint>& points,
                                                 const std::vector<Tree::Item>& facets){
        std::vector<double> distance(points.size(),std::numeric_limits<double>::max());
        for(size_t i=0;i<points.size();i++){
            auto& p = points[i].p;
            for(auto& facet:facets){
                double d = (p - facet.closestPoint(p)).magnitude();
                distance[i] = std::min(d,distance[i]);
            }
        }
        return distance;
    }

    std::vector<double> calcDistanceToTags(MessagePasser mp,const inf::MeshInterface& mesh,
                                       const std::vector<int> wall_tags){
        std::vector<double> distance(mesh.nodeCount(),0.0);
        auto facets = extractFacets(mesh,wall_tags);
        printf("Extracted %li surface facets on rank %i\n",facets.size(),mp.Rank());

        auto surface_partition_extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(auto& facet:facets)
            Parfait::ExtentBuilder::add(surface_partition_extent,facet.extent());
        auto surface_partition_extents = mp.Gather(surface_partition_extent);

        std::vector<QueryPoint> points(mesh.nodeCount());
        for(int i=0;i<mesh.nodeCount();i++){
            mesh.nodeCoordinate(i,points[i].p.data());
        }

        printf("Brute force distance...\n");
        distance = bruteForceSerialDistance(points,facets);
        printf("done.\n");

        return distance;
    }
}

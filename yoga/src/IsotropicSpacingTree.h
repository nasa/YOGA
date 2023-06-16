#pragma once
#include <parfait/Point.h>
#include <parfait/Extent.h>
#include <parfait/ExtentWriter.h>
namespace YOGA{

class Voxel{
  public:

    Voxel(const Parfait::Extent<double>& e):e(e){}
    Parfait::Extent<double> octant(int id) const {
        auto oct = e;
        double dx = 0.5*e.getLength_X();
        double dy = 0.5*e.getLength_Y();
        double dz = 0.5*e.getLength_Z();
        if(onLeft(id)) oct.hi[0] -= dx;
        else oct.lo[0] += dx;
        if(onFront(id)) oct.hi[1] -= dy;
        else oct.lo[1] += dy;
        if(onBottom(id)) oct.hi[2] -= dz;
        else oct.lo[2] += dz;
        return oct;
    }
    double spacing() const {return e.getLength_X();}

    Parfait::Extent<double> e;

  private:
    bool onLeft(int id) const {return id == 0 or id == 3 or id == 4 or id == 7;}
    bool onFront(int id) const {return id < 2 or id == 4 or id == 5;}
    bool onBottom(int id) const {return id < 4;}
};

class IsotropicSpacingTree {
  public:
    class Spacing{
      public:
        Spacing() = default;
        Spacing(const Parfait::Point<double>& p, double h):p(p),h(h){}
        Parfait::Point<double> p;
        double h;
    };

    IsotropicSpacingTree(const Parfait::Extent<double>& domain):domain(domain){
        nodes.emplace_back(domain);
    }
    void setMinSpacing(double h){min_h = h;}
    void insert(const Parfait::Extent<double>& item_extent,double spacing){
        int depth = 0;
        insert(0,item_extent,spacing,depth);
    }
    double getSpacingAt(const Parfait::Point<double>& p) const {
        std::vector<int> containing_voxels;
        findContainingVoxels(0,p, containing_voxels);
        double min_spacing = nodes.front().spacing();
        for(auto id:containing_voxels)
            min_spacing = std::min(min_spacing, nodes[id].spacing());
        return min_spacing;
    }

    std::vector<Spacing> spacingValues(){
        std::vector<Spacing> spacing;
        for(auto&node:nodes){
            int n_kids = 0;
            for(int i=0;i<8;i++)
                if(node.hasChild(i))
                    n_kids++;
            if(n_kids < 8)
                spacing.emplace_back(node.vox.e.center(),node.vox.e.getLength_X());
        }
        return spacing;
    }

    void visualize(const std::string filename){
        std::vector<Extent> extents;
        for(auto&node:nodes){
            int n_kids = 0;
            for(int i=0;i<8;i++)
                if(node.hasChild(i))
                    n_kids++;
            if(n_kids < 8)
                extents.push_back(node.vox.e);
        }

        Parfait::ExtentWriter::write(filename,extents);
    }

  private:
    using Extent = Parfait::Extent<double>;

    class Node{
      public:
        explicit Node(Extent e):vox(e){}
        double spacing() const {return vox.spacing();}
        bool hasChild(int i) const {return child_ids[i] != CHILD_NOT_SET;}
        int childId(int i) const {return child_ids[i];}
        void setChildId(int i, int id){child_ids[i] = id;}
        Extent childExtent(int i) const {
            return vox.octant(i);
        }

        Voxel vox;

      private:
        static constexpr int CHILD_NOT_SET = -1;
        std::array<int,8> child_ids = {CHILD_NOT_SET,CHILD_NOT_SET,CHILD_NOT_SET,CHILD_NOT_SET,
                                       CHILD_NOT_SET,CHILD_NOT_SET,CHILD_NOT_SET,CHILD_NOT_SET};
    };

    int max_depth = 10;
    Extent domain;
    std::vector<Node> nodes;
    double min_h = 0.0;

    void findContainingVoxels(int current_voxel, const Parfait::Point<double>& p,std::vector<int>& containing) const {
        auto& node = nodes[current_voxel];
        if(node.vox.e.intersects(p))
            containing.push_back(current_voxel);
        for(int i=0;i<8;i++)
            if(node.hasChild(i) and node.vox.octant(i).intersects(p))
                findContainingVoxels(node.childId(i), p, containing);
    }

    void insert(int current_voxel,const Extent& e,double s,int depth){
        if(noRefinementNeeded(e, s, nodes[current_voxel].vox,depth))
            return;
        for(int i=0;i<8;i++){
            auto child_e = nodes[current_voxel].vox.octant(i);
            if(e.intersects(child_e)){
                if(not nodes[current_voxel].hasChild(i))
                    addChild(nodes[current_voxel],i);
                insert(nodes[current_voxel].childId(i), e, s, depth++);
            }
        }
    }

    void addChild(Node& node, int child_index){
        int new_child_id = nodes.size();
        node.setChildId(child_index, new_child_id);
        auto child_extent = node.vox.octant(child_index);
        nodes.emplace_back(Node(child_extent));
    }

    bool noRefinementNeeded(const Extent& e, double s, Voxel& voxel, int depth) const {
        return not e.intersects(voxel.e) or voxel.spacing() <= s or depth >= max_depth or voxel.spacing() <= min_h;
    }
};
}
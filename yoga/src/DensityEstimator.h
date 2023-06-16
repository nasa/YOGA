#pragma once
#include <Tracer.h>
#include <parfait/Adt3dExtent.h>
#include <parfait/CartBlock.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/ExtentWriter.h>
#include <parfait/RecursiveBisection.h>
#include <parfait/CartBlockVisualize.h>

namespace YOGA{

    class DensityEstimator{
    public:
        DensityEstimator(const Parfait::CartBlock& b,int npart,const std::vector<double>& nodes_per_cell)
                :adt(b)
        {
            auto cell_ids = identifyNonEmptyPixels(b, nodes_per_cell);
            auto pixels = extractPixels(b,cell_ids);
            auto weights = extractWeights(b,cell_ids,nodes_per_cell);
            double total = std::accumulate(weights.begin(),weights.end(),0);

            printf("Image cells: %i non-empty-pixels: %li\n",b.numberOfCells(),pixels.size());

            double target = total / double(npart);
            double max_w = 0.0;
            for(auto& w:weights) max_w = std::max(max_w,w);
            agglomeratePixelsViaRCB(b, npart, nodes_per_cell, pixels, weights, cell_ids, total);
            subdivideOverfullPixels(target);

            Tracer::begin("image-adt");
            adt.reserve(npart);
            for(int i=0;i<long(extents.size());i++){
                adt.store(i,extents[i]);
            }
            Tracer::end("image-adt");
        }

        void subdivideOverfullPixels(double target_density){
            auto extents_copy = extents;
            auto counts_copy = counts;
            extents.clear();
            counts.clear();
            for(int i=0;i<long(extents_copy.size());i++){
                int count = counts_copy[i];
                auto& e = extents_copy[i];
                if(count > target_density){
                    int n = count / target_density;
                    int fraction = count/ n;
                    auto dimensions = calcRefineDimensions(n,e);
                    Parfait::CartBlock b(e,dimensions[0],dimensions[1],dimensions[2]);
                    for(int cell_id = 0;cell_id<b.numberOfCells();cell_id++){
                        extents.push_back(b.createExtentFromCell(cell_id));
                        counts.push_back(fraction);
                    }
                }
                else{
                    extents.push_back(e);
                    counts.push_back(count);
                }
            }
        }


        void agglomeratePixelsViaRCB(const Parfait::CartBlock& b,
                                     int npart,
                                     const std::vector<double>& nodes_per_cell,
                                     const std::vector<Parfait::Point<double>>& pixels,
                                     std::vector<double>& weights,
                                     const std::vector<int>& cell_ids,
                                     double total) {
            printf("normalize\n"); fflush(stdout);
            Parfait::normalizeWeights(weights);
            Tracer::begin("image-rcb");
            printf("parfait-rcb\n"); fflush(stdout);
            auto part = Parfait::recursiveBisection(pixels, weights, npart, .01);
            //auto part = Parfait::recursiveBisection(pixels, npart, .01);
            printf("parfait-rcb-end\n");fflush(stdout);
            Tracer::end("image-rcb");
            int max_part = 0;
            for(int p:part){
                max_part = std::max(max_part,p);
            }
            int actual_npart = max_part+1;
            extents.resize(actual_npart, Parfait::ExtentBuilder::createEmptyBuildableExtent<double>());
            counts.resize(actual_npart, 0);
            std::vector<bool> touched(b.numberOfCells(),false);
            printf("agglomerating %li pixels into %i parts\n",pixels.size(),actual_npart);
            for (size_t i = 0; i < pixels.size(); i++) {
                int cell_id = cell_ids.at(i);
                int n = nodes_per_cell.at(cell_id);
                //if (n > 0) {
                    int extent_id = part[i];
                    counts.at(extent_id) += n;
                    touched[extent_id] = true;
                    Parfait::ExtentBuilder::add(extents[extent_id], b.createExtentFromCell(cell_id));
                //}
            }

            //std::vector<double> density(counts.begin(),counts.end());
            //Parfait::ExtentWriter::write("density-estimator",extents,density);
            //std::vector<double> stuff(nodes_per_cell.begin(),nodes_per_cell.end());
            //Parfait::visualize(b,stuff,"mesh-image");
            //printf("done...\n");

            //std::vector<double> part_id(part.begin(),part.end());
            //Parfait::PointWriter::write("points",pixels,part_id);
        }

        void estimateContainedNodeCounts(const Parfait::CartBlock& block,std::vector<int>& bins){
            auto ids = adt.retrieve(block);
            sums.resize(bins.size());
            std::fill(sums.begin(),sums.end(),0);
            for(int pixel_id:ids){
                auto& e_pixel = extents[pixel_id];
                auto range = block.getRangeOfOverlappingCells(e_pixel);
                for(int i=range.lo[0];i<range.hi[0];i++){
                    for(int j=range.lo[1];j<range.hi[1];j++){
                        for(int k=range.lo[2];k<range.hi[2];k++){
                            int bin_id = block.convert_ijk_ToCellId(i,j,k);
                            auto e_bin = block.createExtentFromCell(bin_id);
                            auto intersection = e_pixel.intersection(e_bin);
                            double factor = intersection.volume(intersection) / e_pixel.volume(e_pixel);
                            sums[bin_id] += factor * counts[pixel_id];
                        }
                    }
                }
            }
            std::transform(sums.begin(),sums.end(),bins.begin(),[](double x){return std::round(x);});
        }

        long estimateContainedNodeCount(const Parfait::Extent<double>& e){
            auto ids = adt.retrieve(e);
            double sum = 0;
            for(int id:ids){
                auto& e2 = extents[id];
                auto intersection = e.intersection(e2);
                double factor = intersection.volume(intersection) / e2.volume(e2);
                sum += factor*counts[id];
            }
            return long(std::round(sum));
        }
        std::vector<Parfait::Extent<double>> extents;
        std::vector<int> counts;
    private:
        Parfait::Adt3DExtent adt;
        std::vector<double> sums;

        std::vector<int> identifyNonEmptyPixels(const Parfait::CartBlock& image, const std::vector<double>& nodes_per_cell){
            std::vector<int> ids;
            for(int cell_id=0;cell_id<image.numberOfCells();cell_id++){
                if(nodes_per_cell[cell_id] > 0) ids.push_back(cell_id);
            }
            return ids;
        }

        std::vector<Parfait::Point<double>> extractPixels(const Parfait::CartBlock& image,
                                                          const std::vector<int>& ids){
            std::vector<Parfait::Point<double>> pixels;
            for(int id:ids){
                Parfait::Point<double> c;
                image.getCellCentroid(id,c.data());
                pixels.push_back(c);
            }
            return pixels;
        }

        std::vector<double> extractWeights(const Parfait::CartBlock& image,
                    const std::vector<int>& ids,
                    const std::vector<double>& nodes_per_cell){
            std::vector<double> weights;
            for(int id:ids) weights.push_back(nodes_per_cell[id]);
            return weights;
        }

        std::array<int, 3> calcRefineDimensions(int targetChunkCount,
            const Parfait::Extent<double>& voxel) {
            int n = targetChunkCount;
            if (n < 8) {
                double dx = voxel.getLength_X();
                double dy = voxel.getLength_Y();
                double dz = voxel.getLength_Z();
                if (dx > dy and dx > dz) return {n, 1, 1};
                if (dy > dx and dy > dz) return {1, n, 1};
                return {1, 1, n};
            } else if (n < 27) {
                return {2, 2, 2};
            } else if (n < 64) {
                return {3, 3, 3};
            } else if (n < 125) {
                return {4, 4, 4};
            } else {
                return {5, 5, 5};
            }
        }

    };
}

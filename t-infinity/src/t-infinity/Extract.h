#pragma once
#include <vector>
#include <set>
#include <parfait/Facet.h>
#include <MessagePasser/MessagePasser.h>
#include "MeshInterface.h"

namespace inf {

std::vector<int> extractTags(const inf::MeshInterface& mesh);
std::set<int> extractTagsWithDimensionality(const inf::MeshInterface& mesh, int dimensionality);
std::set<int> extract2DTags(const inf::MeshInterface& mesh);
std::set<int> extractAll2DTags(MessagePasser mp, const inf::MeshInterface& mesh);
std::set<int> extractAllTagsWithDimensionality(MessagePasser mp,
                                               const inf::MeshInterface& mesh,
                                               int dimensionality);
std::map<int, std::string> extract2DTagsToNames(MessagePasser mp, const inf::MeshInterface& mesh);
std::map<int, std::string> extract1DTagsToNames(MessagePasser mp, const inf::MeshInterface& mesh);
std::map<int, std::string> extractTagsToNames(MessagePasser mp,
                                              const inf::MeshInterface& mesh,
                                              int dimensionality);
std::map<std::string, std::set<int>> extractNameToTags(MessagePasser mp,
                                                       const inf::MeshInterface& mesh,
                                                       int dimensionality);
std::set<int> extractTagsByName(MessagePasser mp,
                                const inf::MeshInterface& mesh,
                                std::string tag_name);
std::vector<int> extractCellIdsOnTag(const inf::MeshInterface& mesh, int tag, int dimensionality);
std::vector<int> extractOwnedCellIdsOnTag(const inf::MeshInterface& mesh,
                                          int tag,
                                          int dimensionality);
std::vector<int> extractOwnedCellIdsInTagSet(const inf::MeshInterface& mesh,
                                             const std::set<int>& tag_set,
                                             int dimensionality);

std::vector<double> extractOwnedDataAtNodes(const std::vector<double>& node_data,
                                            const MeshInterface& mesh);
std::vector<double> extractOwnedDataAtCells(const std::vector<double>& cell_data,
                                            const MeshInterface& mesh);

std::vector<Parfait::Facet> extractOwned2DFacets(MessagePasser mp, const inf::MeshInterface& mesh);
std::vector<Parfait::Facet> extractOwned2DFacets(MessagePasser mp,
                                                 const inf::MeshInterface& mesh,
                                                 const std::set<int>& tags);
std::vector<Parfait::Facet> extractOwned2DFacets(MessagePasser mp,
                                                 const inf::MeshInterface& mesh,
                                                 const std::set<std::string>& names);
std::tuple<std::vector<Parfait::Facet>, std::vector<int>> extractOwned2DFacetsAndTheirTags(
    MessagePasser mp, const inf::MeshInterface& mesh, const std::set<int>& tags);

std::vector<Parfait::Point<double>> extractPoints(const inf::MeshInterface& mesh);
std::vector<Parfait::Point<double>> extractPoints(const inf::MeshInterface& mesh,
                                                  const std::vector<int>& node_ids);
std::vector<Parfait::Point<double>> extractOwnedPoints(const inf::MeshInterface& mesh);

std::vector<int> extractAllNodeIdsIn2DCells(const inf::MeshInterface& mesh);
std::vector<int> extractAllNodeIdsInCellsWithDimensionality(const inf::MeshInterface& mesh,
                                                            int dimensionality);
std::vector<int> extractNodeIdsOn2DTags(const inf::MeshInterface& mesh, int tag);
std::vector<int> extractNodeIdsViaDimensionalityAndTag(const inf::MeshInterface& mesh,
                                                       int tag,
                                                       int dimensionality);
std::vector<int> extractOwnedNodeIdsOn2DTags(const inf::MeshInterface& mesh, int tag);
std::vector<int> extractOwnedNodeIdsViaDimensionalityAndTag(const inf::MeshInterface& mesh,
                                                            int tag,
                                                            int dimensionality);

std::vector<bool> extractDoOwnCells(const inf::MeshInterface& mesh);
std::vector<Parfait::Point<double>> extractCellCentroids(const inf::MeshInterface& mesh);

std::vector<long> extractOwnedGlobalNodeIds(const inf::MeshInterface& mesh);
std::vector<long> extractGlobalNodeIds(const inf::MeshInterface& mesh);

std::vector<long> extractOwnedGlobalCellIds(const inf::MeshInterface& mesh);
std::vector<long> extractGlobalCellIds(const inf::MeshInterface& mesh);

std::vector<int> extractCellOwners(const inf::MeshInterface& mesh);
std::vector<int> extractNodeOwners(const inf::MeshInterface& mesh);
std::set<int> extractResidentButNotOwnedNodesByDistance(const MeshInterface& mesh,
                                                        int distance,
                                                        const std::vector<std::vector<int>>& n2n);

Parfait::Point<double> extractAverageNormal(const MessagePasser& mp,
                                            const MeshInterface& mesh,
                                            const std::set<int>& tags);
std::vector<Parfait::Point<double>> extractSurfaceArea(const MeshInterface& mesh,
                                                       int surface_cell_dimensionality = 2);
std::vector<Parfait::Point<double>> calcSurfaceAreaAtNodes(const MeshInterface& mesh,
                                                           const std::set<int>& cell_ids);

}

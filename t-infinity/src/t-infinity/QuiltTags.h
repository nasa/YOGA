#include "MeshInterface.h"
#include "TinfMesh.h"
#include "Extract.h"
#include <parfait/ToString.h>

namespace inf {

void relabelSurfaceTagsByNearestNormal(TinfMesh& mesh,
                                       const std::map<int, Parfait::Point<double>>& tag_to_normal,
                                       const std::set<int>& tags_to_overwrite);
std::map<int, Parfait::Point<double>> getCartesianNormalTags(int new_tag_start = 0);

std::map<int, int> quiltTagsDontCompact(const std::vector<std::vector<int>>& tags_to_merge,
                                        const std::set<int>& current_tags);
std::map<int, int> quiltTagsAndCompact(const std::vector<std::vector<int>>& tags_to_merge,
                                       const std::set<int>& current_tags);

void remapSurfaceTags(TinfMesh& m, const std::map<int, int>& old_to_new_tags);

void compactTags(MessagePasser mp, TinfMesh& m);

std::map<int, int> quiltTagsAndCompact(MessagePasser mp,
                                       TinfMesh& m,
                                       std::vector<std::vector<int>> tags_to_merge);
std::map<int, int> quiltTagsAndDontCompact(MessagePasser mp,
                                           TinfMesh& m,
                                           std::vector<std::vector<int>> tags_to_merge);

}
#include "VizFromDictionary.h"
#include "MeshHelpers.h"
#include "IsoSampling.h"
#include "FieldTools.h"
#include "Extract.h"
#include "Shortcuts.h"

namespace inf {
void convertToNodes(MessagePasser mp,
                    const MeshInterface& mesh,
                    std::vector<std::shared_ptr<FieldInterface>>& fields) {
    if (inf::is2D(mp, mesh)) {
        for (auto& field : fields)
            if (field->association() == FieldAttributes::Cell())
                field = FieldTools::cellToNode_simpleAverage(mesh, *field);
    } else {
        for (auto& field : fields)
            if (field->association() == FieldAttributes::Cell())
                field = FieldTools::cellToNode_volume(mesh, *field);
    }
}

void visualizeCrinkleSubvolume(MessagePasser mp,
                               std::shared_ptr<MeshInterface> mesh,
                               const std::vector<std::shared_ptr<FieldInterface>>& fields,
                               const Parfait::Dictionary& item) {
    auto type = item.at("type").asString();  // options: [sphere, x-slice, y-slice, z-slice]
    std::shared_ptr<CellSelector> selector = nullptr;
    if (type == "sphere") {
        auto center = item.at("center").asDoubles();
        auto radius = item.at("radius").asDouble();
        selector = std::make_shared<SphereSelector>(
            radius, Parfait::Point<double>{center[0], center[1], center[2]});
    } else if (type == "plane") {
        auto center = item.at("center").asDoubles();
        auto normal = item.at("normal").asDoubles();
        selector = std::make_shared<PlaneSelector>(
            Parfait::Point<double>{center[0], center[1], center[2]},
            Parfait::Point<double>{normal[0], normal[1], normal[2]});
    } else if (type == "x-slice") {
        double at = item.at("at").asDouble();
        int d = 0;
        double epsilon = 1.0e-10;
        double min = -100e20;
        double max = 100e20;
        Parfait::Extent<double> e{{min, min, min}, {max, max, max}};
        e.lo[d] = at - epsilon;
        e.hi[d] = at + epsilon;
        selector = std::make_shared<ExtentSelector>(e);
    } else if (type == "y-slice") {
        double at = item.at("at").asDouble();
        int d = 1;
        double epsilon = 1.0e-10;
        double min = -100e20;
        double max = 100e20;
        Parfait::Extent<double> e{{min, min, min}, {max, max, max}};
        e.lo[d] = at - epsilon;
        e.hi[d] = at + epsilon;
        selector = std::make_shared<ExtentSelector>(e);
    } else if (type == "z-slice") {
        double at = item.at("at").asDouble();
        int d = 2;
        double epsilon = 1.0e-10;
        double min = -100e20;
        double max = 100e20;
        Parfait::Extent<double> e{{min, min, min}, {max, max, max}};
        e.lo[d] = at - epsilon;
        e.hi[d] = at + epsilon;
        selector = std::make_shared<ExtentSelector>(e);
    } else if (type == "line") {
        auto a = item.at("a").asDoubles();
        auto b = item.at("b").asDoubles();
        Parfait::Point<double> pa{a[0], a[1], a[2]};
        Parfait::Point<double> pb{b[0], b[1], b[2]};
        selector = std::make_shared<LineSegmentSelector>(pa, pb);
    }
    CellIdFilter filter(mp.getCommunicator(), mesh, selector);
    visualizeFromFilter(mp, filter, fields, item.at("filename").asStrings());
}

void visualizeIsoSurface(MessagePasser mp,
                         std::shared_ptr<MeshInterface> mesh,
                         const std::vector<std::shared_ptr<FieldInterface>>& fields,
                         const Parfait::Dictionary& item) {
    typedef std::function<double(int)> IsoFunction;

    if (not item.has("type")) {
        PARFAIT_THROW("item doesn't have a type: " + item.dump(4));
    }
    auto type = item.at("type").asString();  // options: [plane, sphere, isosurface]
    type = Parfait::StringTools::tolower(type);
    IsoFunction function = nullptr;
    if (type == "plane") {
        auto center = item.at("center").asDoubles();
        auto normal = item.at("normal").asDoubles();
        function = isosampling::plane(
            *mesh, {center[0], center[1], center[2]}, {normal[0], normal[1], normal[2]});
    } else if (type == "sphere") {
        auto center = item.at("center").asDoubles();
        auto radius = item.at("radius").asDouble();
        function = isosampling::sphere(*mesh, {center[0], center[1], center[2]}, radius);
    } else {
        PARFAIT_WARNING("Could not find support for sampling geometry of type: " + type);
        return;
    }

    auto iso = isosampling::Isosurface(mp, mesh, function);
    visualizeFromFilter(mp, iso, fields, item.at("filename").asStrings());
}

std::vector<int> getRequestedSurfaceTags(MessagePasser mp,
                                         const MeshInterface& mesh,
                                         const Parfait::Dictionary& item) {
    if (not item.has("surface tags")) {
        return {};
    }
    if (item.at("surface tags").type() == Parfait::Dictionary::String and
        item.at("surface tags").asString() == "no") {
        return {};
    }
    if (item.at("surface tags").type() == Parfait::Dictionary::String and
        item.at("surface tags").asString() == "all") {
        auto tags = inf::extractAll2DTags(mp, mesh);
        return std::vector<int>(tags.begin(), tags.end());
    }
    return item.at("surface tags").asInts();
}

std::vector<std::shared_ptr<FieldInterface>> getRequestedFields(
    std::vector<std::shared_ptr<FieldInterface>> fields, const Parfait::Dictionary& item) {
    // when in doubt, use all fields
    if (not item.has("fields")) {
        return fields;
    }
    if (item.at("fields").type() == Parfait::Dictionary::String and
        item.at("fields").asString() == "all") {
        return fields;
    }

    // many ways to say no fields
    if (item.at("fields").type() == Parfait::Dictionary::String and
        item.at("fields").asString() == "none") {
        return {};
    }
    if (item.at("fields").type() == Parfait::Dictionary::String and
        item.at("fields").asString() == "no") {
        return {};
    }

    // Add just requested fields
    if (item.at("fields").type() == Parfait::Dictionary::StringArray) {
        auto names_vec = item.at("fields").asStrings();
        std::set<std::string> names(names_vec.begin(), names_vec.end());
        std::vector<std::shared_ptr<FieldInterface>> requested_fields;
        for (const auto& f : fields) {
            if (names.count(f->name())) {
                requested_fields.push_back(f);
            }
        }
        return requested_fields;
    }
    return fields;
}

void visualizeItem(MessagePasser mp,
                   std::shared_ptr<MeshInterface> mesh,
                   std::vector<std::shared_ptr<FieldInterface>> fields,
                   const Parfait::Dictionary& item) {
    fields = getRequestedFields(fields, item);
    auto surface_tags = getRequestedSurfaceTags(mp, *mesh, item);
    if (surface_tags.size() > 0) {
        std::tie(mesh, fields) = inf::shortcut::filterBySurfaceTags(mp, mesh, surface_tags, fields);
    }
    convertToNodes(mp, *mesh, fields);

    bool crinkle = false;
    if (item.has("crinkle")) {
        if (item.at("crinkle").asBool()) {
            crinkle = true;
        }
    }
    if (crinkle) {
        visualizeCrinkleSubvolume(mp, mesh, fields, item);
        return;
    }
    if (item.at("type").asString() == "line") {
        auto a = item.at("a").asDoubles();
        auto b = item.at("b").asDoubles();
        Parfait::Point<double> pa{a[0], a[1], a[2]};
        Parfait::Point<double> pb{b[0], b[1], b[2]};
        shortcut::visualize_line_sample(item.at("filename").asString(), mp, mesh, pa, pb, fields);
    } else {
        visualizeIsoSurface(mp, mesh, fields, item);
    }
}

Parfait::Rules getVisualizeFromDictionaryRules() {
    Parfait::Rules rules;
    rules.requireField("type");
    rules["type"].addOption("line").addOption("sphere").addOption("plane");

    Parfait::Rules line_rules;
    line_rules.requireField("a").requireField("b");
    line_rules["a"].requireIs3DPoint();
    line_rules["b"].requireIs3DPoint();
    rules.addOneOf(line_rules);

    Parfait::Rules plane_rules;
    plane_rules.requireField("center").requireField("normal");
    plane_rules["center"].requireIs3DPoint();
    plane_rules["normal"].requireIs3DPoint();
    rules.addOneOf(plane_rules);

    Parfait::Rules sphere_rules;
    sphere_rules.requireField("center").requireField("radius");
    sphere_rules["center"].requireIs3DPoint();
    sphere_rules["radius"].requirePositiveDouble();
    rules.addOneOf(sphere_rules);

    Parfait::Rules surface_tag_rules;
    Parfait::Rules tags_disabled;
    tags_disabled.requireType("string");
    tags_disabled.addEnum("no").addEnum("all");
    surface_tag_rules.addOneOf(tags_disabled);
    Parfait::Rules tags_listed;
    tags_listed.requireType("array");
    surface_tag_rules.addOneOf(tags_listed);
    rules["surface tags"] = surface_tag_rules;
    return rules;
}

void visualizeFromDictionary(MessagePasser mp,
                             std::shared_ptr<MeshInterface> mesh,
                             std::vector<std::shared_ptr<FieldInterface>> fields,
                             const Parfait::Dictionary& dict) {
    for (auto& item : dict.asObjects()) {
        visualizeItem(mp, mesh, fields, item);
    }
}

}
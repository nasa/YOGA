#include "Region.h"

namespace Parfait {
namespace RegionFactory {
    inline std::string buildInvalidConfigMessage(std::string config) {
        return "Could not create a region from configuration:\n<" + config + "\n>";
    }

    inline std::shared_ptr<Region> createExtent(const std::vector<std::string>& config_parts) {
        Parfait::Extent<double> e;
        int i = 2;
        e.lo[0] = StringTools::toDouble(config_parts.at(i + 0));
        e.lo[1] = StringTools::toDouble(config_parts.at(i + 1));
        e.lo[2] = StringTools::toDouble(config_parts.at(i + 2));
        e.hi[0] = StringTools::toDouble(config_parts.at(i + 3));
        e.hi[1] = StringTools::toDouble(config_parts.at(i + 4));
        e.hi[2] = StringTools::toDouble(config_parts.at(i + 5));
        return std::make_shared<ExtentRegion>(e);
    }

    inline std::shared_ptr<Region> createHex(const std::vector<std::string>& config_parts) {
        std::array<Parfait::Point<double>, 8> corners;
        int next = 1;
        for (int i = 0; i < 8; i++) {
            corners[i][0] = Parfait::StringTools::toDouble(config_parts.at(next++));
            corners[i][1] = Parfait::StringTools::toDouble(config_parts.at(next++));
            corners[i][2] = Parfait::StringTools::toDouble(config_parts.at(next++));
        }
        return std::make_shared<HexRegion>(corners);
    }

    inline std::shared_ptr<Region> createSphere(const std::vector<std::string>& config_parts) {
        bool radius_set = false;
        bool center_set = false;
        double radius = 1.0;
        Point<double> center = {0, 0, 0};
        for (unsigned int i = 1; i < config_parts.size(); i++) {
            if (config_parts[i] == "radius") {
                radius = StringTools::toDouble(config_parts.at(i + 1));
                radius_set = true;
            } else if (config_parts[i] == "center") {
                center[0] = StringTools::toDouble(config_parts.at(i + 1));
                center[1] = StringTools::toDouble(config_parts.at(i + 2));
                center[2] = StringTools::toDouble(config_parts.at(i + 3));
                center_set = true;
            } else if (config_parts[i] == "end") {
                break;
            }
        }
        if (not radius_set or not center_set) {
            PARFAIT_THROW(buildInvalidConfigMessage(Parfait::StringTools::join(config_parts, " ")));
        }
        return std::make_shared<SphereRegion>(center, radius);
    }

    inline std::shared_ptr<Region> createOneRegion(const std::string& config) {
        auto config_parts = StringTools::split(config, " ");
        if (config_parts.empty()) PARFAIT_THROW(buildInvalidConfigMessage(config));
        if (config_parts[0] == "sphere") {
            return createSphere(config_parts);
        } else if (config_parts[0] == "cartesian" and config_parts.at(1) == "box") {
            return createExtent(config_parts);
        } else if (config_parts[0] == "hex") {
            return createHex(config_parts);
        }
        PARFAIT_THROW(buildInvalidConfigMessage(config));
    }

    inline std::shared_ptr<Region> create(std::string config) {
        config = Parfait::StringTools::findAndReplace(config, "\n", " ");
        auto config_objects = Parfait::StringTools::split(config, "end");
        config_objects = Parfait::StringTools::removeWhitespaceOnlyEntries(config_objects);

        if (config_objects.size() > 1) {
            auto composite = std::make_shared<CompositeRegion>();
            for (auto& object_config : config_objects) {
                composite->add(createOneRegion(object_config));
            }
            return composite;
        }

        return createOneRegion(config);
    }
}
}

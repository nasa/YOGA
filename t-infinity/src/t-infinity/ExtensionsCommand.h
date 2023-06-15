#include <t-infinity/SubCommand.h>
#include <parfait/Dictionary.h>
#include <parfait/FileTools.h>
#include <t-infinity/PluginLocator.h>
#include "ExtensionsHelpers.h"
#include <sys/stat.h>

using namespace inf;
using namespace std;

class ExtensionsCommand : public SubCommand {
  public:
    std::string description() const override { return "Load subcommand extensions"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"load"}, "load an extension", false);
        m.addParameter({"load-all"}, "load all available extensions", false);
        m.addParameter({"unload"}, "unload an extension", false);
        m.addFlag({"list"}, "list loaded extensions");
        m.addFlag({"avail"}, "list available extensions");
        m.addFlag({"purge"}, "unload all extensions");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        createInfDirIfDoesntExist();
        if (m.has("list")) {
            printf("Loaded extensions:\n");
            list(getLoadedExtensionsFromFile());
        } else if (m.has("avail")) {
            printf("Available extensions:\n");
            list(getAvailableExtensions(prefix));
        } else if (m.has("load")) {
            load(m.get("load"));
        } else if (m.has("unload")) {
            unload(m.get("unload"));
        } else if (m.has("purge")) {
            purge();
        } else if (m.has("load-all")) {
            for (auto& extension : getAvailableExtensions(prefix)) load(extension);
        }
    }

  private:
    string infinity_dir = getInfDir();
    string extension_list_file = getExtensionsSettingsFilename();
    string prefix = DRIVER_PREFIX;

    void load(string extension) {
        Parfait::Dictionary dict;
        auto available = getAvailableExtensions(prefix);
        if (!contains(available, extension)) {
            printf("No extension named: %s\n", extension.c_str());
        } else {
            dict = getExtensionSettings();
            vector<string> loaded;
            if (dict.has("loaded")) loaded = dict["loaded"].asStrings();
            if (not contains(loaded, extension)) {
                loaded.push_back(extension);
            }
            dict["loaded"] = loaded;

            string file = infinity_dir + "/" + extension_list_file;
            Parfait::FileTools::writeStringToFile(file, dict.dump());
            printf("<Settings cached in: %s>\n", file.c_str());
            printf("Loaded extension: %s\n", extension.c_str());
        }
    }

    void purge() {
        Parfait::Dictionary new_settings;
        vector<string> core = {"core"};
        new_settings["loaded"] = core;

        string file = infinity_dir + "/" + extension_list_file;
        Parfait::FileTools::writeStringToFile(file, new_settings.dump());
    }

    void unload(string extension) {
        auto loaded = getLoadedExtensionsFromFile();
        if (!contains(loaded, extension)) {
            printf("Extension not loaded: %s ...doing nothing\n", extension.c_str());
        } else if ("core" == extension) {
            printf("Refusing to unload: core\n");
        } else {
            auto old_settings = getExtensionSettings();
            vector<string> now_loaded;
            for (auto ext : loaded)
                if (ext != extension) now_loaded.push_back(ext);

            Parfait::Dictionary new_settings;
            new_settings["loaded"] = now_loaded;

            string file = infinity_dir + "/" + extension_list_file;
            Parfait::FileTools::writeStringToFile(file, new_settings.dump());
            printf("Unloaded extension: %s\n", extension.c_str());
        }
    }

    void createInfDirIfDoesntExist() {
        if (not Parfait::FileTools::doesDirectoryExist(infinity_dir))
            mkdir(infinity_dir.c_str(), 0700);
    }

    bool contains(const vector<string>& extensions, string extension) {
        return find(extensions.begin(), extensions.end(), extension) != extensions.end();
    }

    void list(const vector<string>& extensions) {
        for (auto ext : extensions) printf("-- %s\n", ext.c_str());
    }
};

CREATE_INF_SUBCOMMAND(ExtensionsCommand)
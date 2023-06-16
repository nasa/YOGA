#include <t-infinity/SubCommand.h>
#include <parfait/FileTools.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/PluginLocator.h>

namespace inf  {

class ListPluginCommand : public SubCommand {
  public:
    std::string description() const override { return "list installed plugins"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto dir = inf::getPluginDir();
        listPlugins(dir,"PreProcessor","createPreProcessor");
        listPlugins(dir,"Fluid Solver","createFluidSolverFactory");
        listPlugins(dir,"Visualization","createVizFactory");
        listPlugins(dir,"Domain Assembler","createDomainAssemblerFactory");

    }

  private:
    void listPlugins(const std::string& path,const std::string& type,const std::string& creator_string) const {
        auto filenames = Parfait::FileTools::listFilesInPath(path);
        std::vector<std::string> plugins;
        std::vector<std::string> dynamic_lib_extensions{"dylib","so"};
        for(auto& file:filenames) {
            if(Parfait::StringTools::contains(file,"SubCommand"))
                continue;
            if (plugins::doesSymbolExist(path + file, creator_string))
                plugins.push_back(Parfait::StringTools::stripExtension(file, dynamic_lib_extensions));
        }
        printf("%s:\n",type.c_str());
        for(auto name: plugins)
            printf("    %s\n",name.c_str());
        printf("\n");

//        printf("in list plugin: "); tracer_print_handle();
    }
};
}

CREATE_INF_SUBCOMMAND(inf::ListPluginCommand)
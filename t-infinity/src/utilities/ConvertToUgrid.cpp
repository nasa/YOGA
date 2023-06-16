#include <MessagePasser/MessagePasser.h>
#include <t-infinity/WriteUgrid.h>
#include <iostream>
#include <string>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/PluginLocator.h>
#include <parfait/CommandLineMenu.h>

void printHelpAndExit(MessagePasser mp, int exit_code) {
    if (mp.Rank() == 0) {
        std::cout << "ConvertToUgrid " << std::endl;
        std::cout << "Usage: ./ConvertToUgrid <filename>" << std::endl;
    }
    mp.Barrier();
    exit(exit_code);
}

bool isFilenameOk(const std::string filename) {
    auto filename_parts = Parfait::StringTools::split(filename, ".");
    auto extension = Parfait::StringTools::getExtension(filename);
    printf("Extension = %s\n", extension.c_str());
    return (extension == "ugrid" or extension == "meshb" or extension == "cgns" or extension == "msh");
}

int main(int argc, const char* argv[]) {
    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);
    Parfait::CommandLineMenu menu;
    menu.addParameter({"--pre-processor"},"choose plugin",false, "NC_PreProcessor");
    menu.addParameter({"--grid"},"filename",true);
    menu.addParameter({"-o"},"output ugrid base name",false,"out.lb8.ugrid");
    menu.parse(argc,argv);

    std::string mesh_loader_plugin_name = menu.get("--pre-processor");
    std::string mesh_filename = menu.get("--grid");
    std::string out_name =  menu.get("-o");
    if(isFilenameOk(mesh_filename)){
        if (mp.Rank() == 0) printf("Using mesh pre-processor: %s\n", mesh_loader_plugin_name.c_str());
        auto pp = inf::getMeshLoader(inf::getPluginDir(), mesh_loader_plugin_name);

        auto mesh = pp->load(mp.getCommunicator(), mesh_filename);

        inf::writeUgrid(out_name, mesh);
    }
    else{
        printf("%s",menu.to_string().c_str());
    }


    MessagePasser::Finalize();
}

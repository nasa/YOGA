#include <t-infinity/CommandRunner.h>
#include <parfait/CommandLine.h>

void removeProgramNameFromArgs(std::vector<std::string>& args) { args.erase(args.begin(), args.begin() + 1); }

int main(int argc, const char* argv[]) {
    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);

    auto args = Parfait::CommandLine::getCommandLineArgs(argc, argv);
    removeProgramNameFromArgs(args);

    std::string sub_command_prefix = "NueCommand_";
    inf::CommandRunner runner(mp, inf::getPluginDir(), {sub_command_prefix});
    runner.run(args);

    MessagePasser::Finalize();
    return 0;
}

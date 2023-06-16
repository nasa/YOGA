#include <t-infinity/CommandRunner.h>
#include <Tracer.h>
#include <parfait/CommandLine.h>
#include "ExtensionsHelpers.h"
#include <t-infinity/Communicator.h>

bool unsafeMode(std::vector<std::string>& args);
void initializeTracerIfRequested(MessagePasser mp, std::vector<std::string>& args);
void removeProgramNameFromArgs(std::vector<std::string>& args) {
    args.erase(args.begin(), args.begin() + 1);
}

int main(int argc, const char* argv[]) {
    inf::initializeMPI();
    MessagePasser mp(inf::getCommunicator());
    std::string driver_prefix = DRIVER_PREFIX;

    auto args = Parfait::CommandLine::getCommandLineArgs(argc, argv);
    removeProgramNameFromArgs(args);
    initializeTracerIfRequested(mp, args);

    std::vector<std::string> sub_command_prefixes;
    auto available_extensions = inf::getLoadedExtensionsFromFile();
    for (const auto& extension : available_extensions)
        sub_command_prefixes.push_back(driver_prefix + "_SubCommand_" + extension + "_");
    inf::CommandRunner runner(mp, inf::getPluginDir(), sub_command_prefixes);

    int command_status = 0;
    if (unsafeMode(args)) {
        runner.run(args);
    } else {
        try {
            runner.run(args);
        } catch (const std::exception& e) {
            command_status = 1;
            printf("%s\n", e.what());
            fflush(stdout);
            mp.Abort(1);
        }
    }

    Tracer::finalize();
    inf::finalizeMPI();
    return command_status;
}

bool unsafeMode(std::vector<std::string>& args) {
    auto x = Parfait::CommandLine::popArgumentMatching({"--unsafe"}, args);
    return not x.empty();
}

void initializeTracerIfRequested(MessagePasser mp, std::vector<std::string>& args) {
    auto tracer_rank_args = Parfait::CommandLine::popArgumentMatching({"--trace-ranks"}, args);

    auto tracer_args = Parfait::CommandLine::popArgumentMatching({"--trace"}, args);
    bool should_trace = (not tracer_args.empty()) or (not tracer_rank_args.empty());
    if (not should_trace) return;

    std::string tracer_basename = "trace";
    if (tracer_args.size() == 2) tracer_basename = tracer_args[1];

    if (tracer_rank_args.size() > 1) {
        tracer_rank_args.erase(tracer_rank_args.begin());
        auto ranks_string = Parfait::StringTools::join(tracer_rank_args, " ");
        auto ranks = Parfait::StringTools::parseIntegerList(ranks_string);
        Tracer::initialize(tracer_basename, mp.Rank(), ranks);
        Tracer::setDebug();
    } else {
        Tracer::initialize(tracer_basename, mp.Rank());
        Tracer::setDebug();
    }
}

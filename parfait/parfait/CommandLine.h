#pragma once
#include <vector>
#include <string>
#include "VectorTools.h"

namespace Parfait {

namespace CommandLine {
    inline std::vector<std::string> getCommandLineArgs(int argc, const char** argv) {
        std::vector<std::string> args = {};
        for (int i = 0; i < argc; i++) {
            args.emplace_back(argv[i]);
        }
        return args;
    }

    inline bool isHelpRequested(const std::vector<std::string>& args) {
        for (auto& arg : args) {
            if (arg == "--help" or arg == "-h") {
                return true;
            }
        }
        return false;
    }

    inline std::vector<std::string> popArgumentMatching(const std::vector<std::string>& flags,
                                                        std::vector<std::string>& args) {
        std::vector<std::string> popped_arguments;
        std::vector<std::string> copied_args;

        bool searching = false;
        for (unsigned int i = 0; i < args.size(); i++) {
            const std::string& arg = args[i];
            if (searching) {
                if (arg[0] == '-') {
                    copied_args.insert(copied_args.end(), args.begin() + i, args.end());
                    break;
                } else {
                    popped_arguments.push_back(arg);
                }
            } else if (Parfait::VectorTools::isIn(flags, arg)) {
                popped_arguments.push_back(arg);
                searching = true;
            } else {
                copied_args.push_back(arg);
            }
        }

        args = copied_args;
        return popped_arguments;
    }
}
}

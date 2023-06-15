#pragma once
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include "PluginLocator.h"
#include "PluginLoader.h"
#include "SubCommand.h"
#include "Version.h"

namespace inf {

class CommandRunner {
  public:
    using Args = const std::vector<std::string>&;
    using String = const std::string&;
    CommandRunner(MessagePasser mp, String sub_command_path, Args sub_command_prefixes)
        : mp(mp), subcommand_path(sub_command_path), subcommand_prefixes(sub_command_prefixes) {
        if (Parfait::FileTools::doesDirectoryExist(subcommand_path) &&
            subcommand_path.back() != '/') {
            subcommand_path += "/";
        }
    }

    void run(Args args) {
        if (versionRequested(args)) {
            mp_rootprint("Version: %s\n", version().c_str());
            return;
        } else if (searchingForSubstring(args)) {
            auto search_string = args.back();
            printSubCommandList(search_string);
            return;
        }
        for (String lib : Parfait::FileTools::listFilesInPath(subcommand_path)) {
            if (not isAKnownSubCommand(lib)) continue;
            auto name = extractSubCommandName(args);
            if (getLibrarySubCommandName(lib) == name) {
                auto sub_command = load(subcommand_path + lib);
                auto sub_args = extractSubCommandArgs(args);
                if (not name.empty() and sub_command) {
                    auto sub_menu = sub_command->menu();
                    if (Parfait::CommandLine::isHelpRequested(sub_args)) {
                        printHelp(name, *sub_command, sub_menu, hiddenHelpRequested(args));
                    } else if (sub_menu.parse(sub_args)) {
                        sub_command->run(sub_menu, mp);
                    } else {
                        printHelp(name, *sub_command, sub_menu, hiddenHelpRequested(args));
                        mp.Barrier();
                        exit(1);
                    }
                    return;
                }
            }
        }
        if (mp.Rank() == 0) printSubCommandList(MATCH_ALL);
    }

    void run(const std::string& command) {
        auto args = Parfait::StringTools::split(command, " ");
        run(args);
    }

  private:
    MessagePasser mp;
    std::string subcommand_path;
    std::vector<std::string> subcommand_prefixes;
    std::map<std::string, std::shared_ptr<SubCommand>> sub_commands;
    const std::string MATCH_ALL;

    bool isAKnownSubCommand(String lib_name) {
        bool is_known = false;
        for (const auto& prefix : subcommand_prefixes)
            if (Parfait::StringTools::contains(lib_name, prefix)) is_known = true;
        return is_known;
    }

    std::string extractSubCommandName(Args args) {
        std::string name;
        if (not args.empty()) name = args.front();
        return name;
    }

    std::vector<std::string> extractSubCommandArgs(Args args) {
        if (args.empty()) return {};
        std::vector<std::string> remaining = args;
        remaining.erase(remaining.begin(), remaining.begin() + 1);
        return remaining;
    }

    std::string getLibrarySubCommandName(String lib) {
        using namespace Parfait::StringTools;
        auto name = stripExtension(lib, ".so");
        auto subcommand_prefix = getPrefix(lib);
        name = findAndReplace(name, "lib" + subcommand_prefix, subcommand_prefix);
        return findAndReplace(name, subcommand_prefix, "");
    }

    std::string getPrefix(String lib) {
        std::string prefix = "";
        for (const auto& p : subcommand_prefixes)
            if (Parfait::StringTools::contains(lib, p)) prefix = p;
        return prefix;
    }

    static std::shared_ptr<inf::SubCommand> load(const std::string& libname) {
        using ReturnType = std::shared_ptr<SubCommand>;
        std::function<ReturnType()> createSubCommand = nullptr;
        try {
            createSubCommand = plugins::loadSymbol<ReturnType()>(libname, "getSubCommand");
        } catch (std::exception& e) {
            printf("Error loading subcommand plugin -> %s\n", libname.c_str());
            printf("%s\n", e.what());
            return nullptr;
        }
        return createSubCommand();
    }

    void printSubCommandList(String search_string) {
        int column_width = 55;
        int left_column_width = 25;
        std::string spaces;
        spaces.resize(left_column_width, ' ');
        std::string header_a = "Sub-commands:";
        std::string header_b = "Description:";
        header_a.resize(left_column_width, ' ');
        printf("%s%s\n", header_a.c_str(), header_b.c_str());
        std::string dashes;
        dashes.resize(left_column_width + column_width, '-');
        printf("%s\n", dashes.c_str());
        for (String lib : Parfait::FileTools::listFilesInPath(subcommand_path)) {
            if (not isAKnownSubCommand(lib)) continue;
            auto sub_command = load(subcommand_path + lib);
            if (sub_command) {
                auto command = getLibrarySubCommandName(lib);
                auto description = sub_command->description();
                if (not matchesSearchString(search_string, command, description)) continue;
                auto lines = Parfait::StringTools::wrapLines(description, column_width);
                command.resize(left_column_width - 3, ' ');
                printf("%s-> %s\n", command.c_str(), lines[0].c_str());

                for (int i = 1; i < int(lines.size()); i++)
                    printf("%s%s\n", spaces.c_str(), lines[i].c_str());
            }
        }
        printf("%s\n", dashes.c_str());
    }

    void printHelp(String name,
                   const SubCommand& sc,
                   Parfait::CommandLineMenu sub_menu,
                   bool print_hidden_options_help) {
        if (mp.Rank() != 0) return;
        if ("null" == name) {
            printSubCommandList(MATCH_ALL);
        } else {
            std::string help_string = sub_menu.to_string();
            if (print_hidden_options_help) help_string = sub_menu.to_string_include_hidden();
            printf("sub-command: %s\n", name.c_str());
            printf("description: %s\n", sc.description().c_str());
            printf("options:\n%s", help_string.c_str());
        }
    }

    bool hiddenHelpRequested(Args args) {
        for (const auto& a : args) {
            if (a == "--show-hidden") return true;
        }
        return false;
    }

    bool versionRequested(Args args) {
        for (const auto& a : args)
            if (a == "--version" or a == "-V") return true;
        return false;
    }
    bool searchingForSubstring(Args args) { return 2 == args.size() and "search" == args.front(); }
    bool matchesSearchString(String s, String command, String description) {
        if (MATCH_ALL == s) return true;
        return Parfait::StringTools::contains(command, s) or
               Parfait::StringTools::contains(description, s);
    }
};
}

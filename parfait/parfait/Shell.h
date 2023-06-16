#pragma once
#include <string>
#include <vector>
#include "StringTools.h"
#include "Throw.h"

namespace Parfait {
class Shell {
  public:
    struct Status {
        int exit_code;
        std::string output;
    };
    inline int sh(std::string command) const noexcept {
        int err = system(command.c_str());
        return err;
    }

    inline int sh(const std::vector<std::string> commands) const noexcept {
        std::string command = Parfait::StringTools::join(commands, " && ");
        return sh(command);
    }

    inline Status exec(std::vector<std::string> commands) const {
        std::string command = Parfait::StringTools::join(commands, " && ");
        return exec(command);
    }

    inline Status exec(std::string command) const {
        if (enable_echo_commands) {
            printf("Command: <%s>\n", command.c_str());
        }
        auto s = exec_impl(command);
        if (enable_throw and s.exit_code != 0) {
            PARFAIT_THROW("Command: " + command + " exit code: " + std::to_string(s.exit_code) +
                          " output: " + s.output);
        }
        return s;
    }

    void setThrowOnFailure() { enable_throw = true; }
    void disableThrowOnFailure() { enable_throw = false; }

    void setEchoCommands() { enable_echo_commands = true; }
    void disableEchoCommands() { enable_echo_commands = false; }

  private:
    bool enable_throw = false;
    bool enable_echo_commands = false;

    inline Status exec_impl(std::string command) const noexcept {
        Status s;
        s.exit_code = 0;
        char buffer[128];
        std::string result = "";
        std::string redirect_command = command + " 2>&1";
        FILE* pipe = popen(redirect_command.c_str(), "r");
        if (!pipe) {
            s.exit_code = -428;
            s.output += "Error opening pipe for command " + command;
            return s;
        }
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            s.exit_code = -429;
            s.output += result + "Error executing command " + command;
            return s;
        }
        s.exit_code = pclose(pipe) / 256;
        if (s.exit_code != 0) {
            result += "Error executing command " + command;
        }
        s.output = result;
        return s;
    }
};
class RemoteShellCommandGenerator {
  public:
    inline RemoteShellCommandGenerator(std::string machine_name) : machine_name(machine_name) {}

    inline std::string generate(std::string command) { return generate(std::vector<std::string>{command}); }

    inline std::string generate(std::vector<std::string> commands) {
        commands.front() = "'" + commands.front();
        commands.back() = commands.back() + "'";
        auto single_command = Parfait::StringTools::join(commands, " && ");
        single_command = "ssh " + machine_name + " " + single_command;
        return single_command;
    }

  private:
    std::string machine_name;
};
}

#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include <parfait/Shell.h>
#include <thread>

void waitOnFilesystem() { std::this_thread::sleep_for(std::chrono::milliseconds(300)); }

TEST_CASE("Shell can run a simple command") {
    Parfait::Shell shell;
    std::string random_filename = Parfait::StringTools::randomLetters(5) + ".txt";
    REQUIRE_FALSE(Parfait::FileTools::doesFileExist(random_filename));

    shell.sh("touch " + random_filename);
    waitOnFilesystem();
    REQUIRE(Parfait::FileTools::doesFileExist(random_filename));

    shell.sh("rm " + random_filename);
}

TEST_CASE("Shell can run multiple commands") {
    std::string random_folder = Parfait::StringTools::randomLetters(5);
    std::string random_filename = Parfait::StringTools::randomLetters(5) + ".txt";

    Parfait::Shell shell;
    shell.sh({"mkdir -p " + random_folder, "cd " + random_folder, "touch " + random_filename});

    waitOnFilesystem();
    REQUIRE(Parfait::FileTools::doesDirectoryExist(random_folder));
    REQUIRE(Parfait::FileTools::doesFileExist(random_folder + "/" + random_filename));
    shell.sh("rm -rf " + random_folder);
}

TEST_CASE("Shell can queue multiple commands") {
    std::string random_folder = Parfait::StringTools::randomLetters(5);
    std::string random_filename = Parfait::StringTools::randomLetters(5) + ".txt";

    Parfait::Shell shell;
    std::vector<std::string> commands;
    commands.push_back("mkdir -p " + random_folder);
    commands.push_back("cd " + random_folder);
    commands.push_back("touch " + random_filename);
    shell.sh(commands);

    waitOnFilesystem();
    REQUIRE(Parfait::FileTools::doesDirectoryExist(random_folder));
    REQUIRE(Parfait::FileTools::doesFileExist(random_folder + "/" + random_filename));
    shell.sh("rm -rf " + random_folder);
}

TEST_CASE("Shell will report errors") {
    Parfait::Shell shell;
    std::string random_folder_that_doesnt_exist = Parfait::StringTools::randomLetters(5);
    REQUIRE(shell.sh("cd " + random_folder_that_doesnt_exist) != 0);
}

TEST_CASE("execute will report errors") {
    Parfait::Shell shell;
    std::string random_folder = Parfait::StringTools::randomLetters(5);
    auto status = shell.exec("cd " + random_folder);
    REQUIRE(status.exit_code != 0);

    shell.sh("mkdir -p " + random_folder);
    status = shell.exec("cd " + random_folder);
    REQUIRE(status.exit_code == 0);

    shell.sh("rm -rf " + random_folder);
}

TEST_CASE("Can queue and execute and get an error code") {
    Parfait::Shell shell;
    std::string random_folder = Parfait::StringTools::randomLetters(5);
    REQUIRE_FALSE(Parfait::FileTools::doesDirectoryExist(random_folder));
    auto status = shell.exec("cd " + random_folder);
    REQUIRE(status.exit_code == 1);
}

TEST_CASE("exec will throw if errors") {
    Parfait::Shell shell;
    shell.setThrowOnFailure();
    std::string random_folder_that_doesnt_exist = Parfait::StringTools::randomLetters(5);
    REQUIRE_THROWS(shell.exec("cd " + random_folder_that_doesnt_exist));
}

TEST_CASE("exec will return stdout") {
    Parfait::Shell shell;
    std::string random_file = Parfait::StringTools::randomLetters(5) + ".txt";
    shell.sh("touch " + random_file);
    auto output = shell.exec("ls").output;
    REQUIRE(Parfait::StringTools::contains(output, random_file));

    shell.sh("rm " + random_file);
}

TEST_CASE("Remote Shell Generator tests") {
    Parfait::RemoteShellCommandGenerator remote_shell("K.larc.nasa.gov");
    SECTION("single command") {
        auto command = remote_shell.generate("pwd");
        REQUIRE(command == "ssh K.larc.nasa.gov 'pwd'");
    }
    SECTION("multiple commands") {
        auto command = remote_shell.generate(std::vector<std::string>{"cd dog", "ls"});
        REQUIRE(command == "ssh K.larc.nasa.gov 'cd dog && ls'");
    }
}
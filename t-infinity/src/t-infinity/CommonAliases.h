#pragma once
#include <parfait/CommandLineMenu.h>

namespace inf {

class Alias {
  public:
    using Strings = std::vector<std::string>;
    static Strings help() { return {"h", "help"}; }
    static Strings inputFile() { return {"f", "file"}; }
    static Strings mesh() { return {"mesh", "m"}; }
    static Strings tags() { return {"tags", "t"}; }
    static Strings select() { return {"select"}; }
    static Strings snap() { return {"snap"}; }
    static Strings preprocessor() { return {"pre-processor"}; }
    static Strings postprocessor() { return {"post-processor"}; }
    static Strings plugindir() { return {"plugin-dir"}; }
    static Strings outputFileBase() { return {"o"}; }
};

class Help {
  public:
    using String = std::string;
    static String help() { return "print help menu"; }
    static String preprocessor() { return "plugin"; }
    static String postprocessor() { return "plugin"; }
    static String mesh() { return {"input mesh file"}; }
    static String plugindir() { return {"set path to installed plugins"}; }
    static String outputFileBase() { return {"output file name"}; }
    static String snap() { return {"input snap file"}; }
    static String inputFile() { return "input filename"; }
};
}

#pragma once
#include <stdio.h>
#include <string>

namespace YOGA {
class RootPrinter {
  public:
    RootPrinter(int rank) : is_root(0 == rank) {}
    void print(std::string s) const {
        if (is_root) printf("%s", s.c_str());
    }
    void println(std::string s) const {
        if (is_root) printf("%s\n", s.c_str());
    }

  private:
    RootPrinter() = delete;
    bool is_root;
};
}

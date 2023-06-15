#include "Demangle.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <execinfo.h>
#include <iterator>
#include <sstream>

#include <cxxabi.h>

void handler() {
    printf("INSIDE HANDLER\n");
    void* trace_elems[50];
    int trace_elem_count(backtrace(trace_elems, 50));
    char** stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
    for (int i = 0; i < trace_elem_count; ++i) {
        auto words = inf::breakSymbolLineIntoWords(stack_syms[i]);
        for (auto& word : words) std::cout << inf::demangle(word) << "     ";
        std::cout << std::endl;
    }
    free(stack_syms);
}

std::string inf::demangle(const std::string& s) {
    size_t len = 256;
    char buffer[256];
    int status = 1;
    const char* p = s.data();
    auto demangled = abi::__cxa_demangle(p, buffer, &len, &status);
    if (0 == status) return demangled;
    return s;
}

std::vector<std::string> inf::breakSymbolLineIntoWords(const std::string& line) {
    std::vector<std::string> words;
    std::istringstream iss(line);
    std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                     std::istream_iterator<std::string>());
    return results;
}

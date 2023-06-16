#pragma once
#include "Throw.h"

namespace Parfait {
namespace FortranUnformatted {
    template <typename T>
    void writeMarker(int n_items, FILE* f) {
        int record_length = n_items * sizeof(T);
        fwrite(&record_length, sizeof(int), 1, f);
    }
    inline void skipMarker(FILE* f) { fseek(f, sizeof(int), SEEK_CUR); }
    inline int readMarker(FILE* f) {
        int m;
        fread(&m, sizeof(int), 1, f);
        return m;
    }

    inline void checkMarker(int expected, FILE* f, const std::string& msg = "", bool silent = false) {
        int actual = readMarker(f);
        if (expected != actual) {
            if (not silent) {
                if (not msg.empty()) printf("Encounter error at: %s\n", msg.c_str());
                printf("expected: %d actual: %d\n", expected, actual);
            }
            PARFAIT_THROW("Invalid read of fortran unformatted file: " + msg);
        }
    }
    inline void skipRecord(FILE* f, std::string msg = "") {
        int m = readMarker(f);
        fseek(f, m, SEEK_CUR);
        checkMarker(m, f, msg);
    }
}
}

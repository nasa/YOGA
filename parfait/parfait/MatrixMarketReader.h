#pragma once
#include "Throw.h"
#include "StringTools.h"
#include "FileTools.h"

class MatrixMarketReader {
  public:
    inline MatrixMarketReader(std::string filename, int block_size) : filename(filename), block_size(block_size) {
        fp = fopen(filename.c_str(), "r");
        if (fp == nullptr) {
            PARFAIT_THROW("Could not open file for reading: " + filename);
        }
        readHeader();
    }
    inline ~MatrixMarketReader() {
        if (fp) fclose(fp);
    }

    inline int numBlockRows() const { return num_rows / block_size; }

    void readEntry(long& row, long& col, double& entry) {
        fread(&row, sizeof(long), 1, fp);
        fread(&col, sizeof(long), 1, fp);
        fread(&entry, sizeof(double), 1, fp);

        row--;
        col--;
    }
    template <typename Callback>
    void readAllEntries(Callback callback) {
        jumpTo(data_offset);

        std::string line;

        if (format == ASCII) {
            int flattened_row, flattened_col;
            double entry;
            while (Parfait::FileTools::readNextLine(fp, line)) {
                line = Parfait::StringTools::strip(line);
                if (parseDataLine(line, flattened_row, flattened_col, entry)) {
                    int block_row = flattened_row / block_size;
                    int block_col = flattened_col / block_size;
                    int equation_row = flattened_row % block_size;
                    int equation_col = flattened_col % block_size;

                    callback(block_row, block_col, equation_row, equation_col, entry);
                }
            }
        }

        if (format == BINARY) {
            long flattened_row, flattened_col;
            double entry;
            for (long nnz = 0; nnz < num_non_zero; nnz++) {
                readEntry(flattened_row, flattened_col, entry);
                long block_row = flattened_row / block_size;
                long block_col = flattened_col / block_size;
                int equation_row = flattened_row % block_size;
                int equation_col = flattened_col % block_size;
                callback(block_row, block_col, equation_row, equation_col, entry);
            }
        }
    }

  private:
    std::string filename;
    FILE* fp;
    size_t header_offset, data_offset;
    int block_size;
    int num_rows, num_columns, num_non_zero;
    enum { ASCII, BINARY };
    int format = ASCII;

    void scanComments() {
        header_offset = 0;

        std::string line;
        while (Parfait::FileTools::readNextLine(fp, line)) {
            if (Parfait::StringTools::contains(line, "binary")) {
                printf("Detected binary file\n");
                format = BINARY;
            }
            if (line[0] == '%')
                header_offset = ftell(fp);
            else
                break;
        }

        jumpTo(header_offset);
    }

    void readHeader() {
        scanComments();
        std::string line;
        Parfait::FileTools::readNextLine(fp, line);
        line = Parfait::StringTools::strip(line);
        auto parts = Parfait::StringTools::split(line, " ");
        num_rows = Parfait::StringTools::toLong(parts[0]);
        num_columns = Parfait::StringTools::toLong(parts[1]);
        num_non_zero = Parfait::StringTools::toLong(parts[2]);
        data_offset = ftell(fp);
    }

    bool parseDataLine(std::string line, int& row, int& col, double& entry) const {
        try {
            auto parts = Parfait::StringTools::split(line, " ");
            if (parts.size() != 3) return false;
            row = Parfait::StringTools::toInt(parts[0]);
            col = Parfait::StringTools::toInt(parts[1]);
            entry = Parfait::StringTools::toDouble(parts[2]);
            row--;  // convert from 1 based indexing
            col--;
        } catch (std::exception& e) {
            return false;
        }
        return true;
    }

    void jumpTo(size_t location) { fseek(fp, location, SEEK_SET); }
};

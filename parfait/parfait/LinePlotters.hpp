// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include <MessagePasser/MessagePasser.h>
#include <vector>
#include "StringTools.h"
#include "FileTools.h"
#include "LinearPartitioner.h"
#include "DataFrame.h"
#include "Checkpoint.h"
#include <iomanip>

namespace Parfait {

namespace LinePlotters {
    inline FILE* openOrThrow(const std::string& filename) {
        FILE* fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) {
            PARFAIT_THROW("Cannot open file to write: " + filename);
        }
        return fp;
    }

    class CSVReader {
      public:
        inline static DataFrame read(const std::string& filename, const std::string& delimiter) {
            if (FileTools::doesFileExist(filename)) {
                auto s = FileTools::loadFileToString(filename);
                return toDataFrame(s, delimiter);
            } else {
                throw std::logic_error("Could not load file: " + filename);
            }
        }
        inline static DataFrame read(const std::string& filename) { return read(filename, ","); }

        inline static DataFrame read(MessagePasser mp, const std::string& filename, const std::string& delimiter) {
            PARFAIT_ASSERT(FileTools::doesFileExist(filename), "Could not load file: " + filename);

            long num_lines = Parfait::FileTools::countLinesInFile(filename);
            long num_rows = num_lines - 1;

            std::string header;
            std::string chunk;
            if (mp.Rank() == 0) {
                FILE* f = fopen(filename.c_str(), "r");
                Parfait::FileTools::readNextLine(f, header);
                std::string chunk_to_send;
                for (int r = 0; r < mp.NumberOfProcesses(); r++) {
                    auto range = Parfait::LinearPartitioner::getRangeForWorker(r, num_rows, mp.NumberOfProcesses());
                    chunk_to_send = Parfait::FileTools::readLinesInFile(f, range.end - range.start);
                    MessagePasser::Message msg(chunk_to_send);
                    if (r == 0) {
                        chunk = chunk_to_send;
                    } else {
                        mp.NonBlockingSend(std::move(msg), r).wait();
                    }
                }

                fclose(f);
            }

            MessagePasser::Message msg;
            if (mp.Rank() != 0) {
                mp.Recv(msg, 0);
                msg.unpack(chunk);
            }
            mp.Broadcast(header, 0);
            return toDataFrame(header + "\n" + chunk, delimiter);
        }
        inline static DataFrame read(MessagePasser mp, const std::string& filename) { return read(mp, filename, ","); }

        inline static DataFrame toDataFrame(const std::string& csv_data_as_string, const std::string& delimiter) {
            DataFrame::MappedColumns csv_data;
            auto lines = StringTools::split(csv_data_as_string, "\n");
            auto column_names = extractHeaderNames(lines.front(), delimiter);
            for (auto& column_name : column_names) {
                csv_data[column_name].reserve(lines.size() - 1);
            }
            for (size_t row = 1; row < lines.size(); row++) {
                auto row_data = StringTools::split(lines.at(row), delimiter);
                for (size_t column = 0; column < row_data.size(); column++) {
                    auto data = StringTools::strip(row_data[column]);
                    if (not data.empty()) {
                        if (column >= column_names.size()) {
                            PARFAIT_THROW("Error parsing csv data. Line has too many columns:\n" + lines.at(row));
                        }
                        csv_data[column_names.at(column)].push_back(StringTools::toDouble(row_data[column]));
                    }
                }
            }
            return DataFrame(csv_data);
        }
        inline static DataFrame toDataFrame(const std::string& csv_data_as_string) {
            return toDataFrame(csv_data_as_string, ",");
        }
        inline static std::vector<std::string> extractHeaderNames(const std::string& line) {
            using namespace StringTools;
            if (isFun3dStyle(line))
                return fun3dStyle(line);
            else
                return commaSeparatedStyle(line);
        }
        inline static std::vector<std::string> extractHeaderNames(const std::string& line, const std::string& delimiter) {
            using namespace StringTools;
            auto words = split(line, delimiter);
            for (auto& word : words) word = lstrip(rstrip(word));
            return words;
        }

        inline static bool isFun3dStyle(const std::string& line) {
            return line.front() == '#' and line.find(',') == line.npos;
        }

        inline static std::vector<std::string> fun3dStyle(const std::string& line) {
            using namespace StringTools;
            auto words = split(line, " ");
            words.erase(words.begin(), words.begin() + 1);
            return words;
        }

        inline static std::vector<std::string> commaSeparatedStyle(const std::string& line) {
            using namespace StringTools;
            auto words = split(line, ",");
            for (auto& word : words) word = lstrip(rstrip(word));
            return words;
        }

        inline static std::vector<double> extractLine(const std::string& line, long row) {
            auto vars = StringTools::split(line, ",");
            std::vector<double> row_data(vars.size());
            for (size_t i = 0; i < row_data.size(); i++) row_data[i] = StringTools::toDouble(vars[i]);
            return row_data;
        }
    };

    class CSVWriter {
      public:
        inline static void write(MessagePasser mp, std::string filename, const DataFrame& df) {
            auto s = dataFrameToString(df);
            FILE* f = nullptr;
            if (mp.Rank() == 0) {
                f = openOrThrow(filename);
                writeCSVHeader(f, df);
                writeCSVBody(f, df);
            }
            for (int rank = 1; rank < mp.NumberOfProcesses(); rank++) {
                if (rank == mp.Rank()) {
                    MessagePasser::Message msg(s);
                    mp.NonBlockingSend(std::move(msg), 0).wait();
                }
                if (mp.Rank() == 0) {
                    MessagePasser::Message rcv;
                    mp.Recv(rcv, rank);
                    std::string frame_as_string;
                    rcv.unpack(frame_as_string);
                    auto frame = CSVReader::toDataFrame(frame_as_string);
                    append(f, frame);
                }
                mp.Barrier();
            }
            if (0 == mp.Rank()) {
                fclose(f);
            }
        }

        inline static void write(std::string filename, const DataFrame& data_frame) {
            filename = StringTools::addExtension(filename, "csv");
            FILE* fp = openOrThrow(filename);
            writeCSVHeader(fp, data_frame);
            writeCSVBody(fp, data_frame);
            fclose(fp);
        }
        inline static void append(FILE* fp, const DataFrame& data_frame) { writeCSVBody(fp, data_frame); }
        inline static void writeCSVHeader(FILE*& fp, const DataFrame& data_frame) {
            fprintf(fp, "%s\n", headerToString(data_frame).c_str());
        }
        inline static void writeCSVBody(FILE*& fp, const DataFrame& data_frame) {
            auto names = data_frame.columns();
            auto num_rows = data_frame.shape()[0];
            for (int r = 0; r < num_rows; r++) fprintf(fp, "%s\n", rowToString(data_frame, r).c_str());
        }

      public:
        inline static std::string headerToString(const DataFrame& df) {
            auto names = df.columns();
            return Parfait::StringTools::join(names, ",");
        }
        inline static std::string rowToString(const DataFrame& df, int r) {
            std::ostringstream row;
            auto names = df.columns();
            row << std::setprecision(15);
            row << df[names.front()][r];
            for (size_t n = 1; n < names.size(); ++n) {
                row << "," << df[names.at(n)][r];
            }
            return row.str();
        }
        inline static std::string dataFrameToString(const DataFrame& df) {
            auto s = headerToString(df) + "\n";
            auto num_rows = df.shape()[0];
            for (int i = 0; i < num_rows; i++) {
                s.append(rowToString(df, i));
                s.append("\n");
            }
            s.resize(s.size() - 1);
            return s;
        }
    };

    class TecplotWriter {
      public:
        inline static void write(std::string filename,
                                 const DataFrame& data_frame,
                                 const std::string& title = "Powered by Parfait") {
            filename = StringTools::addExtension(filename, "dat");
            auto zone_name = StringTools::stripExtension(filename);
            FILE* fp = openOrThrow(filename);

            writeTecplotLineplotHeader(fp, title, zone_name, data_frame);
            writeTecplotLineplotBody(fp, data_frame);

            fclose(fp);
        }
        inline static void append(std::string filename, const DataFrame& data_frame) {
            filename = StringTools::addExtension(filename, "dat");
            auto zone_name = StringTools::stripExtension(filename);
            FILE* fp = fopen(filename.c_str(), "a");
            if (not FileTools::doesFileExist(filename)) {
                fp = openOrThrow(filename);
                writeTecplotLineplotHeader(fp, "Powered By Parfait", zone_name, data_frame);
            } else {
                fp = fopen(filename.c_str(), "a");
            }
            writeTecplotLineplotBody(fp, data_frame);
            fclose(fp);
        }
        inline static void writeTecplotLineplotHeader(FILE*& fp,
                                                      const std::string& title,
                                                      const std::string& zone_name,
                                                      const DataFrame& data_frame) {
            auto names = data_frame.columns();
            fprintf(fp, "TITLE=\"%s\"\n", title.c_str());
            fprintf(fp, "VARIABLES=");
            std::vector<std::string> variable_header = names;
            for (auto& n : variable_header) {
                n = "\"" + n + "\"";
            }
            fprintf(fp, "%s\n", StringTools::join(variable_header, ", ").c_str());

            fprintf(fp, "ZONE T=\"%s\"\n", zone_name.c_str());
        }
        inline static void writeTecplotLineplotBody(FILE*& fp, const DataFrame& data_frame) {
            auto names = data_frame.columns();
            auto num_rows = data_frame.shape()[0];
            for (int r = 0; r < num_rows; r++) {
                std::vector<std::string> row;
                for (auto n : names) {
                    double d = data_frame[n][r];
                    row.push_back(std::to_string(d));
                }
                fprintf(fp, "%s\n", StringTools::join(row, " ").c_str());
            }
        }
    };

}

inline void writeCSV(const std::string& filename, const DataFrame& data_frame) {
    LinePlotters::CSVWriter::write(filename, data_frame);
}
inline void appendCSV(const std::string& filename, const DataFrame& data_frame) {
    auto fname = StringTools::addExtension(filename, "csv");
    FILE* fp;
    if (FileTools::doesFileExist(filename)) {
        fp = fopen(filename.c_str(), "a");
    } else {
        fp = LinePlotters::openOrThrow(filename);
        LinePlotters::CSVWriter::writeCSVHeader(fp, data_frame);
    }
    LinePlotters::CSVWriter::append(fp, data_frame);
    fclose(fp);
}
class TecplotDataFrameReader {
  public:
    inline TecplotDataFrameReader(std::string filename) {
        using namespace Parfait::StringTools;
        auto lines = split(Parfait::FileTools::loadFileToString(filename), "\n");
        int line_index = 0;
        parseColumnNames(line_index, lines);
        skipUntilWeFindData(line_index, lines);
        readData(line_index, lines);
    }

    inline Parfait::DataFrame exportToFrame() {
        Parfait::DataFrame df;
        for (int i = 0; i < int(column_names.size()); i++) {
            auto column_name = column_names[i];
            df[column_name] = data[i];
        }
        return df;
    }

  public:
    std::vector<std::string> column_names;
    std::vector<std::vector<double>> data;

    inline void readData(int& line_index, std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        data.resize(column_names.size());
        for (; line_index < int(lines.size()); line_index++) {
            auto words = split(strip(lines[line_index]), " ");
            PARFAIT_ASSERT(words.size() == column_names.size(), "parsed row with row length mismatch");
            for (int i = 0; i < int(words.size()); i++) {
                auto d = toDouble(words[i]);
                data[i].push_back(d);
            }
        }
    }

    inline void skipUntilWeFindData(int& line_index, std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        std::string w;
        do {
            w = split(strip(lines[line_index++]), " ")[0];
        } while (not isDouble(w));

        // once we find the data, backup one line.
        line_index--;
    }
    inline void parseColumnNames(int& line_index, std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        // get first variable, or variables in the first line
        while (not startsWith(strip(lines[line_index]), "VARIABLES")) {
            line_index++;
        }
        lines[line_index] = findAndReplace(lines[line_index], "=", " ");
        lines[line_index] = findAndReplace(lines[line_index], ",", " ");
        auto words = split(lines[line_index], " ");
        // strip the VARIABLES word
        words.erase(words.begin());
        // and the equals character
        if (words[0] == "=") {
            words.erase(words.begin());
        }
        line_index++;
        // get variables on following lines
        while (not startsWith(strip(lines[line_index]), "ZONE")) {
            lines[line_index] = findAndReplace(lines[line_index], "=", " ");
            lines[line_index] = findAndReplace(lines[line_index], ",", " ");
            auto new_words = split(lines[line_index], " ");
            words.insert(words.end(), new_words.begin(), new_words.end());
            line_index++;
        }
        // proces the variables and add to column names
        for (auto w : words) {
            w = stripFromBeginning(w, "\"");
            w.pop_back();
            column_names.push_back(w);
        }
        for (auto c : column_names) std::cout << c << '\n';
    }
};
inline DataFrame readFile(const std::string& filename) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "dat" or extension == "prf") {
        return readTecplot(filename);
    } else {
        return readCSV(filename);
    }
}
inline DataFrame readCSV(const std::string& filename) { return LinePlotters::CSVReader::read(filename); }
inline DataFrame readTecplot(const std::string& filename) {
    TecplotDataFrameReader reader(filename);
    return reader.exportToFrame();
}

inline void writeTecplot(const std::string& filename, const DataFrame& data_frame, const std::string& title) {
    LinePlotters::TecplotWriter::write(filename, data_frame, title);
}
inline void appendTecplot(const std::string& filename, const DataFrame& data_frame) {
    LinePlotters::TecplotWriter::append(filename, data_frame);
}
}

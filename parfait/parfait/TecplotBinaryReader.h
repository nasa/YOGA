#pragma once
#include <string>
#include <stdio.h>
#include "MapTools.h"
#include "TecplotBinaryHelpers.h"

namespace Parfait {
namespace tecplot {
    class BinaryReader {
      public:
        enum { DOUBLE = 2, FLOAT = 1 };
        BinaryReader(std::string filename) : filename(filename) {}

        struct HeaderInfo {
            std::vector<binary_helpers::ZoneHeaderInfo> zones;
            std::string title;
            std::vector<std::string> variable_names;
        };

        std::vector<Parfait::Point<double>> getPoints() {
            std::vector<Parfait::Point<double>> points;
            points.reserve(num_volume_points);
            auto& x_array = fields_including_xyz.at("X");
            auto& y_array = fields_including_xyz.at("Y");
            auto& z_array = fields_including_xyz.at("Z");

            for (int i = 0; i < num_volume_points; i++) {
                Parfait::Point<double> p{x_array[i], y_array[i], z_array[i]};
                points.push_back(p);
            }
            return points;
        }

        std::vector<std::array<int, 8>> getCells() { return cells; }

        std::vector<double> getField(std::string name) { return fields_including_xyz.at(name); }

        std::array<double, 2> getRangeOfField(std::string field) { return field_ranges.at(field); }

        HeaderInfo readHeader() {
            std::string magic_number;
            magic_number.resize(8);
            fread(&magic_number[0], sizeof(char), 8, fp);

            int one;
            fread(&one, sizeof(int), 1, fp);
            check(one == 1, "Check endian ordering, one is " + std::to_string(one));

            int file_type;
            fread(&file_type, sizeof(int), 1, fp);

            std::string title = readString();

            int num_variables = 0;
            fread(&num_variables, sizeof(int), 1, fp);

            printf("Num Variables %d\n", num_variables);
            std::vector<std::string> var_names(num_variables);
            for (int i = 0; i < num_variables; i++) {
                var_names[i] = readString();
                printf("Variable %d: %s\n", i, var_names[i].c_str());
            }

            float zone_marker;
            fread(&zone_marker, sizeof(float), 1, fp);

            HeaderInfo header_info;
            header_info.title = title;
            header_info.variable_names = var_names;

            while (zone_marker == 299.0) {
                check(zone_marker == 299.0,
                      "Encountered unexpected zone marked, expected 299.0, found: " + std::to_string(zone_marker));

                std::string zone_name = readString();
                binary_helpers::ZoneHeaderInfo zone_header_info;
                if (zone_name == "Volume") zone_header_info.type = binary_helpers::ZoneHeaderInfo::VOLUME;
                if (zone_name == "Surface") zone_header_info.type = binary_helpers::ZoneHeaderInfo::SURFACE;

                int parent_zone, strand_id;
                fread(&parent_zone, sizeof(int), 1, fp);
                fread(&strand_id, sizeof(int), 1, fp);

                double solution_time;
                fread(&solution_time, sizeof(double), 1, fp);

                int unused = readInt();
                check(unused == -1, "Unused expected to be -1 was: " + std::to_string(unused));

                int FEBRICK = 5;
                int FEQUADRILATERAL = 3;
                int zone_type = readInt();
                check(zone_type == FEBRICK or zone_type == FEQUADRILATERAL,
                      "Expected to find FEBRICK or FEQUADRILATERAL zone type, found: " + std::to_string(zone_type));

                // There is some question about the order of the next 3ish items.
                // The tecplot 360_data_format_guide.pdf:
                // http://home.ustc.edu.cn/~cbq/360_data_format_guide.pdf
                // says it should be:
                // data packing
                // specifying data location
                // -- optionally specify location
                // specifying face neighbors

                // But O'Connell suspects based on exploring preplot binary files:
                // specifying data location
                // -- optionally specify location
                // data packing
                // specifying face neighbors/
                int specifying_data_location = readInt();
                check(specifying_data_location == 1 or specifying_data_location == 2,
                      "Expected to not be specifying where data is located, but found: " +
                          std::to_string(specifying_data_location));
                if (specifying_data_location == 1) {
                    for (int i = 0; i < num_variables; i++) {
                        int location = readInt();
                        // 0 is at nodes
                        // 1 is cells
                        check(location == 0 or location == 1, "Unexpected data location: " + std::to_string(location));
                        printf("Zone %s, Variable %s is at %d\n", zone_name.c_str(), var_names.at(i).c_str(), location);
                    }
                }
                int data_packing = readInt();
                printf("data packing %d\n", data_packing);
                check(data_packing == 0,
                      "Expected data packing to be 0 meaning block packing.  Found: " + std::to_string(data_packing));
                int specifying_face_neighbors = readInt();
                check(specifying_face_neighbors == 0,
                      "Expected to not be specifying face neighbors, but found: " +
                          std::to_string(specifying_face_neighbors));

                int num_points = readInt();

                int num_elements = readInt();

                zone_header_info.num_elements = num_elements;
                zone_header_info.num_points = num_points;

                for (int i = 0; i < 3; i++) {
                    int z = readInt();
                    check(z == 0, "expected zero found: " + std::to_string(z));
                }
                int more_aux_data = readInt();
                check(more_aux_data == 0, "Expected no more aux data, found: " + std::to_string(more_aux_data));

                header_info.zones.push_back(zone_header_info);

                zone_marker = readFloat();
            }

            check(zone_marker == 357.0, "Expected separating marker 357.0, found: " + std::to_string(zone_marker));

            return header_info;
        }

        void printHeader(const HeaderInfo& header_info) {
            printf("Reading file %s\n", filename.c_str());
            printf("Num variables: %ld\n", header_info.variable_names.size());
            for (auto& var_name : header_info.variable_names) {
                printf("  %s\n", var_name.c_str());
            }
            printf("Num Zones: %lu\n", header_info.zones.size());
            for (auto z : header_info.zones) {
                printf("Num elements:  %d\n", z.num_elements);
                printf("Num points:    %d\n", z.num_points);
            }
        }
        void read() {
            open();
            auto header_info = readHeader();
            num_volume_elements = header_info.zones.front().num_elements;
            num_volume_points = header_info.zones.front().num_points;
            num_fields_including_xyz = int(header_info.variable_names.size());
            variable_names = header_info.variable_names;
            printHeader(header_info);

            readData();

            fclose(fp);
        }
        void readNodeFields(const std::vector<int>& var_types) {
            for (int i = 0; i < num_fields_including_xyz; i++) {
                double min = readDouble();
                double max = readDouble();
                field_ranges[variable_names[i]] = std::array<double, 2>{min, max};
                printf("Variable %s range: %e %e\n", variable_names[i].c_str(), min, max);
            }

            for (int i = 0; i < num_fields_including_xyz; i++) {
                std::string var_name = variable_names[i];
                if (var_types[i] == DOUBLE) {
                    fields_including_xyz[var_name] = readDoubles(num_volume_points);
                } else if (var_types[i] == FLOAT) {
                    fields_including_xyz[var_name] = convertToDoubles(readFloats(num_volume_points));
                } else {
                    PARFAIT_WARNING("Variable: " + var_name +
                                    " has unknown data type: " + std::to_string(var_types[i]));
                }
            }
        }
        void readElementConnectivity() {
            cells.resize(num_volume_elements);
            for (int c = 0; c < num_volume_elements; c++) {
                readInts(cells[c].data(), 8);
            }
        }
        void readData() {
            float zone_marker = readFloat();
            check(zone_marker == 299.0, "Expected zone marker 299.0, found: " + std::to_string(zone_marker));

            std::vector<int> var_types = readInts(num_fields_including_xyz);
            for (int i = 0; i < num_fields_including_xyz; i++) {
                check(var_types[i] == DOUBLE or var_types[i] == FLOAT,
                      "Expected to read all data as float or doubles, But found: " + std::to_string(var_types[i]));
            }

            int has_passive_variables = readInt();
            check(has_passive_variables == 0,
                  "Expected to not have passive variables, but: " + std::to_string(has_passive_variables));

            int has_variable_sharing = readInt();
            check(has_variable_sharing == 0,
                  "Expected to not have variable sharing, but: " + std::to_string(has_variable_sharing));

            int zone_sharing = readInt();
            check(zone_sharing == -1, "Expected to not share zones, but: " + std::to_string(zone_sharing));

            readNodeFields(var_types);

            readElementConnectivity();
        }

      private:
        std::string filename;
        FILE* fp;
        int num_volume_points;
        int num_volume_elements;
        int num_fields_including_xyz;
        std::vector<std::string> variable_names;
        std::map<std::string, std::vector<double>> fields_including_xyz;
        std::map<std::string, std::array<double, 2>> field_ranges;
        std::vector<std::array<int, 8>> cells;
        void open() {
            fp = fopen(filename.c_str(), "r");
            if (fp == nullptr) {
                PARFAIT_WARNING("Could not open file for reading: " + filename);
                PARFAIT_THROW("Could not open file for reading: " + filename);
            }
        }

        void check(bool thing, std::string message) {
            if (not thing) {
                PARFAIT_THROW(message);
            }
        }

        std::string readString() {
            int next_char = -1;
            std::string title;
            while (next_char != 0) {
                fread(&next_char, sizeof(int), 1, fp);
                if (next_char != 0) title.push_back(next_char);
            }
            return Parfait::StringTools::strip(title);
        }

        int readInt() {
            int i;
            fread(&i, sizeof(int), 1, fp);
            return i;
        }

        float readFloat() {
            float f;
            fread(&f, sizeof(float), 1, fp);
            return f;
        }

        float readDouble() {
            double f;
            fread(&f, sizeof(double), 1, fp);
            return f;
        }

        void readInts(int* ints, int num_to_read) { fread(ints, sizeof(int), num_to_read, fp); }

        std::vector<int> readInts(int num_to_read) {
            std::vector<int> out(num_to_read);
            fread(out.data(), sizeof(int), num_to_read, fp);
            return out;
        }

        std::vector<double> readDoubles(int num_to_read) {
            std::vector<double> out(num_to_read);
            fread(out.data(), sizeof(double), num_to_read, fp);
            return out;
        }

        std::vector<float> readFloats(int num_to_read) {
            std::vector<float> out(num_to_read);
            fread(out.data(), sizeof(float), num_to_read, fp);
            return out;
        }

        std::vector<double> convertToDoubles(const std::vector<float>& floats) {
            std::vector<double> out;
            out.reserve(floats.size());
            for (auto f : floats) {
                out.push_back(double(f));
            }
            return out;
        }
    };
}
}

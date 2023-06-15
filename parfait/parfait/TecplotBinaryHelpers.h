#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <parfait/Throw.h>

namespace Parfait {
namespace tecplot {
    enum VariableLocation { NODE = 0, CELL = 1, INVALID = -1 };
    namespace binary_helpers {
        struct ZoneHeaderInfo {
            enum Type { VOLUME, SURFACE };

            int num_points;
            int num_elements;
            Type type;
            std::string name;
        };
        inline void writeString(FILE* fp, std::string s) {
            for (char i : s) {
                int character = int(i);
                fwrite(&character, sizeof(int), 1, fp);
            }
            int zero = 0;
            fwrite(&zero, sizeof(int), 1, fp);
        }

        inline void writeHeader(FILE* fp,
                                int num_variables,
                                const std::vector<std::string>& node_varible_names,
                                const std::vector<VariableLocation>& variable_locations,
                                const std::vector<ZoneHeaderInfo>& zone_header_info,
                                double solution_time = 0.0) {
            std::string magic_number = "#!TDV112";
            fwrite(magic_number.c_str(), sizeof(char), 8, fp);
            int one = 1;
            fwrite(&one, sizeof(one), 1, fp);
            int FULL = 0;
            fwrite(&FULL, sizeof(FULL), 1, fp);
            std::string title = "Powered by T-infinity";
            writeString(fp, title);

            fwrite(&num_variables, sizeof(int), 1, fp);
            writeString(fp, "X");
            writeString(fp, "Y");
            writeString(fp, "Z");
            for (auto& name : node_varible_names) writeString(fp, name);

            int FEBRICK = 5;
            int FEQUADRILATERAL = 3;
            int element_type;

            for (auto& zone : zone_header_info) {
                switch (zone.type) {
                    case ZoneHeaderInfo::VOLUME:
                        element_type = FEBRICK;
                        break;
                    case ZoneHeaderInfo::SURFACE:
                        element_type = FEQUADRILATERAL;
                        break;
                    default:
                        PARFAIT_THROW("Could not write tecplot zone of unknown type: " + std::to_string(zone.type));
                }
                // write zone markers
                float zone_marker = 299.0;
                fwrite(&zone_marker, sizeof(float), 1, fp);

                writeString(fp, zone.name);
                int parent_zone = -1;
                int strand_id = -2;
                fwrite(&parent_zone, sizeof(int), 1, fp);
                fwrite(&strand_id, sizeof(int), 1, fp);

                fwrite(&solution_time, sizeof(double), 1, fp);

                int unused = -1;
                fwrite(&unused, sizeof(int), 1, fp);

                fwrite(&element_type, sizeof(int), 1, fp);

                // This next section is assuming a bug in the tecplot data format guide for V112
                // The guide says data packing comes before specifying variable locations
                // But through experimentation MDO has determined it does not.
                // He does not know _what_ the correct layout is after specifying field locations
                // Because all the integers after are all just zero since we don't yet support those features.

                // Tecplot supports having some zones with only a subset of the variables
                // The missing variables are called passive variables.  We don't support them
                // Tecplot allows for each variable to be a node in one zone but a cell in another
                // We don't support this.
                int IM_GONNA_TELL_YOU_WHERE_THE_FIELDS_LIVE = 1;
                int data_location = IM_GONNA_TELL_YOU_WHERE_THE_FIELDS_LIVE;
                fwrite(&data_location, sizeof(int), 1, fp);

                int LIVES_AT_NODES = 0;
                data_location = LIVES_AT_NODES;
                fwrite(&data_location, sizeof(int), 1, fp);  // x
                fwrite(&data_location, sizeof(int), 1, fp);  // y
                fwrite(&data_location, sizeof(int), 1, fp);  // z
                for (int i = 0; i < int(variable_locations.size()); i++) {
                    data_location = variable_locations[i];
                    fwrite(&data_location, sizeof(int), 1, fp);
                }

                int BLOCK = 0;
                int data_packing = BLOCK;
                fwrite(&data_packing, sizeof(int), 1, fp);

                int NOT_TELLING_YOU_FACE_NEIGHBORS = 0;  // figure it out your self.
                fwrite(&NOT_TELLING_YOU_FACE_NEIGHBORS, sizeof(int), 1, fp);

                fwrite(&zone.num_points, sizeof(int), 1, fp);
                fwrite(&zone.num_elements, sizeof(int), 1, fp);

                int zero = 0;
                fwrite(&zero, sizeof(int), 1, fp);
                fwrite(&zero, sizeof(int), 1, fp);
                fwrite(&zero, sizeof(int), 1, fp);

                int no_more_data = 0;
                fwrite(&no_more_data, sizeof(int), 1, fp);
            }

            float separating_marker = 357.0;
            fwrite(&separating_marker, sizeof(float), 1, fp);
        }

    }

}
}

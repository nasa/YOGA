#pragma once
#include <parfait/Throw.h>
#include <tinf_solution.h>
#include <t-infinity/FieldInterface.h>
#include <memory>
#include <vector>
#include "VectorFieldAdapter.h"

inline std::vector<std::shared_ptr<inf::FieldInterface>> extractFromCSolution(
    void* solution_interface, int num_nodes) {
    std::vector<std::shared_ptr<inf::FieldInterface>> fields;

    int64_t num_fields = 0;
    char** names;
    TINF_DATA_TYPE* dtype = nullptr;
    int32_t err = tinf_solution_get_nodal_output_names(solution_interface,
                                                       &num_fields,
                                                       (const char***)&names,
                                                       (const enum TINF_DATA_TYPE**)&dtype);
    if (err != TINF_SUCCESS) PARFAIT_THROW("tinf_solution died.");
    printf("inf:: received %d fields from fun3d.\n", static_cast<int>(num_fields));
    fflush(stdout);
    printf("inf:: fields are: \n");
    fflush(stdout);
    for (int i = 0; i < num_fields; i++) {
        printf("inf::  <%s>\n", names[i]);
        fflush(stdout);
    }

    for (int i = 0; i < num_fields; i++) {
        std::vector<double> field(num_nodes);
        for (int n = 0; n < num_nodes; n++) {
            double d = 0.0;
            err = tinf_solution_get_outputs_at_nodes(
                solution_interface, TINF_DOUBLE, n, 1, 1, (const char**)&names[i], &d);
            if (err != TINF_SUCCESS) PARFAIT_THROW("tinf_solution died.");
            field[n] = d;
        }

        fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            std::string(names[i]), inf::FieldAttributes::Node(), 1, field));
    }

    return fields;
}

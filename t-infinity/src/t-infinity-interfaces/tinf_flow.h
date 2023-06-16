#pragma once
#ifndef TINF_FLOW_H
#define TINF_FLOW_H

#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  int32_t flow_create(void** tinf_flow, void* comm);
  int32_t flow_destroy(void** tinf_flow);
  double flow_mach_number(void* context);
  double flow_reynolds_number_per_meter(void* context);
  double flow_reference_temperature(void* context);
  double flow_temperature_kelvin(void* context);
  double flow_gamma(void* context);
  double flow_prandtl_number(void* context);
  int32_t flow_first_order_iterations(void* context);
  int32_t flow_flux_limiter(void* context);
  int32_t flow_viscosity_flag(void* context);
  bool flow_is_two_dimensional(void* context);
  bool flow_has_mixed_elements(void* context);
  double flow_angle_of_attack(void* context);
  double flow_sideslip_angle(void* context);
  double flow_yaw_angle(void* context);
  double flow_density(void* context);
  double flow_u_velocity(void* context);
  double flow_v_velocity(void* context);
  double flow_w_velocity(void* context);
  double flow_pressure(void* context);
  double flow_total_energy(void* context);
  double flow_back_pressure(void* context);
  double flow_adiabatic_wall_temp(void* context);
  bool flow_alternate_freestream(void* context);
  double flow_vinf_ratio(void* context);
  int32_t flow_number_of_initialization_volumes(void* context);
  void flow_get_initialization_volume(void* context, int32_t *vol, int32_t *vtype, double vdata[8], int32_t state_cnt, double state[]);

/* Notes 4/4/19
   Key-value approach
   bool do_you_know_about(void* context, char* variable_keyword);
   bool return_boolean_value(void* context, char* variable_keyword);
   int32t return_int32t_value(void* context, char* variable_keyword);
   double return_double_void* context, char* variable_keyword); */

#ifdef __cplusplus
}
#endif

#endif

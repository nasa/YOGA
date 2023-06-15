#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void tracer_initialize(int process_id);
void tracer_initialize_with_ranks_to_trace(int process_id, int* ranks_to_trace, int num_ranks_to_trace);
int tracer_is_initialized();
void tracer_flush();
int tracer_get_memory_in_mb();
void tracer_trace_memory();
void tracer_begin(const char* event_name);
void tracer_end(const char* event_name);
void tracer_finalize();

#ifdef __cplusplus
}
#endif

#include <tracer_c_interface.h>
#include <Tracer.h>

void tracer_initialize(int process_id) {
    Tracer::initialize(process_id);
}
void tracer_initialize_with_ranks_to_trace(int process_id, int* ranks_to_trace, int num_ranks_to_trace){
    std::set<int> ranks_to_trace_set;
    for(int i = 0; i < num_ranks_to_trace; i++){
        ranks_to_trace_set.insert(ranks_to_trace[i]);
    }

    Tracer::initialize("trace", process_id, ranks_to_trace_set);
}
int tracer_is_initialized(){
    return Tracer::isInitialized();
}
void tracer_begin(const char* event_name){
    Tracer::begin(event_name);
}

void tracer_end(const char *event_name) {
    Tracer::end(event_name);
}

int tracer_get_memory_in_mb(){
    return Tracer::usedMemoryMB();
}

void tracer_trace_memory(){
    Tracer::traceMemory();
}

void tracer_flush() {
    Tracer::flush();
}

void tracer_finalize(){
    Tracer::finalize();
}


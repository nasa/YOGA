#include <tracer_c_interface.h>

int main(){
    int process_id = 0;
    tracer_initialize(process_id);
    tracer_begin("Outer loop");
    tracer_begin("another event");
    tracer_end("another event");
    tracer_end("Outer loop");
    return 0;
}

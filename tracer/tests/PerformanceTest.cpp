#include <Tracer.h>

int main(){

    for(int i = 0; i < 1e5; i++){
        Tracer::begin("fast loop");
        Tracer::end("fast loop");
    }

    Tracer::setDebug();

    for(int i = 0; i < 1e5; i++){
        Tracer::begin("debug loop");
        Tracer::end("debug loop");
    }
}

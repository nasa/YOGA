#pragma once
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <utility>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

class Tracer {
  public:
    static void initialize(std::string filename_base = "trace");
    static void initialize(int process_id);
    static void initialize(std::string filename_base, int process_id, bool enabled = true);
    template <typename Container>
    static void initialize(std::string filename_base, int process_id, Container ranks_to_trace);
    static void finalize();
    static bool isInitialized();
    static bool isEnabled();
    static void setFileName(std::string name);
    static void setThreadName(std::string name);
    static void setFast();
    static void setDebug();
    static void flush();
    static void skipClosingBrace();
    static void counter(std::string name, long n);
    static void counter(std::string name, const std::map<std::string, long>& things);
    static void log(std::string name, std::string category = "DEFAULT");
    static void begin(std::string name, std::string category = "DEFAULT");
    static void end(std::string name, std::string category = "DEFAULT");
    static void beginFlowEvent(int id);
    static void endFlowEvent(int id);
    static size_t usedMemoryMB();
    static double usedMemoryPercent();
    static size_t availableMemoryMB();
    static double availableMemoryPercent();
    static size_t totalMemoryMB();
    static void traceMemory();
    static void mark(std::string mark_name);

  public:
    static std::string& fileName();
    static int& processId();
};

struct TracerScopedEvent {
    TracerScopedEvent(const std::string& event_name);
    virtual ~TracerScopedEvent();
    std::string event_name;
};

#define TRACER_FUNCTION_BEGIN Tracer::begin(__FUNCTION__);
#define TRACER_FUNCTION_END Tracer::end(__FUNCTION__);
#define TRACER_PROFILE_SCOPE(event_name) TracerScopedEvent __tracer_event_wrapper(event_name);
#define TRACER_PROFILE_FUNCTION TRACER_PROFILE_SCOPE(__FUNCTION__)

template <typename Container>
void Tracer::initialize(std::string filename_base, int process_id, Container ranks_to_trace) {
    std::set<int> set_ranks_to_trace(ranks_to_trace.begin(), ranks_to_trace.end());
    bool enabled = set_ranks_to_trace.count(process_id) == 1;
    initialize(filename_base, process_id, enabled);
}

extern "C" {
int tracer_set_handle(void* handle);
int tracer_get_handle(void** handle_ptr);
void tracer_print_handle();
}

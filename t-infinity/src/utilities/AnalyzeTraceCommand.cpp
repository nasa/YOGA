#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <parfait/JsonParser.h>
#include <parfait/FileTools.h>
#include <string>
#include "AnalyzeTraceCommand.h"

using namespace inf;
using namespace Parfait;
using namespace std;
using namespace TraceExtractor;

class AnalyzTraceCommand : public SubCommand {
  public:
    string description() const override { return "Analyze trace files"; }

    CommandLineMenu menu() const override {
        CommandLineMenu m;
        m.addParameter(Alias::help(), Help::help(), false);
        m.addParameter(Alias::inputFile(), Help::inputFile(),true);
        m.addParameter({"load-balance"},"list of events to measure load imbalance",false);
        return m;
    }

    void run(CommandLineMenu m, MessagePasser mp) override {
        auto trace_filename = m.get(Alias::inputFile());
        auto events = getEventsAsDicts(trace_filename);

        if(m.has("load-balance"))
            printLoadImbalances(m.getAsVector("load-balance"), events);
    }

  private:

    void printLoadImbalances(const vector<string>& events_to_measure, const vector<Dictionary>& events){
        map<string,double> event_imbalances, event_wall_clock;
        for(auto e:events_to_measure) {
            auto costs = calcEventCosts(e, events);
            double max = calcMax(costs);
            double mean = calcSum(costs) / double(costs.size());
            double imbalance = max / mean;
            event_imbalances[e] = imbalance;
            event_wall_clock[e] = max * 10e-7;
        }

        for(auto e:events_to_measure)
            printf("Event: \"%s\" imbalance: %lf wall_clock %lf (s)\n", e.c_str(), event_imbalances[e], event_wall_clock[e]);
    }

    long calcMax(const vector<long>& costs){
        long m = 0;
        for(long c:costs)
            m = std::max(c,m);
        return m;
    }

    long calcSum(const vector<long>& costs){
        long sum = 0;
        for(long c:costs)
            sum += c;
        return sum;
    }

    vector<long> calcEventCosts(const string& name, const vector<Dictionary>& events){
        auto ranks = getProcessIds(events);
        vector<long> costs;
        for (int r : ranks)
            costs.push_back(calcEventCostForRank(name, events, r));
        return costs;
    }

    long calcEventCostForRank(const string& name, const vector<Dictionary>& events, int r) const {
        long cost = 0;
        long start = 0;
        long stop = 0;
        for (auto& e : events) {
            if (r == extractRank(e)) {
                if (isBegin(e) and name == extractName(e)) {
                    start = extractTime(e);
                } else if (isEnd(e) and name == extractName(e)) {
                    stop = extractTime(e);
                    cost += (stop - start);
                    stop = start = 0;
                }
            }
        }
        return cost;
    }

    vector<Dictionary> getEventsAsDicts(const string& filename){
        auto contents = FileTools::loadFileToString(filename);
        auto dict = JsonParser::parse(contents);
        return dict.asObjects();
    }

    set<string> getUniqueEventNames(const vector<Dictionary>& events){
        set<string> unique_event_names;
        for(auto& event:events)
            unique_event_names.insert(event.at("name").asString());
        return unique_event_names;
    }

    set<int> getProcessIds(const vector<Dictionary>& events){
        std::set<int> ranks;
        for(auto& e:events)
            ranks.insert(e.at("pid").asInt());
        return ranks;
    }
};

CREATE_INF_SUBCOMMAND(AnalyzTraceCommand)

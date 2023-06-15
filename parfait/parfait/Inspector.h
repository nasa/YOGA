#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Timing.h>
#include <algorithm>

namespace Parfait {

class Inspector {
  public:
    typedef std::function<void(const std::string&)> TraceClosure;
    Inspector() = delete;
    Inspector(MessagePasser mp, int root) : mp(mp), root(root), trace(false) {}
    Inspector(MessagePasser mp, int root, TraceClosure beginTrace, TraceClosure endTrace)
        : mp(mp), root(root), trace(true), begin_trace(beginTrace), end_trace(endTrace) {}

    void clear() {
        task_costs_per_rank.clear();
        local_time_point_begins.clear();
        local_time_point_ends.clear();
    }

    void begin(const std::string& s) {
        local_time_point_begins[s].push_back(Parfait::Now());
        if (trace) begin_trace(s);
    }
    void end(const std::string& s) {
        local_time_point_ends[s].push_back(Parfait::Now());
        if (trace) end_trace(s);
    }
    void collect() {
        assert(local_time_point_begins.size() == local_time_point_ends.size());
        size_t n = local_time_point_begins.size();
        mp.Broadcast(n, 0);
        if (n != local_time_point_begins.size()) {
            throw std::logic_error("Inspector: mismatched task names");
        }
        for (auto& pair : local_time_point_begins) {
            auto name = pair.first;
            auto cost_on_this_rank = costOnRank(name);
            task_costs_per_rank[name] = mp.Gather(cost_on_this_rank, root);
        }
    }

    template <typename T>
    void addExternalField(const std::string& name, const std::vector<T>& v) {
        if (static_cast<int>(v.size()) == mp.NumberOfProcesses())
            task_costs_per_rank[name] = std::vector<double>(v.begin(), v.end());
        else
            printf("Must have 1 entry per rank, skipping external field: %s\n", name.c_str());
    }

    double mean(const std::string& name) {
        auto& vec = task_costs_per_rank[name];
        double x = 0.0;
        for (double d : vec) x += d;
        if (not vec.empty()) x /= double(vec.size());
        return x;
    }

    double max(const std::string& name) {
        auto& vec = task_costs_per_rank[name];
        double x = -std::numeric_limits<double>::max();
        for (double d : vec) x = std::max(x, d);
        return x;
    }

    double costOnRank(const std::string& name) {
        auto time_points_begin = local_time_point_begins.at(name);
        auto time_points_end = local_time_point_ends.at(name);
        assert(time_points_begin.size() == time_points_end.size());
        double cost_on_this_rank = 0.0;
        for (size_t t = 0; t < time_points_begin.size(); ++t) {
            cost_on_this_rank += Parfait::elapsedTimeInSeconds(time_points_begin[t], time_points_end[t]);
        }
        return cost_on_this_rank;
    }
    const std::vector<double>& getCostsPerRank(const std::string& name) const { return task_costs_per_rank.at(name); }

    double imbalance(const std::string& name) { return max(name) / mean(name); }

    std::vector<std::string> names() {
        std::vector<std::string> vec;
        for (auto& pair : task_costs_per_rank) vec.push_back(pair.first);
        return vec;
    }

    std::string asCSV() {
        std::string csv = "";
        if (mp.Rank() == root) {
            bool prepend_comma = false;
            for (auto& pair : task_costs_per_rank) {
                if (prepend_comma)
                    csv += ", ";
                else
                    prepend_comma = true;
                csv += pair.first;
            }
            csv += "\n";
            prepend_comma = false;
            for (int i = 0; i < mp.NumberOfProcesses(); i++) {
                for (auto& pair : task_costs_per_rank) {
                    auto& vec = pair.second;
                    double cost = vec[i];
                    if (prepend_comma)
                        csv += ", ";
                    else
                        prepend_comma = true;
                    csv += std::to_string(cost);
                }
                prepend_comma = false;
                csv += "\n";
            }
        }
        return csv;
    }

  private:
    typedef Parfait::Clock::time_point TimePoint;
    MessagePasser mp;
    const int root;
    const bool trace;
    TraceClosure begin_trace;
    TraceClosure end_trace;
    std::map<std::string, std::vector<TimePoint>> local_time_point_begins;
    std::map<std::string, std::vector<TimePoint>> local_time_point_ends;
    std::map<std::string, std::vector<double>> task_costs_per_rank;
};
}

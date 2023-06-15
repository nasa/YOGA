#pragma once
#include <parfait/LinearPartitioner.h>
namespace Parfait {
class BarGenerators {
  public:
    class Utf8BarGenerator {
      public:
        std::string generate(double normalized_value) {
            int max_bucket = bar_values.size() - 1;
            int bucket = normalized_value * max_bucket;
            return bar_values[bucket];
        }

      private:
        const std::vector<std::string> bar_values{
            " ", u8"\u2581", u8"\u2582", u8"\u2583", u8"\u2584", u8"\u2585", u8"\u2586", u8"\u2587", u8"\u2588"};
    };

    class HorizontalBarGenerator {
      public:
        std::string generate(double normalized_value) {
            int max_bucket = max_width - 1;
            int bucket = normalized_value * max_bucket;
            std::string bar = left_bound_symbol;
            for (int i = 0; i < bucket; i++) bar.append(plot_symbol);
            bar.resize(max_width, ' ');
            bar.append(right_bound_symbol);
            bar.append("\n");
            return bar;
        }
        void setPlotSymbol(const std::string& symbol) { plot_symbol = symbol; }
        void setLeftBoundSymbol(const std::string& symbol) { left_bound_symbol = symbol; }
        void setRightBoundSymbol(const std::string& symbol) { right_bound_symbol = symbol; }
        void setMaxWidth(int n) { max_width = n; }

      private:
        std::string plot_symbol = u8"\u2584";
        std::string left_bound_symbol;
        std::string right_bound_symbol;
        int max_width = 30;
    };
};

class BarChart {
  public:
    template <typename T>
    BarChart(const std::vector<T>& v) : normalized_values(normalize(v, smallest(v), largest(v))) {}

    template <typename BarGenerator>
    std::string generateHistogram(int bar_count, BarGenerator generator) {
        BarChart chart(generateHistogramCounts(bar_count));
        return chart.generateBarChart(bar_count, generator);
    }

    template <typename BarGenerator>
    std::string generateBarChart(int bar_count, BarGenerator bar_generator) {
        std::vector<double> bucketed_values(bar_count, 0.0);
        std::vector<int> counts_per_bucket(bar_count, 0.0);
        for (int i = 0; i < int(normalized_values.size()); i++) {
            auto bucket_id = LinearPartitioner::getWorkerOfWorkItem(i, normalized_values.size(), bar_count);
            bucketed_values[bucket_id] += normalized_values[i];
            counts_per_bucket[bucket_id]++;
        }
        for (int i = 0; i < bar_count; i++) bucketed_values[i] /= std::max(1, counts_per_bucket[i]);
        std::string s;
        for (auto x : bucketed_values) s.append(bar_generator.generate(x));
        return s;
    }

    std::vector<int> generateHistogramCounts(int nbuckets) {
        std::vector<int> counts(nbuckets, 0);
        for (auto x : normalized_values) {
            int bucket = x * (nbuckets - 1);
            counts[bucket]++;
        }
        return counts;
    }

  private:
    std::vector<double> normalized_values;

    template <typename T>
    std::vector<double> normalize(const std::vector<T>& v, T lo, T hi) {
        std::vector<double> values(v.begin(), v.end());
        for (auto& x : values) x -= lo;
        hi -= lo;
        for (auto& x : values) x /= hi;
        return values;
    }

    template <typename T>
    T smallest(const std::vector<T>& v) {
        T x = v.front();
        for (auto val : v) x = std::min(x, val);
        return x;
    }

    template <typename T>
    T largest(const std::vector<T>& v) {
        T x = v.front();
        for (auto val : v) x = std::max(x, val);
        return x;
    }
};
}
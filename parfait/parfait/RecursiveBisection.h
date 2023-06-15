#pragma once
#include <parfait/ExtentBuilder.h>
#include <parfait/Linspace.h>
#include <parfait/ParallelExtent.h>
#include <parfait/PointWriter.h>
#include <parfait/PointGenerator.h>
#include <thread>
#include "Throw.h"

namespace Parfait {
double calcCutRatio(int num_partitions);
bool isWithinTol(double target_ratio, size_t left_count, size_t right_count, double tol);

template <size_t N>
double findCenter(Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol = 1e-8,
                  int iteration = 0);
template <size_t N>
double findCenter(Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  const std::vector<double>& weights,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol = 1e-8,
                  int iteration = 0);

template <size_t N>
double findCenter(MessagePasser mp,
                  Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol = 1e-8,
                  int iteration = 0);

template <size_t N>
double findCenter(MessagePasser mp,
                  Parfait::Extent<double, N> domain,
                  const std::vector<Parfait::Point<double, N>>& points,
                  const std::vector<double>& weights,
                  double target_left_percentage,
                  double center,
                  int dimension,
                  double tol = 1e-8,
                  int iteration = 0);

template <typename T>
std::tuple<std::map<T, T>, std::map<T, T>> collapseAndMap(const std::set<T>& a, const std::set<T>& b) {
    std::map<T, T> a_old_to_new, b_old_to_new;
    T next = 0;
    for (auto t : a) {
        a_old_to_new[t] = next++;
    }
    for (auto t : b) {
        b_old_to_new[t] = next++;
    }
    return {a_old_to_new, b_old_to_new};
}

template <size_t N>
std::vector<int> assignOnePerRank(MessagePasser mp, const std::vector<Parfait::Point<double, N>>& points);

template <size_t N>
std::vector<int> recursiveBisection_serial(std::vector<Parfait::Point<double, N>> points,
                                           const std::vector<double>& weights,
                                           unsigned int leaf_id,
                                           unsigned int depth,
                                           int num_partitions,
                                           double tol);

template <size_t N>
std::vector<int> recursiveBisection(const std::vector<Parfait::Point<double, N>>& points,
                                    int num_partitions,
                                    double tol);

template <size_t N>
std::vector<int> recursiveBisection(MessagePasser orig_mp,
                                    const std::vector<Parfait::Point<double, N>>& points,
                                    int num_partitions,
                                    double tol);

template <size_t N>
std::vector<int> recursiveBisection(MessagePasser orig_mp,
                                    std::vector<Parfait::Point<double, N>> points,
                                    std::vector<double> weights,
                                    int num_partitions,
                                    double tol);
}

#include "RecursiveBisection.hpp"
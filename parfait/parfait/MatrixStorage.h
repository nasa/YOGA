#pragma once
#include <vector>
#include "ParfaitMacros.h"

namespace Parfait {
const int Dynamic = -1;

template <typename T, int Rows, int Columns>
struct MatrixStorage {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return Rows; }
    PARFAIT_DEVICE constexpr int cols() const { return Columns; }
    PARFAIT_DEVICE T* data() { return m; }
    PARFAIT_DEVICE const T* data() const { return m; }
    T m[Rows * Columns];
};

#ifndef __CUDA_ARCH__
template <typename T, int Columns>
struct MatrixStorage<T, Dynamic, Columns> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) : _rows(rows), m(rows * Columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return _rows; }
    PARFAIT_DEVICE constexpr int cols() const { return Columns; }
    T* data() { return m.data(); }
    const T* data() const { return m.data(); }
    int _rows;
    std::vector<T> m;
};
template <typename T, int Rows>
struct MatrixStorage<T, Rows, Dynamic> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) : _columns(columns), m(Rows * columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return Rows; }
    PARFAIT_DEVICE constexpr int cols() const { return _columns; }
    T* data() { return m.data(); }
    const T* data() const { return m.data(); }
    int _columns;
    std::vector<T> m;
};
template <typename T>
struct MatrixStorage<T, Dynamic, Dynamic> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) : _rows(rows), _columns(columns), m(rows * columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return _rows; }
    PARFAIT_DEVICE constexpr int cols() const { return _columns; }
    int _rows;
    int _columns;
    std::vector<T> m;
};
#else
template <typename T, int Columns>
struct MatrixStorage<T, Dynamic, Columns> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return 0; }
    PARFAIT_DEVICE constexpr int cols() const { return 0; }
    PARFAIT_DEVICE T* data() { return nullptr; }
    PARFAIT_DEVICE const T* data() const { return nullptr; }
    T m[1];
};
template <typename T, int Rows>
struct MatrixStorage<T, Rows, Dynamic> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return 0; }
    PARFAIT_DEVICE constexpr int cols() const { return 0; }
    PARFAIT_DEVICE T* data() { return nullptr; }
    PARFAIT_DEVICE const T* data() const { return nullptr; }
    T m[1];
};
template <typename T>
struct MatrixStorage<T, Dynamic, Dynamic> {
    PARFAIT_DEVICE MatrixStorage(int rows, int columns) {}
    PARFAIT_DEVICE constexpr int rows() const { return 0; }
    PARFAIT_DEVICE constexpr int cols() const { return 0; }
    T m[1];
};
#endif

}

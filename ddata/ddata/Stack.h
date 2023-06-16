#pragma once

#include <vector>
#include <array>
#include <stack>
#include <ostream>

namespace Linearize {

class Stack {
  public:
    void beginRecording() {
        clearStacks();
        gradients.clear();
        addStatement(-1);
    }
    void reset() {
        clearStacks();
        gradients.clear();
        num_gradients = 0;
    }
    void clearStacks() {
        statements.lhs_gradient.clear();
        statements.operation.clear();
        operations.rhs_gradient.clear();
        operations.multiplier.clear();
    }
    void calcAdjoint(std::vector<double>& dx) const {
        if (statements.lhs_gradient.empty()) return;
        if (static_cast<int>(dx.size()) != num_gradients)
            throw std::runtime_error("gradients not setup before call to calcAdjoint");
        for (size_t s = statements.size() - 1; s > 0; s--) {
            double lhs_grad = dx[statements.lhs_gradient[s]];
            dx[statements.lhs_gradient[s]] = 0.0;
            if (lhs_grad != 0.0) {
                for (int i = statements.operation[s - 1]; i < statements.operation[s]; ++i) {
                    dx[operations.rhs_gradient[i]] += operations.multiplier[i] * lhs_grad;
                }
            }
        }
    }
    template <size_t N>
    void calcAdjoint(std::vector<std::array<double, N>>& dx) const {
        if (statements.lhs_gradient.empty()) return;
        if (static_cast<int>(dx.size()) != num_gradients)
            throw std::runtime_error("gradients not setup before call to calcAdjoint");
        for (size_t s = statements.size() - 1; s > 0; s--) {
            std::array<double, N> lhs_grad{};
            bool is_nonzero = false;
            for (size_t i = 0; i < N; ++i) {
                auto& grad = dx[statements.lhs_gradient[s]][i];
                if (grad != 0.0) {
                    is_nonzero = true;
                    lhs_grad[i] = grad;
                    grad = 0.0;
                }
            }
            if (is_nonzero) {
                for (int op = statements.operation[s - 1]; op < statements.operation[s]; ++op) {
                    for (size_t i = 0; i < N; ++i) {
                        dx[operations.rhs_gradient[op]][i] += operations.multiplier[op] * lhs_grad[i];
                    }
                }
            }
        }
    }

    template <typename IndependentIndicies, typename DependentIndicies>
    std::vector<double> calcJacobian(const IndependentIndicies& independent, const DependentIndicies& dependent) {
        std::vector<double> jacobian(independent.size() * dependent.size());
        calcJacobian(independent, dependent, [&](int row, int col) -> double& { return jacobian[row * 5 + col]; });
        return jacobian;
    }

    template <typename IndependentIndicies, typename DependentIndicies, typename JacobianSetter>
    void calcJacobian(const IndependentIndicies& independent,
                      const DependentIndicies& dependent,
                      JacobianSetter jacobian) {
        constexpr int PackSize = 4;

        auto num_rows = static_cast<int>(dependent.size());
        auto num_columns = static_cast<int>(independent.size());
        std::array<double, PackSize> zero{};
        std::vector<std::array<double, PackSize>> dx(num_gradients);
        for (int row = 0; row < num_rows / PackSize; ++row) {
            std::fill(dx.begin(), dx.end(), zero);
            for (int i = 0; i < PackSize; ++i) dx[dependent[row + i]][i] = 1.0;
            calcAdjoint(dx);
            for (int i = 0; i < PackSize; ++i) {
                for (int column = 0; column < num_columns; ++column) {
                    jacobian(row + i, column) = dx[independent[column]][i];
                }
            }
        }
        auto remainder = num_rows % PackSize;
        if (remainder != 0) {
            gradients.resize(num_gradients);
            for (int row = num_rows - remainder; row < num_rows; ++row) {
                std::fill(gradients.begin(), gradients.end(), 0.0);
                gradients[dependent[row]] = 1.0;
                calcAdjoint(gradients);
                for (int column = 0; column < num_columns; ++column) {
                    jacobian(row, column) = gradients[independent[column]];
                }
            }
        }
    }

    void calcAdjoint() { calcAdjoint(gradients); }

    int newGradient() {
        num_gradients++;
        return num_gradients - 1;
    }

    void releaseGradient(int gradient_index) {
        // TODO: Implement tracking released gradients to minimize gradient allocations
    }

    void addStatement(int gradient_index) {
        statements.lhs_gradient.emplace_back(gradient_index);
        statements.operation.emplace_back(operations.size());
    }

    void addOperation(double multiplier, int gradient_index) {
        operations.rhs_gradient.emplace_back(gradient_index);
        operations.multiplier.emplace_back(multiplier);
    }

    void print(std::ostream& os) const {
        os << "Number of Gradients: " << num_gradients << std::endl;
        os << std::endl;
        os << "Statements:" << std::endl;
        int i = 0;
        for (size_t s = 0; s < statements.lhs_gradient.size(); ++s) {
            if (statements.lhs_gradient.at(s) < 0) continue;
            os << "[" << i++ << "]  " << statements.lhs_gradient.at(s) << " (lhs) -> " << statements.operation.at(s)
               << " (op) " << std::endl;
        }
        os << std::endl;
        os << "Operations:" << std::endl;
        i = 0;
        for (size_t op = 0; op < operations.rhs_gradient.size(); ++op) {
            os << "[" << i++ << "]  " << operations.multiplier.at(op) << " (multiplier) -> "
               << operations.rhs_gradient.at(op) << " (rhs) " << std::endl;
        }
    }

    struct Statements {
        size_t size() const { return lhs_gradient.size(); }
        std::vector<int> lhs_gradient;
        std::vector<int> operation;
    };
    struct Operations {
        size_t size() const { return rhs_gradient.size(); }
        std::vector<int> rhs_gradient;
        std::vector<double> multiplier;
    };

    Statements statements;
    Operations operations;
    int num_gradients;
    std::vector<double> gradients;
};
inline std::ostream& operator<<(std::ostream& os, const Stack& stack) {
    stack.print(os);
    return os;
}

class ActiveStack {
  public:
    static Stack& get() {
        static Stack instance;
        return instance;
    }
    ActiveStack(ActiveStack const&) = delete;
    void operator=(ActiveStack const&) = delete;

  private:
    ActiveStack() = default;
};

}

#define ACTIVE_STACK Linearize::ActiveStack::get()

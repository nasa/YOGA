#include <RingAssertions.h>
#include <ddata/ETDStack.h>

using namespace Linearize;

template <typename T>
std::vector<T> vector_function(const std::vector<T>& ql, const std::vector<T>& qr) {
    std::vector<T> result;
    for (size_t eqn = 0; eqn < ql.size(); ++eqn) {
        result.push_back(2.1 * pow(ql[eqn], 2) + 3.1 * qr[eqn]);
    }
    return result;
}

TEST_CASE("Can compute jacobian") {
    ACTIVE_STACK.reset();
    std::vector<Var> ql = {1, 2, 3, 4, 5};
    std::vector<Var> qr = {6, 7, 8, 9, 10};
    ACTIVE_STACK.beginRecording();
    auto f = vector_function(ql, qr);
    std::vector<int> indep, dep;
    for (int eqn = 0; eqn < 5; ++eqn) {
        indep.push_back(ql[eqn].gradient_index);
        dep.push_back(f[eqn].gradient_index);
    }
    auto jacobian = ACTIVE_STACK.calcJacobian(indep, dep);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            INFO("row: " << row << " col: " << col);
            double expected = row == col ? 4.2 * ql[col].f() : 0.0;
            double actual = jacobian[row * 5 + col];
            REQUIRE(expected == Approx(actual).margin(1e-12));
        }
    }
}

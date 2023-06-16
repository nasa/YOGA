#include <RingAssertions.h>
#include <ddata/CustomAllocator.h>

TEST_CASE("Custom Allocator") {
    constexpr int n = 10;
    std::vector<double*> expected_ptrs(n);
    {
        std::vector<double, Linearize::MemoryPool<double>> vec(n);
        for (int i = 0; i < n; ++i) expected_ptrs[i] = &vec[i];
    }
    std::vector<double, Linearize::MemoryPool<double>> vec(n);
    for (int i = 0; i < n; ++i) REQUIRE(expected_ptrs[i] == &vec[i]);
}

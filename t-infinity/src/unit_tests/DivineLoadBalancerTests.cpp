#include <RingAssertions.h>
#include <t-infinity/DivineLoadBalancer.h>

TEST_CASE("Even split test") {
    std::vector<double> weights{50, 50};
    int nproc = 4;

    {
        DivineLoadBalancer load_balancer(0, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(1, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(2, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(3, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
}

TEST_CASE("Uneven split test") {
    std::vector<double> weights{0.8, 0.2};
    int nproc = 5;

    {
        DivineLoadBalancer load_balancer(0, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(1, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(2, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(3, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(4, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
}

TEST_CASE("Non normalized split test") {
    std::vector<double> weights{1, 1.1, 1};
    int nproc = 4;

    {
        DivineLoadBalancer load_balancer(0, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(1, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(2, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(3, nproc);
        REQUIRE(2 == load_balancer.getAssignedDomain(weights));
    }
}

TEST_CASE("Equal weights, uneven split", "[derp]") {
    std::vector<double> weights{1, 1, 1};
    int nproc = 4;

    {
        DivineLoadBalancer load_balancer(0, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(1, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(2, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
    {
        DivineLoadBalancer load_balancer(3, nproc);
        REQUIRE(2 == load_balancer.getAssignedDomain(weights));
    }
}

TEST_CASE("Single domain gets all the ranks") {
    std::vector<double> weights{1};
    int nproc = 3;
    DivineLoadBalancer load_balancer(0, nproc);
    REQUIRE(0 == load_balancer.getAssignedDomain(weights));
}

TEST_CASE("mismatched balance on lower number of cores") {
    std::vector<double> weights{3, 1};
    int nproc = 2;
    SECTION("rank 0 gets first domain") {
        int my_rank = 0;
        DivineLoadBalancer load_balancer(my_rank, nproc);
        REQUIRE(0 == load_balancer.getAssignedDomain(weights));
    }
    SECTION("rank 1 gets the second domain") {
        int my_rank = 1;
        DivineLoadBalancer load_balancer(my_rank, nproc);
        REQUIRE(1 == load_balancer.getAssignedDomain(weights));
    }
}

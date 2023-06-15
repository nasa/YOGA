
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space Administration. 
// No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
// 
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the 
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0. 
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed 
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.
#include <complex>
#include "../MessagePasser.h"
#include <RingAssertions.h>

TEST_CASE("Create user defined op on the fly"){
    MessagePasser mp(MPI_COMM_WORLD);
    int n = mp.Rank();

    SECTION("Custom min function from non-capturing lambda") {
        auto min_fun = [](int a, int b) {
            return std::min(a, b);
        };

        int min = mp.Reduce(n, min_fun);
        REQUIRE(0 == min);
    }

    SECTION("Custom function from capturing lambda") {
        double x = 10.0;
        auto max_of_values_less_than_x = [x](double a, double b) {
            double max = 0.0;
            if (a < x and b < x)
                max = std::max(a, b);
            else if (a < x)
                max = a;
            else if (b < x)
                max = b;
            return max;
        };

        double val = mp.Rank() + 9.8;

        double special_max = mp.Reduce(val, max_of_values_less_than_x);
        REQUIRE(9.8 == Approx(special_max));
    }
}

TEST_CASE("Redux on vector"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<int> some_numbers(4);
    std::fill(some_numbers.begin(),some_numbers.end(),mp.Rank());

    auto my_max_function = [](int a,int b){
        return std::max(a,b);
    };

    auto max_values = mp.Reduce(some_numbers,my_max_function);
    REQUIRE(4 == max_values.size());
    for(int m:max_values)
        REQUIRE((mp.NumberOfProcesses()-1) == m);
}

TEST_CASE("Parallel max of each element in a vector"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> vec(mp->NumberOfProcesses(),mp->Rank());
    std::vector<int> result = mp->ParallelMax(vec,root);
    if(mp->Rank() == root)
        for(int i=0;i<mp->NumberOfProcesses();i++)
            REQUIRE((mp->NumberOfProcesses()-1) == result[i]);
}

TEST_CASE("Parallel min of each element in a vector"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> vec(mp->NumberOfProcesses(),mp->Rank()+7);
    std::vector<int> result = mp->ParallelMin(vec,root);
    if(mp->Rank() == root)
        for(int i=0;i<mp->NumberOfProcesses();i++)
            REQUIRE(7 == result[i]);
}

TEST_CASE("Parallel max reduction for a single element"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = 0;
    int value = mp->Rank();
    value = mp->ParallelMax(value, root);
    if(mp->Rank() == root)
        REQUIRE((mp->NumberOfProcesses()-1 == value));
    else
        REQUIRE(value == mp->Rank());
}

TEST_CASE("Parallel min reduction for a single element"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = mp->NumberOfProcesses()-1;
    int value = mp->Rank();
    value = mp->ParallelMin(value, root);
    if(mp->Rank() == root)
        REQUIRE(0 == value);
    else
        REQUIRE(value == mp->Rank());
}

TEST_CASE("Parallel rank of max reduction for a single element"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int value = mp->Rank();
    int rank = mp->ParallelRankOfMax(value);
    REQUIRE((mp->NumberOfProcesses()-1 == rank));
}

TEST_CASE("Parallel min-all reduction for a single element"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int value = mp->Rank();
    value = mp->ParallelMin(value);
    REQUIRE(0 == value);
}

TEST_CASE("Parallel max-all reduction for a single element"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int value = mp->Rank();
    value = mp->ParallelMax(value);
    REQUIRE((mp->NumberOfProcesses()-1 == value));
}

TEST_CASE("Parallel sum of an integer"){
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    auto sum = mp->ParallelSum(1);
    REQUIRE(mp->NumberOfProcesses() == sum);
}

TEST_CASE("Parallel sum of a complex number"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::complex<double> d = {1.0, 1.1};
    auto sum = mp.ParallelSum(d);
    REQUIRE(sum.real() == mp.NumberOfProcesses());
    REQUIRE(sum.imag() == Approx(1.1*mp.NumberOfProcesses()));
}

TEST_CASE("Elemental max"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> vec(mp.NumberOfProcesses(),0);
    vec[mp.Rank()] = mp.Rank();
    auto result = mp.ElementalMax(vec);
    REQUIRE(mp.NumberOfProcesses() == int(result.size()));
    for(int i=0;i<mp.NumberOfProcesses();++i){
        REQUIRE(i == result[i]);
    }
}

TEST_CASE("Set reduction"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::set<int> my_set;
    my_set.insert(mp.Rank());
    auto combined_set = mp.ParallelUnion(my_set);
    REQUIRE(int(combined_set.size()) == mp.NumberOfProcesses());

}

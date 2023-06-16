#include <RingAssertions.h>
#include <complex>
#include <parfait/Point.h>
#include <DifferentiableDonorElement.h>
#include <ComplexDifferentiator.h>
#include <InterpolationTools.h>
#include <YogaTypeSupport.h>
#include <C_InterfaceHelpers.h>
#include <yoga_c_interface.h>

using namespace YOGA;

template <size_t N, template <size_t, typename> class Number>
int countNonZeros(const Number<N, double>& x) {
    int nnz = 0;
    for (size_t i = 0; i < N; ++i)
        if (x.dx(i) != 0) nnz++;
    return nnz;
}

template <typename T>
void calcConstantWeights(int n, const T* points, const T* query_point, T* weights) {
    double equal_weight = 1.0 / double(n);
    for (int i = 0; i < n; i++) weights[i] = equal_weight;
}

template <typename Element>
void checkThatQueryPointAndEachVertexHave1NonZeroDerivative(Element tet) {
    for (int i = 0; i < 3; i++) REQUIRE(1 == countNonZeros(tet.query_point[i]));
    for (auto& v : tet.vertices)
        for (int i = 0; i < 3; i++) REQUIRE(1 == countNonZeros(v[i]));
}

template <typename Element>
void checkPartialDerivativeMapping(Element element) {
    checkThatQueryPointAndEachVertexHave1NonZeroDerivative(element);
    int nvertices = element.VertexCount;
    for (int i = 0; i < nvertices; i++) {
        double deriv = Element::vertexPartialX(element.vertices[i][0], i);
        REQUIRE(1.0 == deriv);
        deriv = Element::vertexPartialY(element.vertices[i][1], i);
        REQUIRE(1.0 == deriv);
        deriv = Element::vertexPartialZ(element.vertices[i][2], i);
        REQUIRE(1.0 == deriv);
    }

    double deriv = Element::queryPointPartialX(element.query_point[0]);
    REQUIRE(1.0 == deriv);
    deriv = Element::queryPointPartialY(element.query_point[1]);
    REQUIRE(1.0 == deriv);
    deriv = Element::queryPointPartialZ(element.query_point[2]);
    REQUIRE(1.0 == deriv);
}

template <typename F1, typename F2, size_t N>
void checkDdataDerivativesAgainstComplex(F1 interpolate_1,
                                         F2 interpolate_2,
                                         std::array<Parfait::Point<double>, N> vertices,
                                         Parfait::Point<double> p) {
    constexpr int n_derivatives = (N + 1) * 3;
    auto element = DifferentiableDonorElementFactory::make(vertices, p);
    typedef decltype(element) Element;

    std::array<ADType<n_derivatives>, N> weights;
    interpolate_2(N, element.vertices.front().data(), element.query_point.data(), weights.data());

    for (size_t vertex = 0; vertex < N; vertex++) {
        ComplexDifferentiator<decltype(interpolate_1), N> differentiator(interpolate_1, vertices, p);

        auto dwdx = differentiator.dwdx(vertex);
        auto dwdy = differentiator.dwdy(vertex);
        auto dwdz = differentiator.dwdz(vertex);

        for (size_t i = 0; i < N; i++) {
            CHECK(dwdx[i] == Approx(Element::vertexPartialX(weights[i], vertex)).margin(1.e-12));
            CHECK(dwdy[i] == Approx(Element::vertexPartialY(weights[i], vertex)).margin(1.e-12));
            CHECK(dwdz[i] == Approx(Element::vertexPartialZ(weights[i], vertex)).margin(1.e-12));
        }
    }
}

template <typename Element>
void checkThatConstantFunctionHasZeroDerivatives(Element e) {
    std::array<YOGA::ADType<15>, e.VertexCount> weights;
    calcConstantWeights(4, e.vertices.front().data(), e.query_point.data(), weights.data());
    for (auto w : weights) REQUIRE(0 == countNonZeros(w));
}

template <size_t N>
void checkInverseDistanceDdataPartialsAgainstComplex(const std::array<Parfait::Point<double>, N>& vertices,
                                      const Parfait::Point<double>& query_point) {
    constexpr int n_derivatives = (N + 1) * 3;

    using ComplexInterpolator = FiniteElementInterpolation<std::complex<double>>;
    using DdataInterpolator = FiniteElementInterpolation<ADType<n_derivatives>>;

    auto interpolate_1 = &ComplexInterpolator::calcInverseDistanceWeights;
    auto interpolate_2 = &DdataInterpolator::calcInverseDistanceWeights;
    checkDdataDerivativesAgainstComplex(interpolate_1, interpolate_2, vertices, query_point);
}

template <size_t N>
void checkFiniteElementDdataPartialsAgainstComplex(const std::array<Parfait::Point<double>, N>& vertices,
                                                     const Parfait::Point<double>& query_point) {
    constexpr int n_derivatives = (N + 1) * 3;

    using ComplexInterpolator = FiniteElementInterpolation<std::complex<double>>;
    using DdataInterpolator = FiniteElementInterpolation<ADType<n_derivatives>>;

    auto interpolate_1 = &ComplexInterpolator::calcWeights;
    auto interpolate_2 = &DdataInterpolator::calcWeights;
    checkDdataDerivativesAgainstComplex(interpolate_1, interpolate_2, vertices, query_point);
}

template <size_t N>
void checkLeastSquaresDdataPartialsAgainstComplex(const std::array<Parfait::Point<double>, N>& vertices,
                                                   const Parfait::Point<double>& query_point) {
    constexpr int n_derivatives = (N + 1) * 3;

    using ComplexInterpolator = LeastSquaresInterpolation<std::complex<double>>;
    using DdataInterpolator = LeastSquaresInterpolation<ADType<n_derivatives>>;

    auto interpolate_1 = &ComplexInterpolator::calcWeights;
    auto interpolate_2 = &DdataInterpolator::calcWeights;
    checkDdataDerivativesAgainstComplex(interpolate_1, interpolate_2, vertices, query_point);
}

TEST_CASE("generate partial derivatives with Ddata") {
    SECTION("tetrahedron") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 4> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {0, 2, 0};
        vertices[3] = {0, 0, 2};

        auto tet = DifferentiableDonorElementFactory::make(vertices, query_point);

        checkPartialDerivativeMapping(tet);

        checkThatConstantFunctionHasZeroDerivatives(tet);

        checkInverseDistanceDdataPartialsAgainstComplex(vertices, query_point);
        checkFiniteElementDdataPartialsAgainstComplex(vertices, query_point);
        checkLeastSquaresDdataPartialsAgainstComplex(vertices, query_point);
    }

    SECTION("pyramid") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 5> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {2, 2, 0};
        vertices[3] = {0, 2, 0};
        vertices[4] = {0, 0, 2};

        auto pyramid = DifferentiableDonorElementFactory::make(vertices, query_point);

        checkPartialDerivativeMapping(pyramid);

        checkInverseDistanceDdataPartialsAgainstComplex(vertices, query_point);
        //checkFiniteElementDdataPartialsAgainstComplex(vertices, query_point);
        checkLeastSquaresDdataPartialsAgainstComplex(vertices, query_point);
    }

    SECTION("prism") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 6> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {0, 2, 0};
        vertices[3] = {0, 0, 1};
        vertices[4] = {2, 0, 1};
        vertices[5] = {0, 2, 1};

        auto prism = DifferentiableDonorElementFactory::make(vertices, query_point);

        checkPartialDerivativeMapping(prism);

        checkInverseDistanceDdataPartialsAgainstComplex(vertices, query_point);
        checkFiniteElementDdataPartialsAgainstComplex(vertices, query_point);
        checkLeastSquaresDdataPartialsAgainstComplex(vertices, query_point);
    }

    SECTION("hex") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 8> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {2, 2, 0};
        vertices[3] = {0, 2, 0};
        vertices[4] = {0, 0, 1};
        vertices[5] = {2, 0, 1};
        vertices[6] = {2, 2, 1};
        vertices[7] = {0, 2, 1};

        auto hex = DifferentiableDonorElementFactory::make(vertices, query_point);

        checkPartialDerivativeMapping(hex);

        checkInverseDistanceDdataPartialsAgainstComplex(vertices, query_point);
        checkFiniteElementDdataPartialsAgainstComplex(vertices, query_point);
        checkLeastSquaresDdataPartialsAgainstComplex(vertices, query_point);
    }
}



template<int NDonors>
void checkCInterfaceAgainstDdata(double* donors,double* query_point){
    constexpr int n_derivatives = (NDonors + 1) * 3;
    using DdataInterpolator = LeastSquaresInterpolation<ADType<n_derivatives>>;
    auto element = DifferentiableDonorElement<n_derivatives>(donors,query_point, NDonors);
    std::vector<ADType<n_derivatives>> weights(NDonors);
    DdataInterpolator::calcWeights(
        NDonors,element.vertices.front().data(),element.query_point.data(),weights.data());

    auto handle = yoga_create_linearized_donor_stencil(NDonors, donors, query_point);
    for(int weight_index =0; weight_index < NDonors; weight_index++) {
        for(int donor_index=0; donor_index < NDonors; donor_index++) {
            double actual = yoga_partial_donor_x(handle, weight_index, donor_index);
            double expected = element.vertexPartialX(weights[weight_index], donor_index);
            REQUIRE(expected == actual);
            actual = yoga_partial_donor_y(handle, weight_index, donor_index);
            expected = element.vertexPartialY(weights[weight_index], donor_index);
            REQUIRE(expected == actual);
            actual = yoga_partial_donor_z(handle, weight_index, donor_index);
            expected = element.vertexPartialZ(weights[weight_index], donor_index);
            REQUIRE(expected == actual);
        }
        double actual = yoga_partial_receptor_x(handle, weight_index);
        double expected = element.queryPointPartialX(weights[weight_index]);
        REQUIRE(expected == actual);
        actual = yoga_partial_receptor_y(handle, weight_index);
        expected = element.queryPointPartialY(weights[weight_index]);
        REQUIRE(expected == actual);
        actual = yoga_partial_receptor_z(handle, weight_index);
        expected = element.queryPointPartialZ(weights[weight_index]);
        REQUIRE(expected == actual);
    }

    yoga_destroy_linearized_donor_stencil(handle);
}

TEST_CASE("get derivatives from c-interface"){
    SECTION("tetrahedron") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 4> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {0, 2, 0};
        vertices[3] = {0, 0, 2};

        checkCInterfaceAgainstDdata<4>(vertices.front().data(),query_point.data());
    }

    SECTION("pyramid") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 5> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {2, 2, 0};
        vertices[3] = {0, 2, 0};
        vertices[4] = {0, 0, 2};

        checkCInterfaceAgainstDdata<5>(vertices.front().data(),query_point.data());
    }

    SECTION("prism") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 6> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {0, 2, 0};
        vertices[3] = {0, 0, 1};
        vertices[4] = {2, 0, 1};
        vertices[5] = {0, 2, 1};

        checkCInterfaceAgainstDdata<6>(vertices.front().data(),query_point.data());
    }

    SECTION("hex") {
        Parfait::Point<double> query_point = {.1, .1, .1};
        std::array<Parfait::Point<double>, 8> vertices;
        vertices[0] = {0, 0, 0};
        vertices[1] = {2, 0, 0};
        vertices[2] = {2, 2, 0};
        vertices[3] = {0, 2, 0};
        vertices[4] = {0, 0, 1};
        vertices[5] = {2, 0, 1};
        vertices[6] = {2, 2, 1};
        vertices[7] = {0, 2, 1};

        checkCInterfaceAgainstDdata<8>(vertices.front().data(),query_point.data());
    }
}

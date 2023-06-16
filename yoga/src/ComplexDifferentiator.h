#pragma once

namespace YOGA{

    template <typename Interpolate,int N>
    class ComplexDifferentiator{
    public:
        ComplexDifferentiator(
                Interpolate interpolate,
                const std::array<Parfait::Point<double>,N>& tet,
                const Parfait::Point<double>& query_point):
                interpolate(interpolate){
            for(int j=0;j<3;j++)
                complex_query_point[j] = query_point[j];
            for(int i=0;i<N;i++)
                for(int j=0;j<3;j++)
                    complex_element[i][j] = tet[i][j];
        }

        std::array<double,N> values(){
            std::array<std::complex<double>,N> complex_weights;
            interpolate(N,complex_element.front().data(),
                        complex_query_point.data(), complex_weights.data());
            std::array<double,N> real_weights;
            for(int i=0;i<N;i++)
                real_weights[i] = complex_weights[i].real();
        }

        std::array<double,N> dwdx(int vertex){
            return calcComplexDerivativesForVertex(vertex,0);
        }

        std::array<double,N> dwdy(int vertex){
            return calcComplexDerivativesForVertex(vertex,1);
        }

        std::array<double,N> dwdz(int vertex){
            return calcComplexDerivativesForVertex(vertex,2);
        }


    private:
        Interpolate interpolate;
        std::array<Parfait::Point<std::complex<double>>,N> complex_element;
        Parfait::Point<std::complex<double>> complex_query_point;
        double epsilon = 1.0e-15;

        std::array<double,N> calcComplexDerivativesForVertex(int vertex,int dimension) {
            std::array<std::complex<double>,N> complex_weights;
            auto perturbed_tet = complex_element;
            perturbed_tet[vertex] = perturb(complex_element[vertex],dimension);

            interpolate(N,perturbed_tet.front().data(),
                        complex_query_point.data(), complex_weights.data());
            std::array<double,N> derivatives;
            for(int i=0;i<N;i++)
                derivatives[i] = complex_weights[i].imag() / epsilon;
            return derivatives;
        }

        Parfait::Point<std::complex<double>> perturb(const Parfait::Point<std::complex<double>>& p,int dimension){
            auto perturbed = p;
            perturbed[dimension].imag(epsilon);
            return perturbed;
        }
    };

}

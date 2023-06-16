#pragma once
#include <DifferentiableDonorElement.h>
#include <YogaTypeSupport.h>

namespace YOGA{

template<int NDonors>
class LinearizedStencil{
  public:
    static int constexpr n_derivatives = (NDonors+1)*3;
    LinearizedStencil(double* donor_xyzs,double* query_xyz)
        :element(donor_xyzs,query_xyz,NDonors){
        weights.resize(NDonors);

        using DdataInterpolator = LeastSquaresInterpolation<ADType<n_derivatives>>;
        DdataInterpolator::calcWeights(NDonors,
                                       element.vertices.front().data(),
                                       element.query_point.data(),
                                       weights.data());
    }

    double partialDonorX(int weight_index,int donor_index){
        return element.vertexPartialX(weights[weight_index],donor_index);
    }
    double partialDonorY(int weight_index,int donor_index){
        return element.vertexPartialY(weights[weight_index],donor_index);
    }
    double partialDonorZ(int weight_index,int donor_index){
        return element.vertexPartialZ(weights[weight_index],donor_index);
    }
    double partialReceptorX(int weight_index){
        return element.queryPointPartialX(weights[weight_index]);
    }
    double partialReceptorY(int weight_index){
        return element.queryPointPartialY(weights[weight_index]);
    }
    double partialReceptorZ(int weight_index){
        return element.queryPointPartialZ(weights[weight_index]);
    }

    DifferentiableDonorElement<n_derivatives> element;
    std::vector<ADType<n_derivatives>> weights;
};

struct StencilWrapper{
    int n_donors;
    void* stencil;
};

template<int NDonors>
StencilWrapper* makeWrappedStencil(double* donor_xyzs, double* query_xyz){
    auto stencil = new LinearizedStencil<NDonors>(donor_xyzs, query_xyz);
    auto wrapper = new StencilWrapper();
    wrapper->stencil = stencil;
    wrapper->n_donors = NDonors;
    return wrapper;
}

enum Partial {DX, DY, DZ};

template<int NDonors>
double donorPartial(StencilWrapper* ptr,int weight_index, int donor_index,Partial xyz){
    auto stencil = (LinearizedStencil<NDonors>*)ptr->stencil;
    switch (xyz) {
        case DX: return stencil->partialDonorX(weight_index,donor_index);
        case DY: return stencil->partialDonorY(weight_index,donor_index);
        case DZ: return stencil->partialDonorZ(weight_index,donor_index);
    }
    return 0.0;
}

template<int NDonors>
double receptorPartial(StencilWrapper* ptr,int weight_index, Partial xyz){
    auto stencil = (LinearizedStencil<NDonors>*)ptr->stencil;
    switch (xyz) {
        case DX: return stencil->partialReceptorX(weight_index);
        case DY: return stencil->partialReceptorY(weight_index);
        case DZ: return stencil->partialReceptorZ(weight_index);
    }
    return 0.0;
}

}
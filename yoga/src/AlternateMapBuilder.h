#pragma once

#include <parfait/ToString.h>
namespace YOGA{
    class AlternateMapBuilder{
      public:

        template<typename T>
        static std::vector<int> getIndicesOfLowest(const std::vector<T>& vec,int n){
            auto less = [&](int a,int b){
                return vec[a] < vec[b];
            };
            return getExtremaIndices(vec,n,less);
        }

        template<typename T>
        static std::vector<int> getIndicesOfHighest(const std::vector<T>& vec,int n){
            auto greater = [&](int a,int b){
                return vec[a] > vec[b];
            };
            return getExtremaIndices(vec,n,greater);
        }

  private:
    template<typename T,typename Compare>
    static std::vector<int> getExtremaIndices(const std::vector<T>& vec,int n,Compare compare){
        if(0==n) return {};
        std::vector<int> indices;
        for(int i=0;i<long(vec.size());i++){
            if(i<n){
                indices.push_back(i);
            }
            else{
                std::sort(indices.begin(),indices.end(),compare);
                if(compare(i,indices.back())){
                    indices.back() = i;
                }
            }
        }
        return indices;
    }

    };
}

#pragma once
#include <vector>
#include <set>
#include <MessagePasser/MessagePasser.h>
#include <parfait/StringTools.h>

namespace YOGA{

class ParallelColorCombinator{
  public:
    template<typename Syncer>
    static std::vector<int> combineToOrdinals(const std::vector<std::vector<int>>& graph,
                                                       const std::vector<int>& colors,
                                                       const std::vector<int>& owning_rank,
                                                       MessagePasser mp,
                                                       const Syncer& syncer){
        printUnique(mp,colors);
        auto agglomerated = combine(colors,owning_rank, mp, syncer);
        printUnique(mp,colors);
        auto ordinals = mapToOrdinals(mp, agglomerated);
        printUnique(mp,colors);
        return ordinals;
    }


  private:

    static void printUnique(MessagePasser mp,const std::vector<int>& colors){
        std::set<int> unique_colors(colors.begin(),colors.end());
        unique_colors = mp.ParallelUnion(unique_colors);
        if(0 == mp.Rank()) {
            printf("There are %li unique colors", unique_colors.size());
            for(auto c:unique_colors)
                printf("%i ",c);
            printf("\n");
        }
    }

    static std::vector<int> mapToOrdinals(const MessagePasser& mp, const std::vector<int>& colors) {
        std::vector<int> ordinals = colors;
        auto old_to_new = getOrdinalMap(mp, colors);
        for(auto& color: ordinals)
            color = old_to_new.at(color);
        return ordinals;
    }

    template<typename Syncer>
    static std::vector<int> combine(const std::vector<int>& colors,
                                    const std::vector<int>& owning_rank,
                                    const MessagePasser& mp, const Syncer& syncer) {
        std::vector<int> agglomerated = colors;
        auto before = agglomerated;
        int i=0;
        do{
            before = agglomerated;
                syncer.sync(agglomerated);
            updateColors(agglomerated,before,owning_rank,mp.Rank());
            i++;
        }
        while(not done(mp, agglomerated, before) and i<5);
        return agglomerated;
    }

    static bool done(const MessagePasser& mp, const std::vector<int>& agglomerated, const std::vector<int>& before) {
        long n = countDifferent(agglomerated,before);
        n = mp.ParallelSum(n);
        if(0 == mp.Rank())
            printf("Updated %li node colors\n",n);
        return n == 0;
    }

    static long countDifferent(const std::vector<int>& a,const std::vector<int>& b){
        long n = 0;
        for(size_t i=0;i<a.size();i++)
            if(a[i] != b[i])
                ++n;
        return n;
    }

    static void updateColors(std::vector<int>& colors,
                             const std::vector<int>& old_colors,
                             const std::vector<int>& owning_rank,
                             int my_rank){

        auto old_to_new = buildMap(old_colors, colors,owning_rank,my_rank);
        printf("old to new: ");
        for(auto& pair:old_to_new)
            printf("%i --> %i\n",pair.first,pair.second);
        for(auto& c:colors)
            if(old_to_new.count(c) == 1)
                c = old_to_new[c];
    }

    static std::map<int, int> buildMap(const std::vector<int>& old_colors,
                                       const std::vector<int>& colors,
                                       const std::vector<int>& owning_rank,
                                       int my_rank) {
        std::map<int,int> old_to_new;
        for(size_t i=0;i<colors.size();i++)
            if(colors[i] != old_colors[i] and owning_rank[i] < my_rank)
                old_to_new[old_colors[i]] = colors[i];
        return old_to_new;
    }

    static std::map<int,int> getOrdinalMap(MessagePasser mp,const std::vector<int>& colors){
        std::set<int> color_set(colors.begin(),colors.end());
        color_set = mp.ParallelUnion(color_set);
        std::map<int,int> old_to_new_color;
        int new_color = 0;
        for(auto& old_color:color_set)
            old_to_new_color[old_color] = new_color++;
        return old_to_new_color;
    }
};

}

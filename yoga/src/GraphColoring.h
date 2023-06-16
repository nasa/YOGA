#pragma once
#include <vector>
#include <queue>

namespace YOGA{

class GraphColoring {
  public:
    typedef const std::vector<std::vector<int>>& Graph;
    typedef std::vector<int> Colors;


    static Colors colorDisjointSubgraphs(Graph graph){
        int n = int(graph.size());
        std::vector<int> colors(n,NOT_SET);
        for(int i=0,current=0;i<n;i++)
            if(colors[i] == NOT_SET)
                floofFill(i,graph,colors,current++);
        return colors;
    }

  private:
    enum {NOT_SET=-1};

    static void floofFill(int starting_node,Graph graph,Colors& colors,int current_color){
        std::queue<int> q;
        q.push(starting_node);
        colors[starting_node] = current_color;
        while(not q.empty()) {
            int node = q.front();
            q.pop();
            colors[node] = current_color;
            auto& nbrs = graph[node];
            paint(nbrs, colors, current_color, q);
        }
    }

    static void paint(const std::vector<int>& ids,Colors& colors,int color,std::queue<int>& q){
        for (auto& id : ids) {
            if(colors[id] == NOT_SET) {
                colors[id] = color;
                q.push(id);
            }
        }
    }
};
}

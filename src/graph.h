#ifndef GRAPH_H
#define GREPH_H

#include <vector>
#include <array>
#include <string>
class Graph
{
    using ull = unsigned long long;
    //the csc format graph
private:
    std::vector<unsigned> edge_index;
    std::vector<unsigned> edges;
    std::vector<std::vector<double>> nodes;

public:
    //for simulation, we should get the dege index addr when to access the dege index
    ull get_edge_index_addr(unsigned index) const;
    ull get_dege_addr(unsigned index) const;
    unsigned get_edge_size(unsigned index) const;

    const std::vector<unsigned> &get_edge_index() const { return edge_index; }
    const std::vector<unsigned> &get_edges() const { return edges; }
    const std::vector<std::vector<double>> &get_nodes() const { return nodes; }

    void parse(std::string graph_name);
    void print() const;
};

#endif
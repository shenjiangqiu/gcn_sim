#ifndef GRAPH_H
#define GRAPH_H

#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
 class Graph {
  using ull = unsigned long long;
  // the csc format graph
private:
  std::vector<unsigned> edge_index;
  std::vector<unsigned> edges;
  std::vector<std::vector<double>> nodes;

public:
  // for simulation, we should get the dege index addr when to access the dege
  // index
  unsigned get_num_nodes() const { return edge_index.size() - 1; }
  ull get_edge_index_addr(unsigned index) const {
    return (ull) & (edge_index.at(index));
  }
  ull get_dege_addr(unsigned index) const {
    return (ull) & (edges.at(index));
  }
  unsigned get_edge_size(unsigned index) const {
    return edge_index.at(index + 1) - edge_index.at(index);
  }
  const std::vector<unsigned> &get_edge_index() const { return edge_index; }
  const std::vector<unsigned> &get_edges() const { return edges; }
  const std::vector<std::vector<double>> &get_nodes() const { return nodes; }
  Graph(const std::string name) { this->parse(name); }
  void parse(std::string graph_name) {
    std::string full_graph_name = graph_name + ".graph";
    std::ifstream graph_in(full_graph_name);
    if (!graph_in.is_open()) {
      throw "cannot open the file";
    }
    int node = 0;
    edge_index.push_back(0);
    while (true) {
      std::string line;
      getline(graph_in, line);
      if (graph_in.eof())
        break;
      edges.push_back(node);
      node++;
      edge_index.push_back(edge_index.back() + 1);
      std::stringstream ss(line);
      while (true) {
        int neighbor;
        ss >> neighbor;
        if (ss.fail())
          break;
        edges.push_back(neighbor);
        edge_index.back() += 1;
      }
    }
  }
  void print() const {
    for (auto index : edge_index) {
      std::cout << index << " ";
    }
    std::cout << std::endl;
    for (auto e : edges) {
      std::cout << e << " ";
    }
    std::cout << std::endl;
  }
};

#endif
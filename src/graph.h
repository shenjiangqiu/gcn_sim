#ifndef GRAPH_H
#define GRAPH_H

#include <array>
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
  ull get_edge_index_addr(unsigned index) const;
  ull get_dege_addr(unsigned index) const;
  unsigned get_edge_size(unsigned index) const;

  const std::vector<unsigned> &get_edge_index() const { return edge_index; }
  const std::vector<unsigned> &get_edges() const { return edges; }
  const std::vector<std::vector<double>> &get_nodes() const { return nodes; }
  Graph(const std::string name) { this->parse(name); }
  void parse(std::string graph_name);
  void print() const;
};

#endif
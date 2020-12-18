#include <fstream>
#include <graph.h>
#include <sstream>

#include <iostream>

Graph::ull Graph::get_edge_index_addr(unsigned int index) const {
  return (Graph::ull) & (edge_index.at(index));
}
Graph::ull Graph::get_dege_addr(unsigned int index) const {
  return (Graph::ull) & (edges.at(index));
}
unsigned int Graph::get_edge_size(unsigned int index) const {
  return edge_index.at(index + 1) - edge_index.at(index);
}
void Graph::parse(std::__cxx11::string graph_name) {
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
    std::istringstream ss(line);
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

void Graph::print() const {
  for (auto index : edge_index) {
    std::cout << index << " ";
  }
  std::cout << std::endl;
  for (auto e : edges) {
    std::cout << e << " ";
  }
  std::cout << std::endl;
}

#include <fmt/format.h>
#include <fstream>
#include <graph.h>
#include <iostream>
int main() {
  Graph<unsigned> m_graph("test");
  m_graph.print();
  return 0;
}
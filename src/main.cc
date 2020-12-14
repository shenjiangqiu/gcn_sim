#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <graph.h>
int main()
{
    Graph m_graph;
    m_graph.parse("test");
    m_graph.print();
    return 0;
}
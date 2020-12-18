#ifndef AGGREGATE_H
#define AGGREGATE_H
#include <buffer.h>
#include <graph.h>
#include <iostream>
#include <memory>
#include <ramulator_wrapper.h>
#include <set>
#include <size.h>
#include <tuple>

class Aggregator {

  // the aggregator. need to deal with partition and memory fetch.

private:
  std::shared_ptr<Graph> m_graph;

  const unsigned input_buffer_size;
  const unsigned output_buffer_size;
  const unsigned edge_buffer_size;
  const unsigned feature_size;
  const unsigned feature_elements;

  unsigned slide_width;
  unsigned slide_height;

  unsigned total_x;
  unsigned total_y;

  unsigned current_x;
  unsigned current_y;

  std::vector<unsigned> current_edge_index;

  unsigned current_window_size;

  unsigned long long m_cycle;

  bool next_input_ready = false;
  bool next_edge_ready = false;
  bool current_nonempty_line_set = false;
  std::set<unsigned> nonempty_line_set;
  ramulator_wrapper m_ramulator;

public:
  bool is_running() const;
  Aggregator(std::shared_ptr<Graph> graph, const ramulator::Config &configs,
             int cacheline, unsigned in_s = 128 * K, unsigned out_s = 16 * M,
             unsigned e_s = 2 * M, unsigned feat_s = 4, unsigned feat_l = 1433)
      : m_graph(graph), input_buffer_size(in_s), output_buffer_size(out_s),
        edge_buffer_size(e_s), feature_size(feat_s), feature_elements(feat_l),
        current_x(0), current_y(0), current_window_size(0), m_cycle(0),
        m_ramulator(configs, cacheline) {
    slide_width = (output_buffer_size / 2) / (feature_size * feature_elements);
    slide_height = (input_buffer_size / 2) / (feature_size * feature_elements);
    std::cout << slide_width << std::endl;
    std::cout << slide_height << std::endl;
    for (auto i = 0u; i < slide_width; i++) {
      current_edge_index.push_back(0);
    }
    total_x = m_graph->get_num_nodes();
    total_y = total_x;
  }
  std::tuple<unsigned, unsigned, unsigned> get_next_window();
  void reset_all();
  void reset_output();
  void reset_input();
  bool is_empty_line(unsigned x, unsigned y);

  void cycle();
};

#endif
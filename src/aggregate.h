#ifndef AGGREGATE_H
#define AGGREGATE_H
#include <buffer.h>
#include <graph.h>
#include <iostream>
#include <memory>
#include <memory_interface.h>
#include <queue>
#include <ramulator_wrapper.h>
#include <set>
#include <size.h>
#include <tuple>
#include <types.h>
class Aggregator {

  // the aggregator. need to deal with partition and memory fetch.

private:
  std::shared_ptr<Graph> m_graph;

  const unsigned input_buffer_size;
  const unsigned output_buffer_size;
  const unsigned edge_buffer_size;
  const unsigned feature_size;
  std::vector<unsigned> feature_elements;

  std::vector<unsigned> slide_width;
  std::vector<unsigned> slide_height;

  unsigned total_x;
  unsigned total_y;

  unsigned current_x;
  unsigned current_y;
  unsigned current_level = 0;
  unsigned current_window_size;
  unsigned current_window_remain_cycles = 0;
  unsigned long long m_cycle;
  bool current_waiting_buffer = true;
  bool next_input_ready = false;
  bool next_edge_ready = false;

  bool finished_all_level = false;

  std::set<unsigned> nonempty_line_set;

  std::shared_ptr<ramulator_wrapper> m_ramulator;
  std::shared_ptr<memory_interface> m_mem_interface;

  Buffer InputBuffer;
  Buffer OutputBuffer;
  Buffer EdgeBuffer;
  unsigned num_cores;
  unsigned data_width;

  std::queue<std::shared_ptr<Req>> input_request_q;
  std::queue<std::shared_ptr<Req>> input_response_q;
  std::queue<std::shared_ptr<Req>> output_request_q;
  std::queue<std::shared_ptr<Req>> output_response_q;
  std::queue<std::shared_ptr<Req>> edge_request_q;
  std::queue<std::shared_ptr<Req>> edge_response_q;

  // 0=in,1=out,2=edge
  std::map<unsigned long long, int> addr_to_dest;
  unsigned global_req_id = 0;

public:
  bool is_running() const;
  Aggregator(std::shared_ptr<Graph> graph, const ramulator::Config configs,
             int cacheline, unsigned in_s, unsigned out_s, unsigned e_s,
             unsigned feat_s, std::vector<unsigned> feat_l, unsigned num_cores,
             unsigned data_width)
      : m_graph(graph), input_buffer_size(in_s), output_buffer_size(out_s),
        edge_buffer_size(e_s), feature_size(feat_s), feature_elements(feat_l),
        current_x(num_cores), current_y(num_cores),
        current_window_size(num_cores), m_cycle(0),
        m_ramulator(std::make_shared<ramulator_wrapper>(configs, cacheline)),
        InputBuffer("inputBuffer"), OutputBuffer("outputBuffer"),
        EdgeBuffer("edgeBuffer"), num_cores(num_cores), data_width(data_width),
        input_request_q(), input_response_q(), output_request_q(),
        output_response_q(), edge_request_q(), edge_response_q() {
    for (unsigned i = 0; i < feature_elements.size(); i++) {
      slide_width[i] =
          (output_buffer_size / 2) / (feature_size * feature_elements[i]);
      slide_height[i] =
          (input_buffer_size / 2) / (feature_size * feature_elements[i]);
      std::cout << slide_width[i] << std::endl;
      std::cout << slide_height[i] << std::endl;
      std::cout << "\n";
    }

    total_x = m_graph->get_num_nodes();
    total_y = total_x / num_cores;
  }
  std::tuple<unsigned, unsigned, unsigned> get_next_window();
  void reset_all();
  void reset_output();
  void reset_input();
  bool is_empty_line(unsigned x, unsigned y);

  void cycle();
  unsigned calculate_the_cycle_of_window(unsigned x, unsigned y, unsigned z);
  bool buffer_ready(unsigned row, unsigned col);
  void prefetch(unsigned, unsigned, unsigned);
  unsigned long long get_input_addr_by_location(unsigned x, unsigned y,
                                                unsigned z);
};

#endif
#ifndef AGGREGATE_H
#define AGGREGATE_H
#include <buffer.h>
#include <graph.h>
#include <iostream>
#include <memory>
#include <queue>
#include <ramulator_wrapper.h>
#include <set>
#include <size.h>
#include <tuple>
#include <types.h>

class memory_interface {
private:
  unsigned waiting_size;
  std::queue<std::shared_ptr<Req>> req_queue;
  std::queue<std::pair<unsigned long long, bool>> out_send_queue;
  std::queue<unsigned long long> response_queue;
  std::queue<std::shared_ptr<Req>> task_return_queue;

  std::map<unsigned, unsigned> id_to_numreqs_map;
  std::map<unsigned long long, shared_ptr<Req>> addr_to_req_map;

public:
  bool avaliable() { return req_queue.size() < waiting_size; }
  void send(std::shared_ptr<Req> req) {
    assert(req_queue.size() < waiting_size);
    req_queue.push(req);
    auto num_reqs = (req->len + 63) / 64;
    id_to_numreqs_map.insert({req->id, num_reqs});
  }
  std::pair<unsigned long long, bool> get_next() {
    return out_send_queue.front();
  }
  bool ret_avaliable() { return task_return_queue.size() > 0; }
  std::shared_ptr<Req> get_req() {
    auto ret = task_return_queue.front();
    task_return_queue.pop();
    return ret;
  }

  memory_interface();

  void cycle() {
    // send policy changed here
    if (!req_queue.empty()) {
      auto next_req = req_queue.front();
      if (next_req->len > 0) {
        addr_to_req_map.insert({next_req->addr, next_req});
        out_send_queue.push(
            {next_req->addr, next_req->req_type == mem_request::write});
        if (next_req->len - 64 <= 0) {
          req_queue.pop();
        } else {
          next_req->addr += 64;
          next_req->len -= 64;
        }
      }
    }
    if (!response_queue.empty()) {
      auto &resp = response_queue.front();
      auto req = addr_to_req_map.at(resp);
      addr_to_req_map.erase(resp);
      if (--id_to_numreqs_map.at(req->id) == 0) {
        id_to_numreqs_map.erase(req->id);
        task_return_queue.push(req);
      }
    }
  }
};

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
  bool next_input_ready;
  bool next_edge_ready;
  bool current_nonempty_line_set;
  bool finished_all_level = false;

  std::set<unsigned> nonempty_line_set;
  ramulator_wrapper m_ramulator;

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
        m_ramulator(configs, cacheline), InputBuffer("inputBuffer"),
        OutputBuffer("outputBuffer"), EdgeBuffer("edgeBuffer"),
        num_cores(num_cores), data_width(data_width), input_request_q(),
        input_response_q(), output_request_q(), output_response_q(),
        edge_request_q(), edge_response_q() {
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
  std::tuple<unsigned, unsigned, unsigned> get_next_window(unsigned level);
  void reset_all();
  void reset_output();
  void reset_input();
  bool is_empty_line(unsigned x, unsigned y, unsigned level);

  void cycle();
  unsigned calculate_the_cycle_of_window(unsigned x, unsigned y);
  bool buffer_ready(unsigned row, unsigned col);
};

#endif
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
class Comb {

public:
  unsigned calculate_comb_cycle(unsigned number_v, unsigned size_v_in,
                                unsigned size_v_out) {
    unsigned total_cycle = 0;

    auto total_row = num_module * num_row;
    auto number_round_v = (number_v + total_row - 1) / total_row;
    auto number_round_w = (size_v_out + num_col - 1) / num_col;
    auto the_last_v = number_v - (number_round_v - 1) * total_row;
    auto the_last_w = size_v_out - (number_round_w - 1) * total_row;

    // for every full task get the cycle:
    for (auto i = 0; i < number_round_v - 1; i++) {
      for (auto j = 0; j < number_round_w - 1; j++) {
        total_cycle += total_row + size_v_in + num_col;
      }
    }
    for (auto i = 0; i < number_round_v - 1; i++) {
      if (the_last_w != 0) {
        total_cycle += total_row + size_v_in + the_last_w;
      }
    }
    total_cycle += the_last_v + size_v_in + the_last_w;

    
    return total_cycle;
  }
  unsigned get_comb_cycle();
  void set_comb_cycle(unsigned cycle) {
    assert(current_remain_cycle == 0);
    current_remain_cycle = cycle;
  }
  void set_busy() { busy = true; }
  void set_no_busy() { busy = false; }
  bool is_busy() { return current_remain_cycle != 0; }
  void cycle() {
    if (current_remain_cycle != 0) {
      current_remain_cycle--;
    }
  }

private:
  unsigned current_remain_cycle;
  bool busy;

  unsigned num_module;
  unsigned num_row;
  unsigned num_col;
};

template <typename Node_type> class Aggregator {

  // the aggregator. need to deal with partition and memory fetch.

private:
  std::shared_ptr<Comb> m_combination_unit;
  // multi level, multi nodes, multi
  // elements per node, 3d array
  Node_type *feature_vectors;

  std::shared_ptr<Graph<Node_type>> m_graph;

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
  Buffer CombOutputBuffer;
  Buffer EdgeBuffer;
  Aggregator_buffer aggr_buffer;
  unsigned num_cores;

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
  Aggregator(std::shared_ptr<Graph<Node_type>> graph,
             const ramulator::Config configs, int cacheline, unsigned in_s,
             unsigned out_s, unsigned e_s, unsigned feat_s,
             std::vector<unsigned> feat_l)
      : m_graph(graph), input_buffer_size(in_s), output_buffer_size(out_s),
        edge_buffer_size(e_s), feature_size(feat_s), feature_elements(feat_l),
        current_x(0), current_y(0), current_window_size(0), m_cycle(0),
        m_ramulator(std::make_shared<ramulator_wrapper>(configs, cacheline)),
        InputBuffer("inputBuffer"), OutputBuffer("outputBuffer"),
        EdgeBuffer("edgeBuffer"), input_request_q(), input_response_q(),
        output_request_q(), output_response_q(), edge_request_q(),
        edge_response_q() {
    for (unsigned i = 0; i < feature_elements.size(); i++) {
      slide_width[i] =
          (output_buffer_size / 2) / (feature_size * feature_elements[i]);
      slide_height[i] =
          (input_buffer_size / 2) / (feature_size * feature_elements[i]);
      std::cout << slide_width[i] << std::endl;
      std::cout << slide_height[i] << std::endl;
      std::cout << "\n";
    }
    unsigned total_level = feat_l.size();
    unsigned total_elements = 0;
    for (unsigned i = 0; i < total_level; i++) {
      total_elements += feat_l[i] * m_graph->get_num_nodes();
    }
    feature_vectors = new Node_type[total_elements];

    total_x = m_graph->get_num_nodes();
    total_y = total_x;
  }
  std::tuple<unsigned, unsigned, unsigned> get_next_window() {
    auto next_start_y = current_y + current_window_size;
    auto next_start_x = current_x;

    // reset_input();
    if (next_start_y >= total_y) {
      next_start_y = 0;
      next_start_x += slide_width[current_level];
      // reset_output();
      if (next_start_x >= total_x) {
        // reset
        // reset_all();

        return {0, 0, 0};
      }
    }

    // skipping zero lines:
    while (is_empty_line(next_start_x, next_start_y)) {
      next_start_y++;
      if (next_start_y >= total_y) {
        next_start_x += slide_width[current_level];
        next_start_y = 0;
        // reset_output();
        if (next_start_x >= total_x) {
          // reset
          // reset_all();

          return {0, 0, 0};
        }
      }
    }

    // shrinking:
    auto new_start_windows_size =
        std::min(slide_height[current_level], total_y - next_start_y);

    while (is_empty_line(next_start_x,
                         next_start_y + new_start_windows_size - 1)) {
      new_start_windows_size--;
      assert(new_start_windows_size != 0);
    }
    // current_x = next_start_x;
    // current_y = next_start_y;
    // current_window_size = new_start_windows_size;

    return {next_start_x, next_start_y, new_start_windows_size};
  }

  void reset_all() {
    // clean all the buffer
    EdgeBuffer.clear();
    InputBuffer.clear();
  }
  void reset_output() {
    // need to reset the edge
    EdgeBuffer.just_move_the_buffer();
  }

  void reset_input() { InputBuffer.just_move_the_buffer(); }
  bool is_empty_line(unsigned x, unsigned y) {
    const auto &edge_index = m_graph->get_edge_index();
    const auto &edges = m_graph->get_edges();
    static unsigned current_bufferd = -1;
    static unsigned current_bufferd_level = -1;
    if (current_bufferd != x or current_bufferd_level != current_level) {
      current_bufferd = x;
      current_bufferd_level = current_level;
      nonempty_line_set.clear();

      auto edge_start = edge_index.at(x);
      auto edge_end = edge_index.at(x + slide_width[current_level]);
      while (edge_start < edge_end) {
        nonempty_line_set.insert(edges.at(edge_start));
        edge_start++;
      }
    }

    return nonempty_line_set.count(y) == 0;
  }

  void cycle() {
    // output_buffer_to_comb

    // the combination logic
    InputBuffer.cycle();
    // write back
    CombOutputBuffer.cycle();
    EdgeBuffer.cycle();
    m_mem_interface->cycle();

    if (aggr_buffer.is_current_ready() and !m_combination_unit->is_busy() and
        CombOutputBuffer.is_next_empty()) {
      auto next_cycle = m_combination_unit->calculate_comb_cycle(OutputBuffer);
      m_combination_unit->set_comb_cycle(next_cycle);
      m_combination_unit->set_busy();
    }
    if (m_combination_unit->is_busy()) {
      auto remain_cycle = m_combination_unit->get_comb_cycle();
      if (remain_cycle == 0 and CombOutputBuffer.is_current_empty()) {
        // finished
        // set not busy
        m_combination_unit->set_no_busy();
        // out buffer move to current and start to write
        CombOutputBuffer.just_move_the_buffer();
        // aggregate buffer move next to current
        aggr_buffer.move();

      } else {
        // may be not empy
      }
    }

    // local cycle:
    if (InputBuffer.is_out_send_q_ready()) {

      // have the buffer,but not issue to the dram.
      // issue to dram
      auto req = InputBuffer.pop_out_send_req();
      m_mem_interface->send(req);
    }
    if (EdgeBuffer.is_out_send_q_ready()) {

      // have the buffer,but not issue to the dram.
      // issue to dram
      auto req = EdgeBuffer.pop_out_send_req();
      m_mem_interface->send(req);
    }

    // the very beginning
    if (current_window_size == 0) {
      // no task generated now
      assert(InputBuffer.is_current_task_ready() == false);

      assert(current_x == 0 and current_y == 0);
      auto [x, y, z] = get_next_window();
      if (x == 0 and y == 0 and z == 0) {
        // no more tasks remain.
        if (current_level + 1 == feature_elements.size()) {
          current_level++;
        } else {
          finished_all_level = true;
          // finish the simulation, or we need waiting for the writing back.
        }
      } else {
        current_x = x;
        current_y = y;
        current_window_size = z;
        // got new task,
        if (InputBuffer.is_next_task_ready()) {
          assert(false);
          // move next to current
          assert(InputBuffer.get_next_location() ==
                 std::make_tuple(current_x, current_y, current_window_size,
                                 current_level));
          InputBuffer.just_move_the_buffer();
          assert(EdgeBuffer.is_next_task_ready());
          EdgeBuffer.just_move_the_buffer();
        } else {
          assert(!EdgeBuffer.is_next_task_ready());

          // all new
          auto req = std::make_shared<Req>();
          req->addr = get_input_addr_by_location(x, y, z);
          req->len = z * feature_size * feature_elements[current_level];
          req->req_type = mem_request::read;
          req->t = device_types::input_buffer;
          // will send to the firt
          InputBuffer.send(req);
          auto [nx, ny, nz] = get_next_window();
          prefetch_input(nx, ny, nz);
          prefetch_edge(nx, ny, nz);

          // TODO contine the cycle function;
        }
      }
    }
    // aggregator_to_output_buffer
    if (current_window_remain_cycles == 0 and current_waiting_buffer) {
      assert(current_window_size != 0);
      // we are waiting buffer now
      assert(InputBuffer.get_current_location() ==
             std::make_tuple(current_x, current_y, current_window_size,
                             current_level));
      assert(EdgeBuffer.get_current_location() ==
             std::make_tuple(current_x, current_y, current_window_size,
                             current_level));
      assert(OutputBuffer.get_current_location() ==
             std::make_tuple(current_x, current_y, current_window_size,
                             current_level));
      if (!InputBuffer.is_current_data_ready() or
          !EdgeBuffer.is_current_data_ready() or
          !OutputBuffer.is_current_data_ready()) {
        // not ready now
      } else {
        // all ready now
        current_window_remain_cycles = calculate_the_cycle_of_window(
            current_x, current_y, current_window_size);
        current_waiting_buffer = false;
      }
    }

    if (current_window_size != 0) {
      if (current_window_remain_cycles != 0) {
        assert(!current_waiting_buffer);
        current_window_remain_cycles--;
        if (current_window_remain_cycles == 0) {
          // finished this windows,switch to the next windows;
          auto [x, y, z] = get_next_window(); // will
          if (x == 0 and y == 0 and z == 0) {
            // finished current level, move to next level;
            current_level++;
            if (current_level == feature_elements.size()) {
              // finished all level;
              finished_all_level = true;
            }
            current_x = x;
            current_y = y;
            current_window_size = z;
            return;
          }

          if (x != current_x) {
            // move to next output buffer
            current_y = y;
            current_x = x;
            current_window_size = z;
            reset_output();
            reset_input();
            auto [nx, ny, nz] = get_next_window();
            prefetch_input(nx, ny, nz);
            prefetch_edge(nx, ny, nz);
            assert(InputBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));
            assert(EdgeBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));
            assert(OutputBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));

            if (buffer_ready(x, y)) {
              current_window_remain_cycles =
                  calculate_the_cycle_of_window(x, y, z);

            } else {
              current_window_remain_cycles = 0;
              current_waiting_buffer = true;
            }

          } else {
            assert(current_y != y);
            current_y = y;
            current_x = x;
            current_window_size = z;

            reset_input();

            auto [nx, ny, nz] = get_next_window();
            prefetch_input(nx, ny, nz);
            assert(InputBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));
            assert(EdgeBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));
            assert(OutputBuffer.get_current_location() ==
                   std::make_tuple(x, y, z, current_level));

            if (buffer_ready(x, y)) {
              current_window_remain_cycles =
                  calculate_the_cycle_of_window(x, y, z);

            } else {
              current_window_remain_cycles = 0;
              current_waiting_buffer = true;
            }
          }

        } else {
          // only pass this cycle..do nothing. it's still running
        }
      } else {
        // no task remain, maybe waiting for the buffer
        // maybe need to start the next level;
        if (current_waiting_buffer) {
          // contineuw waiting
        }
      }
    }
    // input_to_output
    // edge_to_output
    // dram_to_input
    // dram_to_edge
  }

  unsigned calculate_the_cycle_of_window(unsigned x, unsigned y, unsigned z) {
    return 100;
  }
  bool buffer_ready(unsigned row, unsigned col) {
    auto input_ready = InputBuffer.is_current_data_ready();
    auto edge_ready = EdgeBuffer.is_current_data_ready();

    assert(std::get<0>(InputBuffer.get_current_location()) == row);
    assert(std::get<1>(InputBuffer.get_current_location()) == col);

    assert(std::get<0>(EdgeBuffer.get_current_location()) == row);
    assert(std::get<1>(EdgeBuffer.get_current_location()) == col);

    return input_ready and edge_ready;
  }
  void prefetch_input(unsigned x, unsigned y, unsigned z) {
    // the next buffer should be emtpy
    if (z == 0) {
      return;
    }
    assert(InputBuffer.is_next_empty());
    auto input_req = std::make_shared<Req>();
    input_req->addr = get_input_addr_by_location(x, y, z);
    input_req->len = z * feature_elements[current_level] * feature_size;
    input_req->req_type = mem_request::read;
    input_req->t = device_types::input_buffer;

    InputBuffer.insert_next(input_req);
    InputBuffer.set_next_location(std::make_tuple(x, y, z, current_level));
  }

  void prefetch_edge(unsigned x, unsigned y, unsigned z) {
    // the next buffer should be emtpy
    if (z == 0) {
      return;
    }
    assert(EdgeBuffer.is_next_empty());
    auto edge_req = std::make_shared<Req>();
    edge_req->addr = get_edge_addr_by_location(x, y, z);
    edge_req->len = z * feature_elements[current_level] * feature_size;
    edge_req->req_type = mem_request::read;
    edge_req->t = device_types::edge_buffer;

    EdgeBuffer.insert_next(edge_req);
    EdgeBuffer.set_next_location(std::make_tuple(x, y, z, current_level));
  }

  unsigned long long get_input_addr_by_location(unsigned x, unsigned y,
                                                unsigned z) {
    // the input addr
    unsigned start_index = 0;
    for (unsigned i = 0; i < current_level; i++) {
      start_index += total_x * feature_elements[i];
    }
    start_index += y * feature_elements[current_level];
    return (unsigned long long)&feature_vectors[start_index];
  }
  unsigned long long get_edge_addr_by_location(unsigned x, unsigned y,
                                               unsigned z) {

    // the edge addr
    return m_graph->get_dege_addr(x);
  }
};

#endif
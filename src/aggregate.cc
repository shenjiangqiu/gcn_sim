#include <aggregate.h>

#include <algorithm>
#include <assert.h>
#include <ramulator_wrapper.h>
#include <tuple>
void Aggregator::reset_all() {
  current_y = 0;
  current_x = 0;
  current_window_size = 0;

  nonempty_line_set.clear();
}
void Aggregator::reset_input() {}
void Aggregator::reset_output() { nonempty_line_set.clear(); }
std::tuple<unsigned, unsigned, unsigned> Aggregator::get_next_window() {
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

  while (
      is_empty_line(next_start_x, next_start_y + new_start_windows_size - 1)) {
    new_start_windows_size--;
    assert(new_start_windows_size != 0);
  }
  // current_x = next_start_x;
  // current_y = next_start_y;
  // current_window_size = new_start_windows_size;

  return {next_start_x, next_start_y, new_start_windows_size};
}
// given the location, get if the line is emtpy
// return the line is emtpy
bool Aggregator::is_empty_line(unsigned int x, unsigned int y) {
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

void Aggregator::cycle() {
  // output_buffer_to_comb

  // the combination logic
  InputBuffer.cycle();
  OutputBuffer.cycle();
  EdgeBuffer.cycle();
  m_mem_interface->cycle();

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

  // check the buffer ready or not
  if (current_waiting_buffer) {
    if (InputBuffer.is_current_data_ready() and OutputBuffer.is_next_empty() and
        EdgeBuffer.is_current_data_ready()) {
      current_waiting_buffer = false;
    }
  }
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
        req->len = z * slide_width[current_level];
        req->req_type = mem_request::read;
        req->t = device_types::input_buffer;
        // will send to the firt
        InputBuffer.send(req);
        auto [nx, ny, nz] = get_next_window();
        prefetch(nx, ny, nz);

        //TODO contine the cycle function;
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
          if (buffer_ready(x, y)) {
            current_window_remain_cycles =
                calculate_the_cycle_of_window(x, y, z);
            auto [nx, ny, nz] = get_next_window();
            prefetch(nx, ny, nz);
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
          if (buffer_ready(x, y)) {
            current_window_remain_cycles =
                calculate_the_cycle_of_window(x, y, z);
            auto [nx, ny, nz] = get_next_window();
            prefetch(nx, ny, nz);
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
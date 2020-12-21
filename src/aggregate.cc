#include <aggregate.h>

#include <algorithm>
#include <assert.h>
#include <ramulator_wrapper.h>
#include <tuple>
void Aggregator::reset_all() {
  current_y = 0;
  current_x = 0;
  current_window_size = 0;

  current_nonempty_line_set = false;
  nonempty_line_set.clear();
}
void Aggregator::reset_input() {}
void Aggregator::reset_output() {
  current_y = 0;

  current_window_size = 0;
  current_nonempty_line_set = false;
  nonempty_line_set.clear();
}
std::tuple<unsigned, unsigned, unsigned>
Aggregator::get_next_window(unsigned level) {
  auto next_start_y = current_y + current_window_size;
  auto next_start_x = current_x;

  reset_input();
  if (next_start_y >= total_y) {
    next_start_y = 0;
    next_start_x += slide_width[level];
    reset_output();
    if (next_start_x >= total_x) {
      // reset
      reset_all();

      return {0, 0, 0};
    }
  }

  // skipping zero lines:
  while (is_empty_line(next_start_x, next_start_y, level)) {
    next_start_y++;
    if (next_start_y >= total_y) {
      next_start_x += slide_width[level];
      next_start_y = 0;
      reset_output();
      if (next_start_x >= total_x) {
        // reset
        reset_all();

        return {0, 0, 0};
      }
    }
  }

  // shrinking:
  auto new_start_windows_size =
      std::min(slide_height[level], total_y - next_start_y);

  while (is_empty_line(next_start_x, next_start_y + new_start_windows_size - 1,
                       level)) {
    new_start_windows_size--;
    assert(new_start_windows_size != 0);
  }
  current_x = next_start_x;
  current_y = next_start_y;
  current_window_size = new_start_windows_size;

  return {next_start_x, next_start_y, new_start_windows_size};
}

bool Aggregator::is_empty_line(unsigned int x, unsigned int y, unsigned level) {
  const auto &edge_index = m_graph->get_edge_index();
  const auto &edges = m_graph->get_edges();
  if (current_nonempty_line_set == false) {
    auto edge_start = edge_index.at(x);
    auto edge_end = edge_index.at(x + slide_width[level]);
    while (edge_start < edge_end) {
      nonempty_line_set.insert(edges.at(edge_start));
      edge_start++;
    }
    current_nonempty_line_set = true;
  }

  return nonempty_line_set.count(y) == 0;
}

void Aggregator::cycle() {
  // output_buffer_to_comb

  // the combination logic

  // aggregator_to_output_buffer
  if (current_window_size != 0) {
    if (current_window_remain_cycles != 0) {
      current_window_remain_cycles--;
      if (current_window_remain_cycles == 0) {
        // finished this windows,switch to the next windows;
        auto [x, y, z] = get_next_window(current_level); // will
        current_x = x;
        current_y = y;
        current_window_size = z;
        if (x == 0 and y == 0 and z == 0) {
          // finished current level, move to next level;
          current_level++;
          if (current_level == feature_elements.size()) {
            // finished all level;
            finished_all_level = true;
          }
        } else if (buffer_ready(x, y)) {
          current_window_remain_cycles = calculate_the_cycle_of_window(x, y);
        } else {
          current_window_remain_cycles = 0;
          current_waiting_buffer = true;
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
#include <aggregate.h>

#include <algorithm>
#include <assert.h>
#include <ramulator_wrapper.h>
#include <tuple>
void Aggregator::reset_all() {
  current_y = 0;
  current_x = 0;
  current_window_size = 0;
  std::for_each(current_edge_index.begin(), current_edge_index.end(),
                [](auto &&v) { v = 0; });
  current_nonempty_line_set = false;
  nonempty_line_set.clear();
}
void Aggregator::reset_input() {}
void Aggregator::reset_output() {
  current_y = 0;
  std::for_each(current_edge_index.begin(), current_edge_index.end(),
                [](auto &&v) { v = 0; });
  current_window_size = 0;
  current_nonempty_line_set = false;
  nonempty_line_set.clear();
}
std::tuple<unsigned, unsigned, unsigned> Aggregator::get_next_window() {
  auto next_start_y = current_y + current_window_size;
  auto next_start_x = current_x;

  reset_input();
  if (next_start_y >= total_y) {
    next_start_y = 0;
    next_start_x += slide_width;
    reset_output();
    if (next_start_x >= total_x) {
      // reset
      reset_all();

      return {0, 0, 0};
    }
  }

  // skipping zero lines:
  while (is_empty_line(next_start_x, next_start_y)) {
    next_start_y++;
    if (next_start_y >= total_y) {
      next_start_x += slide_width;
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
  auto new_start_windows_size = std::min(slide_height, total_y - next_start_y);

  while (
      is_empty_line(next_start_x, next_start_y + new_start_windows_size - 1)) {
    new_start_windows_size--;
    assert(new_start_windows_size != 0);
  }
  current_x = next_start_x;
  current_y = next_start_y;
  current_window_size = new_start_windows_size;

  return {next_start_x, next_start_y, new_start_windows_size};
}

bool Aggregator::is_empty_line(unsigned int x, unsigned int y) {
  const auto &edge_index = m_graph->get_edge_index();
  const auto &edges = m_graph->get_edges();
  if (current_nonempty_line_set == false) {
    auto edge_start = edge_index.at(x);
    auto edge_end = edge_index.at(x + slide_width);
    while (edge_start < edge_end) {
      nonempty_line_set.insert(edges.at(edge_start));
      edge_start++;
    }
    current_nonempty_line_set = true;
  }

  return nonempty_line_set.count(y) == 0;
}

void Aggregator::cycle() {
  
}
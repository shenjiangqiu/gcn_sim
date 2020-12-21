#ifndef BUFFER_H
#define BUFFER_H
#include <assert.h>
#include <string>
class Buffer {
private:
  std::string name;

  unsigned long long current_addr = 0;
  unsigned current_lenghth = 0;
  bool current_data_ready = false;
  bool current_task_ready = false;

  unsigned long long next_addr = 0;
  unsigned next_lenghth = 0;
  bool next_data_ready = false;
  bool next_task_ready = false;

public:
  Buffer(std::string name) : name(name) {}
  // when next buffer is ready, move it to current
  void add_task_and_move(unsigned long long addr, unsigned lenghth);
  // move the next buffer to current, and do not add new task.
  void just_move_the_buffer() {
    current_addr = next_addr;
    current_lenghth = next_lenghth;

    current_data_ready = true;
    current_task_ready = true;
    next_task_ready = false;
    next_data_ready = false;
  }

  

  unsigned long long get_current_addr() { return current_addr; }
  unsigned get_current_lenghth() { return current_lenghth; }
  bool is_current_data_ready() { return current_data_ready; }
  bool is_current_task_ready() { return current_task_ready; }

  unsigned long long get_next_addr() { return next_addr; }
  unsigned get_next_lenghth() { return next_lenghth; }
  bool is_next_data_ready() { return next_data_ready; }
  bool is_next_task_ready() { return next_task_ready; }

  void set_current_data_ready() {
    assert(current_data_ready == false);
    current_data_ready = true;
  }
  void set_current_task_ready() {
    assert(current_task_ready == false);
    current_task_ready = true;
  }
  void set_next_data_ready() {
    assert(next_data_ready == false);
    next_data_ready = true;
  }
  void set_next_task_ready() {
    assert(next_task_ready == false);
    next_task_ready = true;
  }
  
};
#endif /* BUFFER_H */

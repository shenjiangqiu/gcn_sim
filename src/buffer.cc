#include <buffer.h>

void Buffer::add_task_and_move(unsigned long long addr, unsigned int lenghth) {
  // when the next buffer is read, move it to the current buffer for futher
  // service.
  

  current_addr = next_addr;
  current_lenghth = next_lenghth;

  current_task_ready = true;
  current_data_ready = true;

  next_addr = addr;
  next_lenghth = lenghth;
  next_task_ready = false;
  next_data_ready = false;
}

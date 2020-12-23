#include <buffer.h>

void Buffer::add_task_and_move(unsigned long long addr, unsigned int lenghth) {
  // when the next buffer is read, move it to the current buffer for futher
  // service.

  current_addr = next_addr;
  current_lenghth = next_lenghth;

  current_task_ready = next_task_ready;
  current_data_ready = next_data_ready;
  current_task_sent = next_task_sent;

  next_addr = addr;
  next_lenghth = lenghth;
  next_task_ready = true;
  next_data_ready = false;
  next_task_sent = false;

  current_buffer_task = next_buffer_task;
  next_buffer_task = nullptr;
}

void Buffer::cycle() {
  /***********************************************************
   *                    // DO THE TASKS:                     *
   * // WHEN CURRENT OR NEXT NOT READY, SEND THEM TO MEMORY, *
   * // WHEN CURRENT OR NEXT RECIVED, MARK THEM AS RECIEVED  *
   ***********************************************************/
  if (!in_task_queu.empty()) {
    if (!current_buffer_task) {
      current_buffer_task = in_task_queu.front();
      in_task_queu.pop();
      current_task_ready = true;
      current_data_ready = false;
      current_task_sent = false;
    } else if (!next_buffer_task) {
      next_buffer_task = in_task_queu.front();
      in_task_queu.pop();
      next_task_ready = true;
      next_data_ready = false;
      next_task_sent = false;
    }
  }

  if (current_task_ready and !current_data_ready and !current_task_sent) {
    // send the task
    current_task_sent = true;
    assert(current_buffer_task);
    out_send_queue.push(current_buffer_task);
  } else if (next_task_ready and !next_data_ready and !next_task_sent) {
    next_task_sent = true;
    assert(next_buffer_task);
    out_send_queue.push(next_buffer_task);
  }

  if (!ret_queue.empty()) {
    auto ret = ret_queue.front();
    if (current_buffer_task and current_buffer_task->id == ret->id) {
      // current ready
      current_data_ready = true;

    } else {
      assert(next_buffer_task and next_buffer_task->id == ret->id);
      next_data_ready = true;
    }
  }
}